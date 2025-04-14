#ifndef __APPGUARD_H__
#define __APPGUARD_H__

#include <stdint.h>
#include <nginx.h>
#include <ngx_http.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Opaque handle for the AppGuard gRPC client.
     *
     * The internal structure is hidden from the user to provide encapsulation
     * and to allow ABI compatibility across versions.
     */
    typedef struct appguard_handle_t appguard_handle_t;

    /**
     * @brief Creates a new AppGuard client handle connected to the given server.
     *
     * This function initializes the internal gRPC stub and connects to the specified
     * server address using insecure credentials.
     *
     * @param server_addr The address of the gRPC server (e.g., `"localhost:50051"`).
     * @return Pointer to a new `appguard_handle_t` instance, or `NULL` on failure.
     */
    appguard_handle_t *new_appguard_handle(const char *server_addr);

    /**
     * @brief Destroys the AppGuard client handle and releases resources.
     *
     * This function should be called when the handle is no longer needed.
     *
     * @param handle Pointer to the handle to destroy. Safe to pass NULL.
     */
    void delete_appguard_handle(appguard_handle_t *handle);

    /**
     * @brief Possible actions returned by the AppGuard server.
     */
    typedef enum
    {
        APPGUARD_UNKNOWN, /** The server could not determine an action. */
        APPGUARD_ALLOW,   /** The request is allowed to proceed. */
        APPGUARD_DENY     /** The request should be denied. */
    } appguard_action_t;

        /**
     * @brief Handles an incoming HTTP request by querying the AppGuard server.
     *
     * This function sends relevant data from the NGINX HTTP request to the AppGuard gRPC
     * server and returns the action to be taken (allow, deny, or unknown).
     *
     * @param handle The AppGuard client handle created with `new_appguard_handle`.
     * @param request Pointer to the NGINX HTTP request to inspect.
     * @return The action recommended by the AppGuard server.
     */
    appguard_action_t handle_http_request(appguard_handle_t *handle, ngx_http_request_t *request);
    
#ifdef __cplusplus
}
#endif

#endif