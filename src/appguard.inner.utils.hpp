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
}

#endif