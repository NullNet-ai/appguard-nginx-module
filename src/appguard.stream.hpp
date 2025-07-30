#pragma once

#include "generated/appguard.pb.h"
#include "generated/appguard.grpc.pb.h"
#include "generated/commands.pb.h"
#include "generated/commands.grpc.pb.h"

#include <thread>
#include <atomic>
#include <condition_variable>
#include <grpcpp/grpcpp.h>

/**
 * @class AppGuardStream
 * @brief Manages a gRPC heartbeat stream to the AppGuard server.
 *
 * This class establishes and maintains a heartbeat stream with the AppGuard server,
 * handling token updates and device status changes. It supports automatic reconnection
 * and thread-safe access to shared resources.
 */
class AppGuardStream
{
public:
    /**
     * @brief Constructs an AppGuardStream and starts the heartbeat stream.
     * @param channel Shared pointer to the gRPC channel.
     * @param installation_code NullNet installation code.
     */
    AppGuardStream(
        std::shared_ptr<grpc::Channel> channel,
        const std::string &installation_code);

    /**
     * @brief Destructor that stops the heartbeat stream.
     */
    ~AppGuardStream();

    /**
     * @brief Checks if the heartbeat stream is currently running.
     * @return True if running; otherwise, false.
     */
    inline auto Running() const noexcept { return this->running.load(); }

    /**
     * @brief Waits for the token to become available until the specified timeout duration.
     * @param timeout The maximum duration to wait for the token.
     * @return The token string if available before the timeout; empty string otherwise.
     */
    std::string WaitForToken(std::chrono::milliseconds timeout = std::chrono::milliseconds(5'000));

private:
    /**
     * @brief Starts the heartbeat stream in a separate thread.
     */
    void Start();

    /**
     * @brief Stops the heartbeat stream and joins the thread.
     */
    void Stop();

    /**
     * @brief Sets the authentication token in a thread-safe manner.
     * @param token The new token string to set.
     */
    void SetToken(const std::string &token);

private:
    // Installation code.
    std::string installation_code;
    // Authentication token received from the server.
    std::string token;
    // Mutex to protect access to the token.
    mutable std::mutex token_mutex;
    // Condition variable to wait for token availability.
    std::condition_variable token_cv;
    // Thread handling the control channel.
    std::thread thread;
    // Flag indicating if the control channel is running.
    std::atomic_bool running;
    // gRPC channel to the server.
    std::shared_ptr<grpc::Channel> channel;
    // Mutex to protect access to the gRPC context.
    std::mutex context_mutex;
    // gRPC client context for the control channel.
    std::unique_ptr<grpc::ClientContext> context;
};
