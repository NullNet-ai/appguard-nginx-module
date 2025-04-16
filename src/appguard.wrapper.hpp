#ifndef __APPGUARD_CLIENT_HPP__
#define __APPGUARD_CLIENT_HPP__

#include "appguard.pb.h"
#include "appguard.grpc.pb.h"
#include "appguard.client.info.hpp"
#include "appguard.stream.hpp"

/**
 * @brief Wrapper class for interacting with the AppGuard service.
 *
 * This class provides methods to create a client instance and handle various types of connections
 * and requests with the AppGuard service.
 */
class AppGuardWrapper
{
public:
    /**
     * @brief Creates and initializes an AppGuard client instance.
     *
     * @param client_info Configuration information required to establish the client.
     * @param deadline Optional timeout duration for establishing the connection.
     * @return An initialized AppGuardWrapper instance.
     */
    static AppGuardWrapper CreateClient(
        AppGaurdClientInfo client_info,
        const std::chrono::milliseconds &deadline = std::chrono::milliseconds(5'000));

    /**
     * @brief Destructor.
     */
    ~AppGuardWrapper() = default;

    /**
     * @brief Copy constructor.
     */
    AppGuardWrapper(AppGuardWrapper &) = default;

    /**
     * @brief Move constructor.
     */
    AppGuardWrapper(AppGuardWrapper &&) = default;

    /**
     * @brief Handles a TCP connection request.
     *
     * @param connection The TCP connection details to be handled.
     * @return The response from the AppGuard service.
     */
    [[nodiscard]] appguard::AppGuardTcpResponse
    HandleTcpConnection(appguard::AppGuardTcpConnection connection);

    /**
     * @brief Handles an HTTP request.
     *
     * @param request The HTTP request details to be handled.
     * @return The response from the AppGuard service.
     */
    [[nodiscard]] appguard::AppGuardResponse
    HandleHttpRequest(appguard::AppGuardHttpRequest request);

    /**
     * @brief Handles an HTTP response.
     *
     * @param request The HTTP response details to be handled.
     * @return The response from the AppGuard service.
     */
    [[nodiscard]] appguard::AppGuardResponse
    HandleHttpResponse(appguard::AppGuardHttpResponse request);

private:
    /**
     * @brief Private constructor used by CreateClient to initialize the wrapper.
     *
     * @param channel The gRPC channel to communicate with the AppGuard service.
     * @param app_id The application ID for authentication.
     * @param app_secret The application secret for authentication.
     */
    AppGuardWrapper(std::shared_ptr<grpc::Channel> channel, const std::string &app_id, const std::string &app_secret);

    /**
     * @brief Validates the current status of the assosiated device.
     *
     * Throws an exception if the stream is not in a valid state.
     */
    void ValidateStatus() const;

    /**
     * @brief Retrieves the current authentication token.
     *
     * @return The current token as a string.
     */
    std::string AcquireToken() const;

    // gRPC channel for communication.
    std::shared_ptr<grpc::Channel> channel;
    // Stream handling continuous communication with AppGuard.
    std::shared_ptr<AppGuardStream> stream;
};

#endif