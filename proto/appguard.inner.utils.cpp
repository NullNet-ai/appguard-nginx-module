#include "appguard.inner.utils.h"

#include <array>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>

namespace appguard::inner_utils
{
    bool parse_sockaddr(const sockaddr *addr, std::string &ip, uint16_t &port)
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

    std::unordered_map<std::string, std::string> parse_ngx_request_headers(ngx_http_request_t *request)
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

            retval.insert(header_key, header_value);
        }

        return retval;
    }

    std::unordered_map<std::string, std::string> parse_query_parameters(const std::string &uri)
    {
        std::unordered_map<std::string, std::string> retval;

        auto pos = uri.find('?');
        if (pos == std::string::npos || pos + 1 >= uri.length())
            return retval; // No query string

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
}