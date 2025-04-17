#include "appguard.tcp.ucache.hpp"
#include <sstream>

static size_t CACHE_MAX_SIZE = 1024;

static AppguardTcpInfoCache::Key MakeKey(ngx_connection_t *conn)
{
    std::stringstream ss;

    sockaddr_in *src = reinterpret_cast<sockaddr_in *>(conn->sockaddr);
    sockaddr_in *dst = reinterpret_cast<sockaddr_in *>(conn->local_sockaddr);

    char src_ip[INET_ADDRSTRLEN] = {0};
    char dst_ip[INET_ADDRSTRLEN] = {0};

    inet_ntop(AF_INET, &(src->sin_addr), src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(dst->sin_addr), dst_ip, INET_ADDRSTRLEN);

    ss << src_ip << ":" << ntohs(src->sin_port)
       << "->" << dst_ip << ":" << ntohs(dst->sin_port);

    return ss.str();
}

AppguardTcpInfoCache &AppguardTcpInfoCache::Instance()
{
    static AppguardTcpInfoCache instance;
    return instance;
}

void AppguardTcpInfoCache::Put(ngx_connection_t *connection, const AppguardTcpInfoCache::Value &value)
{
    std::lock_guard<std::mutex> lock(this->mutex);

    AppguardTcpInfoCache::Key key = MakeKey(connection);

    auto iterator = this->map.find(key);
    if (iterator != this->map.end())
    {
        iterator->second->second = value;
        this->usage.splice(
            this->usage.begin(),
            this->usage,
            iterator->second);
    }

    this->usage.emplace_front(key, value);
    this->map[key] = this->usage.begin();

    if (this->map.size() > CACHE_MAX_SIZE)
    {
        auto last = this->usage.end();
        --last;

        this->map.erase(last->first);
        this->usage.pop_back();
    }
}

std::optional<AppguardTcpInfoCache::Value> AppguardTcpInfoCache::Get(ngx_connection_t *connection)
{
    std::lock_guard<std::mutex> lock(this->mutex);

    AppguardTcpInfoCache::Key key = MakeKey(connection);

    auto iterator = this->map.find(key);
    if (iterator == this->map.end())
        return std::nullopt;

    this->usage.splice(
        this->usage.begin(),
        this->usage,
        iterator->second);

    return iterator->second->second;
}