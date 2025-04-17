#include "appguard.inner.utils.hpp"

#include <array>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>

namespace appguard::inner_utils
{
    static bool ParseSocketAddr(const sockaddr *addr, std::string &ip, uint16_t &port)
    {
        if (!addr)
            return false;

        std::array<char, INET6_ADDRSTRLEN> buffer{};

        if (addr->sa_family == AF_INET)
        {
            const sockaddr_in *addr_in = reinterpret_cast<const sockaddr_in *>(addr);
            if (!inet_ntop(AF_INET, &(addr_in->sin_addr), buffer.data(), INET_ADDRSTRLEN))
                return false;

            ip = buffer.data();
            port = ntohs(addr_in->sin_port);
            return true;
        }
        else if (addr->sa_family == AF_INET6)
        {
            const sockaddr_in6 *addr_in6 = reinterpret_cast<const sockaddr_in6 *>(addr);
            if (!inet_ntop(AF_INET6, &(addr_in6->sin6_addr), buffer.data(), INET6_ADDRSTRLEN))
                return false;

            ip = buffer.data();
            port = ntohs(addr_in6->sin6_port);
            return true;
        }

        return false;
    }

    static std::unordered_map<std::string, std::string> NgxParseHeadersList(ngx_list_t *headers)
    {
        std::unordered_map<std::string, std::string> retval;

        ngx_list_part_t *part = &headers->part;
        ngx_table_elt_t *header = static_cast<ngx_table_elt_t *>(part->elts);

        for (ngx_uint_t i = 0; /* infinite */; i++)
        {
            if (i >= part->nelts)
            {
                if (part->next == nullptr)
                    break;
                part = part->next;
                header = static_cast<ngx_table_elt_t *>(part->elts);
                i = 0;
            }

            retval.try_emplace(
                NgxStringToStdString(&header[i].key),
                NgxStringToStdString(&header[i].value));
        }

        return retval;
    }

    static std::unordered_map<std::string, std::string> NgxParseResponseHeaders(ngx_http_request_t *request)
    {
        return NgxParseHeadersList(&request->headers_out.headers);
    }

    static std::unordered_map<std::string, std::string> NgxParseRequestHeaders(ngx_http_request_t *request)
    {
        return NgxParseHeadersList(&request->headers_in.headers);
    }

    static std::unordered_map<std::string, std::string> ParseQueryParameters(const std::string &uri)
    {
        std::unordered_map<std::string, std::string> retval;

        auto pos = uri.find('?');
        if (pos == std::string::npos || pos + 1 >= uri.length())
            return retval;

        std::string query = uri.substr(pos + 1);
        std::stringstream ss(query);
        std::string pair;

        while (std::getline(ss, pair, '&'))
        {
            auto eq_pos = pair.find('=');
            if (eq_pos != std::string::npos)
            {
                std::string key = pair.substr(0, eq_pos);
                std::string value = pair.substr(eq_pos + 1);
                retval[key] = value;
            }
            else
            {
                retval[pair] = "";
            }
        }

        return retval;
    }

    std::string NgxStringToStdString(ngx_str_t *str)
    {
        if (!str || !str->data || str->data == 0)
            return std::string();

        return std::string(
            reinterpret_cast<char *>(str->data),
            str->len);
    }

    appguard::AppGuardTcpConnection ExtractTcpConnectionInfo(ngx_http_request_t *request)
    {
        appguard::AppGuardTcpConnection tcp_connection;

        std::string ip_address{};
        uint16_t port{};

        if (ParseSocketAddr(request->connection->sockaddr, ip_address, port))
        {
            tcp_connection.set_source_ip(ip_address);
            tcp_connection.set_source_port(port);
        }

        if (ParseSocketAddr(request->connection->local_sockaddr, ip_address, port))
        {
            tcp_connection.set_destination_ip(ip_address);
            tcp_connection.set_destination_port(port);
        }

        auto protocol = NgxStringToStdString(&request->http_protocol);
        tcp_connection.set_protocol(protocol);

        return tcp_connection;
    }

    appguard::AppGuardHttpRequest ExtractHttpRequestInfo(ngx_http_request_t *request)
    {
        appguard::AppGuardHttpRequest http_request;

        std::string uri(reinterpret_cast<char *>(request->uri.data), request->uri.len);

        auto query_params = ParseQueryParameters(uri);
        for (const auto &[key, value] : query_params)
        {
            http_request.mutable_query()->try_emplace(key, value);
        }

        http_request.set_original_url(uri);

        auto headers = NgxParseRequestHeaders(request);
        for (const auto &[key, value] : headers)
        {
            http_request.mutable_headers()->try_emplace(key, value);
        }

        std::string method = NgxStringToStdString(&request->method_name);

        http_request.set_method(method);

        return http_request;
    }

    appguard::AppGuardHttpResponse ExtractHttpResponseInfo(ngx_http_request_t *request) 
    {
        appguard::AppGuardHttpResponse http_response;

        auto headers = NgxParseResponseHeaders(request);

        for (const auto &[key, value] : headers)
        {
            http_response.mutable_headers()->try_emplace(key, value);
        }

        http_response.set_code(request->headers_out.status);

        return http_response;
    }

    appguard::FirewallPolicy StringToFirewallPolicy(const std::string &str)
    {
        auto lowercase = str;
        std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), [](unsigned char c)
                       { return static_cast<unsigned char>(std::tolower(c)); });

        if (lowercase == "allow")
            return appguard::FirewallPolicy::ALLOW;
        else
            return appguard::FirewallPolicy::DENY;
    }
}