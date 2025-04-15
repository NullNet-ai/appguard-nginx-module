#ifndef __APPGUARD_NGINX_MODULE_HPP__
#define __APPGUARD_NGINX_MODULE_HPP__

extern "C"
{
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <string>

/**
 * @brief NGINX module integration for AppGuard.
 *
 * Provides initialization and request handling logic for integrating AppGuard
 * gRPC-based decision-making into NGINX request lifecycle.
 */
class AppGuardNginxModule
{
public:
    /**
     * @brief Configuration structure for the AppGuard NGINX module.
     */
    struct Config
    {
        ngx_flag_t enabled = NGX_CONF_UNSET;
        ngx_str_t server_addr = ngx_null_string;
    };

    /**
     * @brief Initializes the AppGuard module during NGINX configuration phase.
     *
     * Registers HTTP phase handlers and module-specific settings.
     *
     * @param cf NGINX configuration context.
     * @return `NGX_OK` on success or `NGX_ERROR` on failure.
     */
    static ngx_int_t Initialize(ngx_conf_t *cf);

    /**
     * @brief Creates a new server-level configuration.
     *
     * Called by NGINX to allocate configuration memory for a server block.
     *
     * @param cf NGINX configuration context.
     * @return A pointer to the new configuration structure.
     */
    static void *CreateSrvConfig(ngx_conf_t *cf);

    /**
     * @brief Merges parent and child server-level configurations.
     *
     * Ensures that the child inherits unspecified values from the parent.
     *
     * @param cf     NGINX configuration context.
     * @param parent Pointer to the parent configuration.
     * @param child  Pointer to the child configuration.
     * @return `NGX_CONF_OK` on success, or a pointer to an error string on failure.
     */
    static char *MergeSrvConfig(ngx_conf_t *cf, void *parent, void *child);

    /**
     * @brief Main request handler for the AppGuard module.
     *
     * Invoked during request processing to communicate with the AppGuard server
     * and determine the response policy.
     *
     * @param request The current NGINX HTTP request.
     * @return An appropriate NGINX status code.
     */
    static ngx_int_t Handler(ngx_http_request_t *request);
};

#endif