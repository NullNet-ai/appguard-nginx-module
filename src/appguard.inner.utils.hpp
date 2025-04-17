#ifndef __APPGUARD_INNER_UTILS_H__
#define __APPGUARD_INNER_UTILS_H__

#include <sys/socket.h>
#include <string>
#include <unordered_map>

extern "C"
{
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include "appguard.pb.h"

namespace appguard::inner_utils
{
    /**
     * @brief Converts an Nginx ngx_str_t to a C++ std::string.
     *
     * This function safely converts an Nginx string (ngx_str_t) to a standard C++ string.
     * It checks for null pointers and ensures that the data is valid before performing the conversion.
     *
     * @param str Pointer to the ngx_str_t structure to convert.
     * @return A std::string containing the same data as the input ngx_str_t.
     *         Returns an empty string if the input is null or invalid.
     */
    std::string NgxStringToStdString(ngx_str_t *str);
    /**
     * @brief Extracts TCP connection information from an NGINX HTTP request.
     *
     * Populates an `AppGuardTcpConnection` object with source IP and port,
     * destination IP and port, and the HTTP protocol version used by the client.
     *
     * @param request Pointer to the NGINX HTTP request.
     * @return A populated `AppGuardTcpConnection` object.
     */
    appguard::AppGuardTcpConnection ExtractTcpConnectionInfo(ngx_http_request_t *request);

    /**
     * @brief Extracts HTTP request information from an NGINX request.
     *
     * Converts URI, headers, method, and query parameters into an `AppGuardHttpRequest`.
     *
     * @param request Pointer to the NGINX HTTP request.
     * @return A populated `AppGuardHttpRequest` object.
     */
    appguard::AppGuardHttpRequest ExtractHttpRequestInfo(ngx_http_request_t *request);

    appguard::AppGuardHttpResponse ExtractHttpResponseInfo(ngx_http_request_t *request);

    /**
     * @brief Converts a string to the corresponding FirewallPolicy enum value.
     *
     * This function performs a case-insensitive comparison of the input string
     * to determine the appropriate FirewallPolicy. If the string matches "allow"
     * (case-insensitive), it returns FirewallPolicy::ALLOW; otherwise, it returns
     * FirewallPolicy::DENY.
     *
     * @param str The input string representing the firewall policy.
     * @return The corresponding FirewallPolicy enum value.
     */
    appguard::FirewallPolicy StringToFirewallPolicy(const std::string &str);
}

#endif