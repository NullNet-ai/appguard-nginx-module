#ifndef __APPGUARD_STREAM_HPP__
#define __APPGUARD_STREAM_HPP__

#include "appguard.pb.h"
#include "appguard.grpc.pb.h"

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
     * @param app_id Application identifier.
     * @param app_secret Application secret key.
     */
    AppGuardStream(
        std::shared_ptr<grpc::Channel> channel,
        const std::string &app_id,
        const std::string &app_secret);

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
     * @brief Checks if the device status is ACTIVE.
     * @return True if device status is ACTIVE; otherwise, false.
     */
    inline auto IsDeviceStatusActive() const noexcept
    {
        return this->device_status.load() == appguard::DeviceStatus::ACTIVE;
    }

    /**
     * @brief Checks if the device status is DRAFT.
     * @return True if device status is DRAFT; otherwise, false.
     */
    inline auto IsDeviceStatusDraft() const noexcept
    {
        return this->device_status.load() == appguard::DeviceStatus::DRAFT;
    }

    /**
     * @brief Checks if the device status is ARCHIVED.
     * @return True if device status is ARCHIVED; otherwise, false.
     */
    inline auto IsDeviceStatusAchived() const noexcept
    {
        return this->device_status.load() == appguard::DeviceStatus::ARCHIVED;
    }

    /**
     * @brief Checks if the device status is UNKNOWN.
     * @return True if device status is UNKNOWN; otherwise, false.
     */
    inline auto IsDeviceStatusUnknown() const noexcept
    {
        return this->device_status.load() == appguard::DeviceStatus::DS_UNKNOWN;
    }

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

    /**
     * @brief Sets the device status atomically.
     * @param status The new device status to set.
     */
    inline void SetDeviceStatus(appguard::DeviceStatus status) noexcept
    {
        this->device_status.store(status);
    }

private:
    // Application identifier.
    std::string app_id;
    // Application secret key.
    std::string app_secret;
    // Authentication token received from the server.
    std::string token;
    // Mutex to protect access to the token.
    mutable std::mutex token_mutex;
    // Condition variable to wait for token availability.
    std::condition_variable token_cv;
    // Current device status.
    std::atomic<appguard::DeviceStatus> device_status;
    // Thread handling the heartbeat stream.
    std::thread thread;
    // Flag indicating if the stream is running.
    std::atomic_bool running;
    // gRPC channel to the server.
    std::shared_ptr<grpc::Channel> channel;
    // Mutex to protect access to the gRPC context.
    std::mutex context_mutex;
    // gRPC client context for the stream.
    std::unique_ptr<grpc::ClientContext> context;
};

#endif