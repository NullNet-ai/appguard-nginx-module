#include "appguard.wrapper.hpp"
#include "appguard.client.exception.hpp"

#include <grpcpp/grpcpp.h>

AppGuardWrapper::AppGuardWrapper(std::unique_ptr<AppGuardWrapper::Stub> stub)
    : stub(std::move(stub))
{
}

AppGuardWrapper AppGuardWrapper::create(const std::string &server_addr)
{
    auto channel = grpc::CreateChannel(server_addr, grpc::InsecureChannelCredentials());
    auto stub = appguard::AppGuard::NewStub(channel);
    return AppGuardWrapper(std::move(stub));
}

appguard::AppGuardTcpResponse
AppGuardWrapper::HandleTcpConnection(appguard::AppGuardTcpConnection connection)
{
    grpc::ClientContext context;
    appguard::AppGuardTcpResponse response;

    if (auto status = this->stub->HandleTcpConnection(&context, connection, &response); !status.ok())
    {
        throw AppGuardClientException(status.error_code(), status.error_message());
    }

    return response;
}

appguard::AppGuardResponse
AppGuardWrapper::HandleHttpRequest(appguard::AppGuardHttpRequest request)
{
    grpc::ClientContext context;
    appguard::AppGuardResponse response;

    if (auto status = this->stub->HandleHttpRequest(&context, request, &response); !status.ok())
    {
        throw AppGuardClientException(status.error_code(), status.error_message());
    }

    return response;
}

appguard::AppGuardResponse
AppGuardWrapper::HandleHttpResponse(appguard::AppGuardHttpResponse request)
{
    grpc::ClientContext context;
    appguard::AppGuardResponse response;

    if (auto status = this->stub->HandleHttpResponse(&context, request, &response); !status.ok())
    {
        throw AppGuardClientException(status.error_code(), status.error_message());
    }

    return response;
}