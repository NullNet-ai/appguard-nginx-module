#ifndef __APPGUARD_NGINX_MODULE_H__
#define __APPGUARD_NGINX_MODULE_H__

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct
{
    ngx_flag_t enabled;
    ngx_flag_t report_body;
    ngx_str_t server_addr;
} ngx_http_appguard_srv_conf_t;

#endif