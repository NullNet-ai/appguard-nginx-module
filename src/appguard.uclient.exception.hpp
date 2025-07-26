#pragma once

#include <stdexcept>
#include <string>
#include <grpcpp/grpcpp.h>

/**
 * @brief Custom status codes used by AppGuard to represent specific client-side errors.\
 */
enum class AppGuardStatusCode
{
    // The authentication stream is not active.
    APPGUARD_AUTH_STREAM_NOT_RUNNING,
    // Failed to acquire an authentication token.
    APPGUARD_FAILED_TO_ACQUIRE_TOKEN,
    // Failed to save client.
    APPGUARD_FAILED_TO_SAVE_CLIENT,
    // Connection attempt timed out.
    APPGUARD_CONNECTION_TIMEOUT,
    // Failed to load the server's certificate
    APPGUARD_CERTIFICATE_NOT_FOUND,
    // Failed to obtain device UUID
    APPGUARD_FAILED_TO_OBTAIN_UUID,
    // Storage operation error
    APPGUARD_STORAGE_OPERATION_FAILURE
};

/**
 * @brief Exception type representing errors specific to the AppGuard client.
 */
class AppGuardClientException : public std::runtime_error
{
public:
    /**
     * @brief Constructs a new AppGuardClientException with a custom message.
     * @param message The exception message.
     */
    AppGuardClientException(const std::string &message);

    /**
     * @brief Destructor override.
     */
    ~AppGuardClientException() noexcept override = default;

    /**
     * @brief Creates an exception based on a gRPC status object.
     * @param status The gRPC status returned from a remote procedure call.
     * @return An AppGuardClientException with information extracted from the gRPC status.
     */
    static AppGuardClientException FromGrpcStatus(const grpc::Status &status);

    /**
     * @brief Creates an exception based on an AppGuard-specific status code.
     * @param code The custom AppGuard error code.
     * @return An AppGuardClientException corresponding to the provided code.
     */
    static AppGuardClientException FromCustomCode(AppGuardStatusCode code);
};

/**
 * @brief Throws an AppGuardClientException if the condition evaluates to true.
 * @param cond The condition to evaluate.
 * @param code The AppGuardStatusCode to include in the exception.
 */
#define THROW_IF_CUSTOM(cond, code)                              \
    do                                                           \
    {                                                            \
        if (cond)                                                \
        {                                                        \
            throw AppGuardClientException::FromCustomCode(code); \
        }                                                        \
    } while (0)
/**
 * @brief Throws an AppGuardClientException if the gRPC status is not OK.
 * @param status The gRPC status object to evaluate.
 */
#define THROW_IF_GRPC(status)                                      \
    do                                                             \
    {                                                              \
        if (!(status).ok())                                        \
        {                                                          \
            throw AppGuardClientException::FromGrpcStatus(status); \
        }                                                          \
    } while (0)

/**
 * @brief Executes a statement while silently ignoring any thrown exceptions
 * @param statement The statement or expression to execute
 */
#define IGNORE_ALL_EXCEPTIONS(statement) \
    do                                   \
    {                                    \
        try                              \
        {                                \
            (statement);                 \
        }                                \
        catch (...)                      \
        {                                \
        }                                \
    } while (0)
