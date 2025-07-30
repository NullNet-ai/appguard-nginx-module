#include "appguard.uclient.info.hpp"

#include <tuple>

bool AppGaurdClientInfo::operator==(const AppGaurdClientInfo &other) const
{
    return std::tie(this->installation_code, this->server_addr, this->tls) ==
           std::tie(other.installation_code, other.server_addr, other.tls);
}

bool AppGaurdClientInfo::operator<(const AppGaurdClientInfo &other) const
{
    return std::tie(this->installation_code, this->server_addr, this->tls) <
           std::tie(other.installation_code, other.server_addr, other.tls);
}