#include "appguard.storage.hpp"
#include "appguard.uclient.exception.hpp"

Storage &Storage::GetInstance()
{
    static Storage instance;
    return instance;
}

void Storage::Initialize()
{
    static std::once_flag initialized;
    std::call_once(initialized, [&]()
                   { GetInstance(); });
}

void Storage::Set(const std::string &key, const std::string &value)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->data[key] = value;
}

std::optional<std::string> Storage::Get(const std::string &key) const
{
    std::lock_guard<std::mutex> lock(this->mutex);
    if (auto iter = this->data.find(key); iter != this->data.end())
    {
        return {iter->second};
    }
    else
    {
        return {};
    }
}

void Storage::Clear()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->data.clear();
}