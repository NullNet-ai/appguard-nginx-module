#include "appguard.uclient.exception.hpp"
#include <sstream>

static inline std::string GrpcStatusCodeToStr(grpc::StatusCode code)
{
    switch (code)
    {
    case grpc::StatusCode::CANCELLED:
        return "CANCELLED";
    case grpc::StatusCode::INVALID_ARGUMENT:
        return "INVALID_ARGUMENT";
    case grpc::StatusCode::DEADLINE_EXCEEDED:
        return "DEADLINE_EXCEEDED";
    case grpc::StatusCode::NOT_FOUND:
        return "NOT_FOUND";
    case grpc::StatusCode::ALREADY_EXISTS:
        return "ALREADY_EXISTS";
    case grpc::StatusCode::PERMISSION_DENIED:
        return "PERMISSION_DENIED";
    case grpc::StatusCode::UNAUTHENTICATED:
        return "UNAUTHENTICATED";
    case grpc::StatusCode::RESOURCE_EXHAUSTED:
        return "RESOURCE_EXHAUSTED";
    case grpc::StatusCode::FAILED_PRECONDITION:
        return "FAILED_PRECONDITION";
    case grpc::StatusCode::OUT_OF_RANGE:
        return "OUT_OF_RANGE";
    case grpc::StatusCode::UNIMPLEMENTED:
        return "UNIMPLEMENTED";
    case grpc::StatusCode::INTERNAL:
        return "INTERNAL";
    case grpc::StatusCode::UNAVAILABLE:
        return "UNAVAILABLE";
    case grpc::StatusCode::DATA_LOSS:
        return "DATA_LOSS";
    default:
        return "UNKNOWN";
    }
}

static inline std::string AppGuardStatusCodeToStr(AppGuardStatusCode code)
{
    switch (code)
    {
    case AppGuardStatusCode::APPGUARD_AUTH_STREAM_NOT_RUNNING:
        return "APPGUARD_AUTH_STREAM_NOT_RUNNING";
    case AppGuardStatusCode::APPGUARD_CONNECTION_TIMEOUT:
        return "APPGUARD_CONNECTION_TIMEOUT";
    case AppGuardStatusCode::APPGUARD_FAILED_TO_ACQUIRE_TOKEN:
        return "APPGUARD_FAILED_TO_ACQUIRE_TOKEN";
    case AppGuardStatusCode::APPGUARD_FAILED_TO_SAVE_CLIENT:
        return "APPGUARD_FAILED_TO_SAVE_CLIENT";
    case AppGuardStatusCode::APPGUARD_CERTIFICATE_NOT_FOUND:
        return "APPGUARD_CERTIFICATE_NOT_FOUND";
    case AppGuardStatusCode::APPGUARD_FAILED_TO_OBTAIN_UUID:
        return "APPGUARD_FAILED_TO_OBTAIN_UUID";
    case AppGuardStatusCode::APPGUARD_STORAGE_OPERATION_FAILURE:
        return "APPGUARD_STORAGE_OPERATION_FAILURE";
    default:
        return "UNKNOWN";
    }
}

AppGuardClientException::AppGuardClientException(const std::string &message)
    : std::runtime_error(message)
{
}

AppGuardClientException AppGuardClientException::FromGrpcStatus(const grpc::Status &status)
{
    std::stringstream ss;

    ss << "gRPC error code " << GrpcStatusCodeToStr(status.error_code());

    return AppGuardClientException(ss.str());
}

AppGuardClientException AppGuardClientException::FromCustomCode(AppGuardStatusCode code)
{
    std::stringstream ss;

    ss << "AppGuardStatusCode status code " << AppGuardStatusCodeToStr(code);

    return AppGuardClientException(ss.str());
}