#ifndef __APPGUARD_UCLIENT_INFO_HPP__
#define __APPGUARD_UCLIENT_INFO_HPP__

#include <string>

/**
 * @brief Holds the configuration details required to connect to the AppGuard service.
 */
struct AppGaurdClientInfo
{
    // Unique identifier for the application.
    std::string app_id;
    // Secret key used for authenticating the application.
    std::string app_secret;
    // Address of the AppGuard server (e.g., "localhost:50051").
    std::string server_addr;
    // Indicates whether to use TLS for the connection.
    bool tls;

    /**
     * @brief Default constructor.
     */
    AppGaurdClientInfo() = default;

    /**
     * @brief Copy constructor.
     */
    AppGaurdClientInfo(AppGaurdClientInfo &) = default;

    /**
     * @brief Move constructor.
     */
    AppGaurdClientInfo(AppGaurdClientInfo &&) = default;

    /**
     * @brief Destructor.
     */
    ~AppGaurdClientInfo() noexcept = default;

    /**
     * @brief Equality operator.
     *
     * Compares two AppGuardClientInfo instances for equality.
     *
     * @param other The other AppGuardClientInfo instance to compare with.
     * @return true if all members are equal; false otherwise.
     */
    bool operator==(const AppGaurdClientInfo &other) const;

    /**
     * @brief Less-than operator.
     *
     * Compares two AppGuardClientInfo instances to determine ordering.
     *
     * @param other The other AppGuardClientInfo instance to compare with.
     * @return true if this instance is considered less than the other; false otherwise.
     */
    bool operator<(const AppGaurdClientInfo &other) const;
};

#endif