#include "appguard.stream.hpp"

AppGuardStream::AppGuardStream(
    std::shared_ptr<grpc::Channel> channel,
    const std::string &app_id,
    const std::string &app_secret)
    : app_id(app_id),
      app_secret(app_secret),
      token({}),
      device_status(appguard::DeviceStatus::DS_UNKNOWN),
      running(false),
      channel(channel)
{
    this->Start();
}

AppGuardStream::~AppGuardStream()
{
    this->Stop();
}

#include <sstream>

static void log_custom_message(const char *message)
{
    FILE *log_file = fopen("/var/log/nginx/custom.log", "a"); // open for appending
    if (log_file == NULL)
    {
        perror("fopen failed");
        return;
    }

    // Optional: include timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[custom] [%s] %s\n", timestamp, message);
    fclose(log_file);
}

void AppGuardStream::Start()
{
    this->running = true;
    this->thread = std::thread([this]()
                               {
        while (this->running) {
           
            { 
                std::lock_guard lock(this->context_mutex);
                this->context = std::make_unique<grpc::ClientContext>();
            }

            appguard::HeartbeatRequest request;

            request.set_app_id(this->app_id);
            request.set_app_secret(this->app_secret);

            auto stub = appguard::AppGuard::NewStub(this->channel);
            auto reader = stub->Heartbeat(this->context.get(), request);

            appguard::HeartbeatResponse response;
            while (this->running && reader->Read(&response)) {
                this->SetToken(response.token());
                this->SetDeviceStatus(response.status());
            }

            auto status = reader->Finish();
            if (!status.ok() && this->running) {
                std::stringstream ss;

                ss << "Error auth stream " << status.error_code() << " " << status.error_message() << " " << status.error_details() << std::endl;
                log_custom_message(ss.str().data());
                this->SetToken("");
                this->SetDeviceStatus(appguard::DeviceStatus::DS_UNKNOWN);

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
            this->context->TryCancel();
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