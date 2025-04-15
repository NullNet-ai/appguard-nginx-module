#ifndef __APPGUARD_NGINX_MODULE_HPP__
#define __APPGUARD_NGINX_MODULE_HPP__

extern "C"
{
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <string>

class AppGuardNginxModule
{
public:
    struct Config
    {
        ngx_flag_t enabled = NGX_CONF_UNSET;
        ngx_str_t server_addr = ngx_null_string;
    };

    static ngx_int_t Initialize(ngx_conf_t *cf);
    static void *CreateSrvConfig(ngx_conf_t *cf);
    static char *MergeSrvConfig(ngx_conf_t *cf, void *parent, void *child);
    static ngx_int_t Handler(ngx_http_request_t *request);
};

#endif