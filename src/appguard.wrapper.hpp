#ifndef __APPGUARD_CLIENT_HPP__
#define __APPGUARD_CLIENT_HPP__

#include "appguard.pb.h"
#include "appguard.grpc.pb.h"

/**
 * @brief Client wrapper for communicating with the AppGuard gRPC server.
 *
 * This class provides high-level methods for handling TCP and HTTP interactions
 * with the AppGuard service, abstracting the underlying gRPC implementation.
 */
class AppGuardWrapper
{
public:
    using Stub = appguard::AppGuard::Stub;

    ~AppGuardWrapper() = default;

    /**
     * @brief Factory method to create a new AppGuardWrapper instance.
     *
     * This method creates a gRPC channel and initializes the client stub
     * to communicate with the specified AppGuard server address.
     *
     * @param server_addr The address of the AppGuard gRPC server (e.g., "localhost:50051").
     * @return An initialized `AppGuardWrapper` instance.
     * @throws `AppGuardClientException` if the gRPC request fails.
     */
    static AppGuardWrapper create(const std::string &server_addr);

    /**
     * @brief Handles a TCP connection event by querying the AppGuard server.
     *
     * Sends information about an incoming TCP connection and receives a structured response.
     *
     * @param connection The TCP connection request data.
     * @return The response from the AppGuard server.
     * @throws `AppGuardClientException` if the gRPC request fails.
     */
    [[nodiscard]] appguard::AppGuardTcpResponse
    HandleTcpConnection(appguard::AppGuardTcpConnection connection);

    /**
     * @brief Handles an HTTP request by querying the AppGuard server.
     *
     * Sends relevant HTTP request metadata and receives a response that may indicate
     * whether to allow or deny the request.
     *
     * @param request The HTTP request data.
     * @return The response from the AppGuard server.
     * @throws `AppGuardClientException` if the gRPC request fails.
     */
    [[nodiscard]] appguard::AppGuardResponse
    HandleHttpRequest(appguard::AppGuardHttpRequest request);

    /**
     * @brief Handles an HTTP response event by sending metadata to AppGuard.
     *
     * This may be used for logging, auditing, or post-response validation by AppGuard.
     *
     * @param request The HTTP response data.
     * @return The response from the AppGuard server.
     * @throws `AppGuardClientException` if the gRPC request fails.
     */
    [[nodiscard]] appguard::AppGuardResponse
    HandleHttpResponse(appguard::AppGuardHttpResponse request);

private:
    AppGuardWrapper(std::unique_ptr<Stub> stub);

    std::unique_ptr<Stub> stub;
};

#endif