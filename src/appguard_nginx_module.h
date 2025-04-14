#ifndef __APPGUARD_NGINX_MODULE_H__
#define __APPGUARD_NGINX_MODULE_H__

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Configuration structure for the AppGuard NGINX module (per-server level).
     *
     * This structure defines settings that can be configured in the NGINX configuration file
     * within the `server` block, controlling the behavior of the AppGuard integration.
     */
    typedef struct
    {
        ngx_flag_t enabled;
        ngx_flag_t report_body;
        ngx_str_t server_addr;
    } ngx_http_appguard_srv_conf_t;

#ifdef __cplusplus
}
#endif

#endif