#include "appguard.h"
#include "appguard.pb.h"
#include "appguard.grpc.pb.h"
#include "appguard.inner.utils.h"

#include <grpcpp/grpcpp.h>
#include <optional>

struct appguard_handle_t
{
    std::mutex mutex;
    std::unique_ptr<appguard::AppGuard::Stub> stub;
};

appguard_handle_t *new_appguard_handle(const char *server_addr)
{
    auto retval = new (std::nothrow) appguard_handle_t;
    if (!retval)
        return nullptr;

    try
    {
        auto channel = grpc::CreateChannel(server_addr, grpc::InsecureChannelCredentials());
        retval->stub = appguard::AppGuard::NewStub(channel);
        return retval;
    }
    catch (...)
    {
        delete retval;
        return nullptr;
    }
}

void delete_appguard_handle(appguard_handle_t *handle)
{
    if (handle)
    {
        handle->stub.reset();
        delete handle;
    }
}

static std::optional<appguard::AppGuardTcpInfo>
request_tcp_info(const appguard_handle_t *handle, ngx_http_request_t *request)
{
    std::string ip_address{};
    uint16_t port{};

    appguard::AppGuardTcpConnection tcp_connection;

    if (appguard::inner_utils::parse_sockaddr(request->connection->sockaddr, ip_address, port))
    {
        tcp_connection.set_source_ip(ip_address);
        tcp_connection.set_source_port(port);
    }

    if (appguard::inner_utils::parse_sockaddr(request->connection->local_sockaddr, ip_address, port))
    {
        tcp_connection.set_destination_ip(ip_address);
        tcp_connection.set_destination_port(port);
    }

    std::string protocol(reinterpret_cast<char *>(request->http_protocol.data), request->http_protocol.len);
    if (!protocol.empty())
        tcp_connection.set_protocol(protocol);

    appguard::AppGuardTcpResponse response;
    grpc::ClientContext context;

    std::lock_guard guard(handle->mutex);
    if (handle->stub->HandleTcpConnection(&context, tcp_connection, &response).ok())
    {
        return {response.tcp_info()};
    }
    else
    {
        return std::nullopt;
    }
}

static std::optional<appguard::FirewallPolicy>
handle_http_request_submittion(appguard_handle_t *handle, ngx_http_request_t *request, appguard::AppGuardTcpInfo &&tcp_info)
{
    appguard::AppGuardHttpRequest http_request;

    std::string uri(reinterpret_cast<char *>(request->uri.data), request->uri.len);

    auto query_params = appguard::inner_utils::parse_query_parameters(uri);
    for (const auto &[key, value] : query_params)
    {
        http_request.mutable_query()->insert(key, value);
    }

    http_request.set_original_url(uri);

    auto headers = appguard::inner_utils::parse_ngx_request_headers(request);
    for (const auto &[key, value] : headers)
    {
        http_request.mutable_headers()->insert(key, value);
    }

    std::string method(reinterpret_cast<char *>(request->method_name.data), request->method_name.len);
    http_request.set_method(method);

    http_request.set_allocated_tcp_info(&tcp_info);

    appguard::AppGuardResponse response;
    grpc::ClientContext context;

    std::lock_guard guard(handle->mutex);

    if (handle->stub->HandleHttpRequest(&context, http_request, &response).ok())
    {
        return {response.policy()};
    }
    else
    {
        return std::nullopt;
    }
}

appguard_action_t handle_http_request(appguard_handle_t *handle, ngx_http_request_t *request)
{
    try
    {
        auto tcp_info_result = request_tcp_info(handle, request);

        if (!tcp_info_result.has_value())
        {
            return appguard_action_t::APPGUARD_UNKNOWN;
        }

        auto tcp_info = tcp_info_result.value();

        auto policy = handle_http_request_submittion(handle, request, std::move(tcp_info));

        switch (policy.value_or(appguard::FirewallPolicy::UNKNOWN))
        {
        case appguard::FirewallPolicy::ALLOW:
            return appguard_action_t::APPGUARD_ALLOW;
        case appguard::FirewallPolicy::DENY:
            return appguard_action_t::APPGUARD_DENY;
        default:
            return appguard_action_t::APPGUARD_UNKNOWN;
        }
    }
    catch (...)
    {
        return appguard_action_t::APPGUARD_UNKNOWN;
    }
}