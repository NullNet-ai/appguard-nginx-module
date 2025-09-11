#include "appguard.stream.hpp"
#include "appguard.inner.utils.hpp"
#include "appguard.storage.hpp"
#include "appguard.uclient.exception.hpp"
#include "appguard.http.ucache.hpp"

#define CLIENT_CATEGORY "AppGuard Client"
#define CLIENT_TYPE "NGINX"
#define MAKE_APP_ID_STORAGE_KEY(code) "app_id:" + code
#define MAKE_APP_SECRET_STORAGE_KEY(code) "app_secret:" + code

namespace internal
{
    using CM = appguard_commands::ClientMessage;
    using SM = appguard_commands::ServerMessage;

    static bool PerformAuthorization(
        const AppGuardStream *stream,
        grpc::ClientReaderWriter<CM, SM> *rw_stream,
        const std::string &installation_code,
        const std::string &target_os,
        const std::string &uuid)
    {
        CM request;

        request.mutable_authorization_request()->set_code(installation_code);
        request.mutable_authorization_request()->set_type(CLIENT_TYPE);
        request.mutable_authorization_request()->set_category(CLIENT_CATEGORY);
        request.mutable_authorization_request()->set_target_os(target_os);
        request.mutable_authorization_request()->set_uuid(uuid);

        if (!rw_stream->Write(request))
        {
            ngx_log_error(
                NGX_LOG_ERR,
                ngx_cycle->log,
                0,
                "AppGuard: Failed to send authorization request");
            return false;
        }

        SM response;
        while (stream->Running() && rw_stream->Read(&response))
        {
            if (response.has_device_authorized())
            {
                const auto &data = response.device_authorized();

                if (data.has_app_id())
                {
                    const auto key = MAKE_APP_ID_STORAGE_KEY(installation_code);
                    Storage::GetInstance().Set(key, data.app_id());
                }

                if (data.has_app_secret())
                {
                    const auto key = MAKE_APP_SECRET_STORAGE_KEY(installation_code);
                    Storage::GetInstance().Set(key, data.app_secret());
                }

                return true;
            }

            if (response.has_authorization_rejected())
            {
                ngx_log_error(
                    NGX_LOG_ERR,
                    ngx_cycle->log,
                    0,
                    "AppGuard Server Rejected Authorization");
                return false;
            }
        }

        ngx_log_error(
            NGX_LOG_ERR,
            ngx_cycle->log,
            0,
            "AppGuard Server Authorization failed");

        return false;
    }

    static bool PerformAuthentication(
        const AppGuardStream *stream,
        grpc::ClientReaderWriter<CM, SM> *rw_stream,
        const std::string &installation_code)
    {
        Storage::GetInstance().Update();

        auto app_id = Storage::GetInstance().Get(MAKE_APP_ID_STORAGE_KEY(installation_code));
        auto app_secret = Storage::GetInstance().Get(MAKE_APP_SECRET_STORAGE_KEY(installation_code));

        if (!app_id.has_value() || !app_secret.has_value())
        {
            ngx_log_error(
                NGX_LOG_ERR,
                ngx_cycle->log,
                0,
                "AppGuard: Failed to obtain credentials.");

            return false;
        }

        appguard_commands::ClientMessage authmsg;
        authmsg.mutable_authentication()->set_app_id(app_id.value());
        authmsg.mutable_authentication()->set_app_secret(app_secret.value());

        if (!rw_stream->Write(authmsg))
        {
            ngx_log_error(
                NGX_LOG_ERR,
                ngx_cycle->log,
                0,
                "AppGuard: Failed to send authentication.");

            return false;
        }

        return true;
    }
}

AppGuardStream::AppGuardStream(
    std::shared_ptr<grpc::Channel> channel,
    const std::string &installation_code)
    : installation_code(installation_code),
      token({}),
      running(false),
      channel(channel)
{
    this->Start();
}

AppGuardStream::~AppGuardStream()
{
    this->Stop();
}

void AppGuardStream::Start()
{

    std::string device_uuid = appguard::inner_utils::GetSystemUUID();
    std::string target_os = appguard::inner_utils::GetOsAsString();

    this->running = true;
    this->thread = std::thread(
        [this,
         uuid = std::move(device_uuid),
         os = std::move(target_os)]()
        {
        while (this->running) {
            { 
                std::lock_guard lock(this->context_mutex);
                this->context = std::make_unique<grpc::ClientContext>();
            }

            auto stub = appguard::AppGuard::NewStub(this->channel);
            auto rw_stream = stub->ControlChannel(this->context.get());

            // Step 1: Send Authorization request and await for the verdict
            if (!internal::PerformAuthorization(this, rw_stream.get(), installation_code, os, uuid)) {
                this->running = false;
                std::lock_guard lock(this->context_mutex);
                this->context.reset();
                return;
            }

            // Step 2: Collect credentials and send authentication
            if (!internal::PerformAuthentication(this, rw_stream.get(), installation_code)) {
                this->running = false;
                std::lock_guard lock(this->context_mutex);
                this->context.reset();
                return;
            }

            // Step 3: Accept messages from the server
            internal::SM message;
            while (this->Running() && rw_stream->Read(&message)) {
                if (const auto& token = message.update_token_command(); !token.empty())
                {
                    this->SetToken(token);
                    continue;
                }

                if (message.has_device_deauthorized()) {
                    IGNORE_ALL_EXCEPTIONS({
                        Storage::GetInstance().Clear();
                    });

                    ngx_log_error(
                        NGX_LOG_ERR,
                        ngx_cycle->log,
                        0,
                        "AppGuard: Server sent de-authorization");

                    this->running = false;
                    std::lock_guard lock(this->context_mutex);
                    this->context.reset();
                    return;
                }

                if (message.has_set_firewall_defaults())
                {
                    const auto command = message.set_firewall_defaults();
                    auto& instance = AppguardHttpCache<HttpRequestCacheKey>::GetInstance();
                    
                    instance.Clear();
                    instance.Enable(command.cache());
                    continue;
                }
            }

            auto status = rw_stream->Finish();
            if (!status.ok() && this->running) {
                this->SetToken("");
                this->running = false;
                std::chrono::seconds timeout(5);
                std::this_thread::sleep_for(timeout);
                this->running = true;
            }

            this->context.reset();
        } 
        this->running = false; });
}

void AppGuardStream::Stop()
{
    if (!this->running)
        return;

    this->running = false;

    {
        std::lock_guard lock(this->context_mutex);
        if (this->context)
        {
            this->context->TryCancel();
        }
    }

    if (this->thread.joinable())
        this->thread.join();
}

void AppGuardStream::SetToken(const std::string &token)
{
    {
        std::lock_guard lock(this->token_mutex);
        this->token = token;
    }

    this->token_cv.notify_all();
}

std::string AppGuardStream::WaitForToken(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(this->token_mutex);

    if (!this->token.empty())
        return this->token;

    token_cv.wait_for(lock, timeout, [this]
                      { return !this->token.empty(); });

    return this->token;
}