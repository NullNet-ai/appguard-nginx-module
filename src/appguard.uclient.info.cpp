#include "appguard.uclient.info.hpp"

#include <tuple>

bool AppGaurdClientInfo::operator==(const AppGaurdClientInfo &other) const
{
    return std::tie(this->app_id, this->app_secret, this->server_addr, this->tls) ==
           std::tie(other.app_id, other.app_secret, other.server_addr, other.tls);
}

bool AppGaurdClientInfo::operator<(const AppGaurdClientInfo &other) const
{
    return std::tie(this->app_id, this->app_secret, this->server_addr, this->tls) <
           std::tie(other.app_id, other.app_secret, other.server_addr, other.tls);
}