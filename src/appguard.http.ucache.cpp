#include "appguard.http.ucache.hpp"
#include <tuple>
#include <sstream>

static std::string SerializeQuery(const google::protobuf::Map<std::string, std::string> &queryMap)
{
    std::ostringstream oss;
    bool first = true;

    for (const auto &[key, value] : queryMap)
    {
        if (!first)
        {
            oss << "&";
        }

        oss << key << "=" << value;
        first = false;
    }

    return oss.str();
}

static std::string FindUserAgent(const google::protobuf::Map<std::string, std::string> &headersMap)
{
    const auto iterator = headersMap.find("user-agent");
    return iterator != headersMap.cend() ? iterator->second : std::string();
}

HttpRequestCacheKey HttpRequestCacheKey::FromRequest(const appguard::AppGuardHttpRequest &request)
{
    HttpRequestCacheKey instance;

    instance.method = request.method();
    instance.url = request.original_url();
    instance.sourceIp = request.tcp_info().connection().source_ip();

    instance.query = SerializeQuery(request.query());
    instance.userAgent = FindUserAgent(request.headers());

    return instance;
}

bool HttpRequestCacheKey::operator<(const HttpRequestCacheKey &other) const noexcept
{
    return std::tie(this->method, this->query, this->sourceIp, this->url, this->userAgent) ==
           std::tie(other.method, other.query, other.sourceIp, other.url, other.userAgent);
}