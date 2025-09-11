#include "appguard.wrapper.hpp"
#include "appguard.uclient.exception.hpp"
#include "appguard.http.ucache.hpp"

#include <grpcpp/grpcpp.h>
#include <fstream>
#include <sstream>

static std::string readServerCertificate(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);

    THROW_IF_CUSTOM(!file.is_open(), AppGuardStatusCode::APPGUARD_CERTIFICATE_NOT_FOUND);

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static inline std::shared_ptr<grpc::Channel> OpenChannel(const std::string &addr, bool tls, const std::string &server_cert_path)
{
    if (tls)
    {
        grpc::SslCredentialsOptions options;

        options.pem_root_certs =
            server_cert_path.empty() ? "" : readServerCertificate(server_cert_path);

        auto credentials = grpc::SslCredentials(options);
        return grpc::CreateChannel(addr, credentials);
    }
    else
    {
        auto credentials = grpc::InsecureChannelCredentials();
        return grpc::CreateChannel(addr, credentials);
    }
}

AppGuardWrapper::AppGuardWrapper(std::shared_ptr<grpc::Channel> channel, const std::string &installation_code)
    : channel(channel), stream(new AppGuardStream(channel, installation_code))
{
}

AppGuardWrapper AppGuardWrapper::CreateClient(AppGaurdClientInfo client_info, const std::chrono::milliseconds &deadline)
{
    static std::map<AppGaurdClientInfo, AppGuardWrapper> clients{};

    if (auto iter = clients.find(client_info); iter != clients.end())
    {
        return iter->second;
    }

    auto channel = OpenChannel(client_info.server_addr, client_info.tls, client_info.server_cert_path);
    std::chrono::system_clock::time_point deadline_time = std::chrono::system_clock::now() + deadline;

    THROW_IF_CUSTOM(!channel->WaitForConnected(deadline_time), AppGuardStatusCode::APPGUARD_CONNECTION_TIMEOUT);

    auto client = AppGuardWrapper(channel, client_info.installation_code);
    const auto [_, success] = clients.emplace(client_info, client);

    THROW_IF_CUSTOM(!success, AppGuardStatusCode::APPGUARD_FAILED_TO_SAVE_CLIENT);

    return client;
}

appguard::AppGuardTcpResponse
AppGuardWrapper::HandleTcpConnection(appguard::AppGuardTcpConnection connection)
{
    auto token = this->AcquireToken();
    connection.set_token(token);

    auto stub = appguard::AppGuard::NewStub(this->channel);

    grpc::ClientContext context;
    appguard::AppGuardTcpResponse response;

    auto status = stub->HandleTcpConnection(&context, connection, &response);
    THROW_IF_GRPC(status);

    return response;
}

appguard_commands::FirewallPolicy
AppGuardWrapper::HandleHttpRequest(appguard::AppGuardHttpRequest &request)
{
    auto cacheKey = HttpRequestCacheKey::FromRequest(request);
    auto &cache = AppguardHttpCache<HttpRequestCacheKey>::GetInstance();

    if (auto cacheEntry = cache.Get(cacheKey); cacheEntry.has_value() && cache.IsEnabled())
    {
        return cacheEntry.value();
    }

    auto token = this->AcquireToken();
    request.set_token(token);

    auto stub = appguard::AppGuard::NewStub(this->channel);

    grpc::ClientContext context;
    appguard::AppGuardResponse response;

    auto status = stub->HandleHttpRequest(&context, request, &response);
    THROW_IF_GRPC(status);

    if (cache.IsEnabled())
    {
        cache.Put(std::move(cacheKey), response.policy());
    }

    return response.policy();
}

appguard_commands::FirewallPolicy
AppGuardWrapper::HandleHttpResponse(appguard::AppGuardHttpRequest &request, appguard::AppGuardHttpResponse &response)
{
    auto token = this->AcquireToken();
    response.set_token(token);

    auto stub = appguard::AppGuard::NewStub(this->channel);

    grpc::ClientContext context;
    appguard::AppGuardResponse retval;

    auto status = stub->HandleHttpResponse(&context, response, &retval);
    THROW_IF_GRPC(status);

    auto cacheKey = HttpRequestCacheKey::FromRequest(request);
    auto &cache = AppguardHttpCache<HttpRequestCacheKey>::GetInstance();

    if (cache.IsEnabled())
    {
        cache.Put(std::move(cacheKey), retval.policy());
    }

    return retval.policy();
}

std::string AppGuardWrapper::AcquireToken() const
{
    THROW_IF_CUSTOM(!this->stream->Running(), AppGuardStatusCode::APPGUARD_AUTH_STREAM_NOT_RUNNING);

    auto token = this->stream->WaitForToken();

    THROW_IF_CUSTOM(token.empty(), AppGuardStatusCode::APPGUARD_FAILED_TO_ACQUIRE_TOKEN);

    return token;
}
