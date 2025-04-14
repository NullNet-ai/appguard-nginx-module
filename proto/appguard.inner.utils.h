#ifndef __APPGUARD_INNER_UTILS_H__
#define __APPGUARD_INNER_UTILS_H__

#include <sys/socket.h>
#include <string>
#include <unordered_map>
#include <ngx_http.h>

namespace appguard::inner_utils
{
    /**
     * @brief Parses a sockaddr structure to extract the IP address and port.
     *
     * Supports both IPv4 and IPv6 address families (`AF_INET` and `AF_INET6`).
     *
     * @param addr Pointer to a sockaddr structure (either `sockaddr_in` or `sockaddr_in6`).
     * @param ip Output reference to a `std::string` where the IP address will be stored.
     * @param port Output reference to a `uint16_t` where the port number will be stored.
     * @return `true` if parsing was successful (valid and supported address family), `false` otherwise.
     */
    bool parse_sockaddr(const sockaddr *addr, std::string &ip, uint16_t &port);

    /**
     * @brief Extracts all request headers from an NGINX HTTP request.
     *
     * Iterates through the `headers_in` list in the given `ngx_http_request_t` and
     * stores the headers as key-value pairs in a `std::unordered_map`.
     *
     * @param request Pointer to the NGINX HTTP request structure (`ngx_http_request_t*`).
     * @return An unordered map containing header names as keys and their corresponding values as strings.
     */
    std::unordered_map<std::string, std::string> parse_ngx_request_headers(ngx_http_request_t *request);

    /**
     * @brief Parses a URI string and extracts query parameters into a map.
     *
     * This function scans the portion of the URI after the '?' character and splits
     * the query string into key-value pairs. Parameters without a value will have an empty string.
     *
     * @param uri The full URI string, potentially containing query parameters.
     * @return A map containing query parameter names and their associated values.
     */
    std::unordered_map<std::string, std::string> parse_query_parameters(const std::string &uri);
}

#endif