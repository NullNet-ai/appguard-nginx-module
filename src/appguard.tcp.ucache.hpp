#pragma once

extern "C"
{
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <list>
#include <unordered_map>
#include <optional>

#include "generated/appguard.pb.h"

/**
 * @brief A fixed-size LRU (Least Recently Used) cache for storing AppGuard IP info.
 *
 * This cache uses connection-level data from ngx_connection_t (source/destination IP and port)
 * to store and retrieve information about IP addresses in association with AppGuard metadata.
 */
class AppguardTcpInfoCache
{
public:
    /// The value stored in the cache — IP metadata from AppGuard.
    using Value = appguard::AppGuardTcpInfo;
    /// The key type — a stringified connection descriptor (e.g., "1.2.3.4:1234->5.6.7.8:80").
    using Key = std::string;

    /**
     * @brief Returns the singleton instance of the cache.
     *
     * @return Reference to the singleton AppguardTcpInfoCache.
     */
    static AppguardTcpInfoCache &Instance();

    /**
     * @brief Inserts or updates an entry in the cache.
     *
     * If the entry already exists, it is updated and moved to the front of the usage list.
     * If the cache exceeds its maximum size, the least recently used entry is evicted.
     *
     * @param conn The NGINX connection object used to generate the key.
     * @param value The AppGuardIpInfo value to store.
     */
    void Put(ngx_connection_t *conn, const Value &value);

    /**
     * @brief Retrieves an entry from the cache if it exists.
     *
     * Accessing an entry moves it to the front of the usage list.
     *
     * @param conn The NGINX connection object used to generate the key.
     * @return An optional containing the value if present, or std::nullopt.
     */
    std::optional<Value> Get(ngx_connection_t *conn);

private:
    /// Private constructor to enforce singleton pattern.
    AppguardTcpInfoCache() = default;

private:
    /// Mutex to protect access to the cache in multithreaded environments.
    std::mutex mutex;
    /// List of key-value pairs ordered by usage (front = most recently used).
    std::list<std::pair<Key, Value>> usage;
    /// Hash map to quickly access list nodes by key.
    std::unordered_map<Key, decltype(usage.begin())> map;
};
