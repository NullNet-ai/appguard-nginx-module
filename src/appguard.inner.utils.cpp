#include "appguard.inner.utils.hpp"

#include <array>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>

namespace appguard::inner_utils
{
    static bool parse_sockaddr(const sockaddr *addr, std::string &ip, uint16_t &port)
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

    static std::unordered_map<std::string, std::string> parse_ngx_request_headers(ngx_http_request_t *request)
    {
        std::unordered_map<std::string, std::string> retval = {};

        ngx_list_part_t *part = &request->headers_in.headers.part;
        ngx_table_elt_t *h = reinterpret_cast<ngx_table_elt_t *>(part->elts);

        for (ngx_uint_t i = 0;; i++)
        {
            if (i >= part->nelts)
            {
                if (!part->next)
                    break;

                part = part->next;
                h = reinterpret_cast<ngx_table_elt_t *>(part->elts);
                i = 0;
            }

            std::string header_key(reinterpret_cast<char *>(h[i].key.data), h[i].key.len);
            std::string header_value(reinterpret_cast<char *>(h[i].value.data), h[i].value.len);

            retval.try_emplace(header_key, header_value);
        }

        return retval;
    }

    static std::unordered_map<std::string, std::string> parse_query_parameters(const std::string &uri)
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

    appguard::AppGuardTcpConnection ExtractTcpConnectionInfo(ngx_http_request_t *request)
    {
        appguard::AppGuardTcpConnection tcp_connection;

        std::string ip_address{};
        uint16_t port{};

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

        return tcp_connection;
    }

    appguard::AppGuardHttpRequest ExtractHttpRequestInfo(ngx_http_request_t *request)
    {
        appguard::AppGuardHttpRequest http_request;

        std::string uri(reinterpret_cast<char *>(request->uri.data), request->uri.len);

        auto query_params = appguard::inner_utils::parse_query_parameters(uri);
        for (const auto &[key, value] : query_params)
        {
            http_request.mutable_query()->try_emplace(key, value);
        }

        http_request.set_original_url(uri);

        auto headers = appguard::inner_utils::parse_ngx_request_headers(request);
        for (const auto &[key, value] : headers)
        {
            http_request.mutable_headers()->try_emplace(key, value);
        }

        std::string method(reinterpret_cast<char *>(request->method_name.data), request->method_name.len);
        http_request.set_method(method);

        return http_request;
    }
}