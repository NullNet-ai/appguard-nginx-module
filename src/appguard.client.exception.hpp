#ifndef __APPGUARD_CLIENT_EXCEPTION_HPP__
#define __APPGUARD_CLIENT_EXCEPTION_HPP__

#include <stdexcept>
#include <string>

#include <grpcpp/support/status_code_enum.h>

/**
 * @brief Exception type for AppGuard client errors.
 *
 * Thrown when an operation in the AppGuard client fails due to gRPC errors,
 * initialization issues, or protocol violations.
 */
class AppGuardClientException : public std::runtime_error
{
public:
    /**
     * @brief Constructs a new AppGuardClientException with a gRPC status code and message.
     *
     * @param code    The gRPC status code representing the type of failure.
     * @param message A human-readable message describing the failure.
     */
    AppGuardClientException(grpc::StatusCode code, const std::string &message)
        : std::runtime_error(message), status_code(code) {}

    /**
     * @brief Returns the gRPC status code associated with this exception.
     *
     * @return `grpc::StatusCode`
     */
    inline grpc::StatusCode code() const noexcept
    {
        return this->status_code;
    }

private:
    grpc::StatusCode status_code;
};

#endif