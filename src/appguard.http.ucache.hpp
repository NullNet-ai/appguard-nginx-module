#pragma once

#include <mutex>
#include <map>
#include <optional>
#include "generated/commands.pb.h"
#include "generated/appguard.pb.h"

template <typename T>
class AppguardHttpCache
{
public:
    using value_type = appguard_commands::FirewallPolicy;

    static AppguardHttpCache<T> &GetInstance() noexcept
    {
        static AppguardHttpCache<T> instance;
        return instance;
    }

    void Put(T &&key, value_type value)
    {
        std::lock_guard lock(this->mutex);
        if (!this->enabled)
            return;

        this->cache.insert({std::move(key), std::move(value)});
    }

    std::optional<value_type> Get(const T &key) const
    {
        std::lock_guard lock(this->mutex);
        if (!this->enabled)
            return std::nullopt;

        auto iterator = this->cache.find(key);
        return iterator == this->cache.end() ? std::nullopt : std::make_optional(iterator->second);
    }

    void Clear()
    {
        std::lock_guard lock(this->mutex);
        if (!this->enabled)
            return;

        this->cache.clear();
    }

    void Enable(bool enable)
    {
        std::lock_guard lock(this->mutex);
        this->enabled = enable;
    }

    bool IsEnabled() const
    {
        std::lock_guard lock(this->mutex);
        return this->enabled;
    }

private:
    AppguardHttpCache() = default;

private:
    mutable std::mutex mutex{};
    bool enabled{false};
    std::map<T, value_type> cache{};
};

struct HttpRequestCacheKey
{
public:
    static HttpRequestCacheKey FromRequest(const appguard::AppGuardHttpRequest &request);
    bool operator<(const HttpRequestCacheKey &other) const noexcept;

    HttpRequestCacheKey() = default;
    HttpRequestCacheKey(const HttpRequestCacheKey &) = default;
    HttpRequestCacheKey(HttpRequestCacheKey &&) = default;

private:
    std::string url;
    std::string method;
    std::string query;
    std::string userAgent;
    std::string sourceIp;

    friend struct HttpRequestCacheKeyHash;
};
