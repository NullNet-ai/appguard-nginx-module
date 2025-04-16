#include "appguard.wrapper.hpp"
#include "appguard.client.exception.hpp"

#include <grpcpp/grpcpp.h>

static inline std::shared_ptr<grpc::Channel> OpenChannel(const std::string &addr, bool tls)
{
    if (tls)
    {
        grpc::SslCredentialsOptions options;
        auto credentials = grpc::SslCredentials(options);
        return grpc::CreateChannel(addr, credentials);
    }
    else
    {
        auto credentials = grpc::InsecureChannelCredentials();
        return grpc::CreateChannel(addr, credentials);
    }
}

AppGuardWrapper::AppGuardWrapper(
    std::shared_ptr<grpc::Channel> channel,
    const std::string &app_id,
    const std::string &app_secret)
    : channel(channel), stream(new AppGuardStream(channel, app_id, app_secret))
{
}

AppGuardWrapper AppGuardWrapper::CreateClient(AppGaurdClientInfo client_info, const std::chrono::milliseconds &deadline)
{
    static std::map<AppGaurdClientInfo, AppGuardWrapper> clients{};

    if (auto iter = clients.find(client_info); iter != clients.end())
    {
        return iter->second;
    }

    auto channel = OpenChannel(client_info.server_addr, client_info.tls);
    std::chrono::system_clock::time_point deadline_time = std::chrono::system_clock::now() + deadline;
    if (!channel->WaitForConnected(deadline_time))
    {
        throw AppGuardClientException(grpc::StatusCode::UNAVAILABLE, "Connection timed out");
    }

    auto client = AppGuardWrapper(channel, client_info.app_id, client_info.app_secret);
    const auto [_, success] = clients.emplace(client_info, client);

    if (!success)
        throw AppGuardClientException(grpc::StatusCode::UNKNOWN, "Failed to save the connection");

    return client;
}

appguard::AppGuardTcpResponse
AppGuardWrapper::HandleTcpConnection(appguard::AppGuardTcpConnection connection)
{
    this->ValidateStatus();

    auto token = this->AcquireToken();
    connection.set_token(token);

    auto stub = appguard::AppGuard::NewStub(this->channel);

    grpc::ClientContext context;
    appguard::AppGuardTcpResponse response;

    if (auto status = stub->HandleTcpConnection(&context, connection, &response); !status.ok())
    {
        throw AppGuardClientException(status.error_code(), status.error_message());
    }

    return response;
}

appguard::AppGuardResponse
AppGuardWrapper::HandleHttpRequest(appguard::AppGuardHttpRequest request)
{
    this->ValidateStatus();

    auto token = this->AcquireToken();
    request.set_token(token);

    auto stub = appguard::AppGuard::NewStub(this->channel);

    grpc::ClientContext context;
    appguard::AppGuardResponse response;

    if (auto status = stub->HandleHttpRequest(&context, request, &response); !status.ok())
    {
        throw AppGuardClientException(status.error_code(), status.error_message());
    }

    return response;
}

appguard::AppGuardResponse
AppGuardWrapper::HandleHttpResponse(appguard::AppGuardHttpResponse request)
{
    this->ValidateStatus();

    auto token = this->AcquireToken();
    request.set_token(token);

    auto stub = appguard::AppGuard::NewStub(this->channel);

    grpc::ClientContext context;
    appguard::AppGuardResponse response;

    if (auto status = stub->HandleHttpResponse(&context, request, &response); !status.ok())
    {
        throw AppGuardClientException(status.error_code(), status.error_message());
    }

    return response;
}

void AppGuardWrapper::ValidateStatus() const
{
    // @TODO:
    // Probably allow only ACTIVE devices

    // if (!this->stream->IsDeviceStatusActive())
    //     throw AppGuardClientException(
    //         grpc::StatusCode::ABORTED,
    //         "Aborting gRPC request, since device is not active."
    //     );
}

std::string AppGuardWrapper::AcquireToken() const
{
    auto token = this->stream->WaitForToken();

    if (token.empty())
    {
        throw AppGuardClientException(grpc::StatusCode::UNAUTHENTICATED, "Failed to acquire authentication token");
    }

    return token;
}
