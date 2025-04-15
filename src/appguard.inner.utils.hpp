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
    appguard::AppGuardTcpConnection ExtractTcpConnectionInfo(ngx_http_request_t *request);

    appguard::AppGuardHttpRequest ExtractHttpRequestInfo(ngx_http_request_t *request);
}

#endif