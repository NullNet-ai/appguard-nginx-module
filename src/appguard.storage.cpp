#include "appguard.storage.hpp"
#include "appguard.uclient.exception.hpp"

#include <fstream>
#include <sstream>
#include <filesystem>

extern "C"
{
#include <ngx_core.h>
}

#define MUTEX_NAME "/appguard-nginx-cfg"
#define CONFIG_PATH "/var/cache/nginx/appguard.conf"

Storage::Storage() : data(), mutex(MUTEX_NAME)
{
}

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
    std::lock_guard lock(this->mutex);
    this->data[key] = value;
    SaveToFile();
}

std::optional<std::string> Storage::Get(const std::string &key) const
{
    std::lock_guard lock(this->mutex);
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
    std::lock_guard lock(this->mutex);
    this->data.clear();
    SaveToFile();
}

void Storage::Update()
{
    std::lock_guard lock(this->mutex);
    LoadFromFile();
}

void Storage::LoadFromFile()
{
    std::fstream file(CONFIG_PATH);

    if (!file.is_open())
    {
        using namespace std::string_literals;
        std::string msg = "AppGuard: Faild to open configuration file "s + CONFIG_PATH;

        ngx_log_error(
            NGX_LOG_ERR,
            ngx_cycle->log,
            0,
            msg.data());

        return;
    }

    this->data.clear();
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        size_t delimiter_pos = line.find('=');
        if (delimiter_pos == std::string::npos)
            continue;

        std::string key = line.substr(0, delimiter_pos);
        std::string value = line.substr(delimiter_pos + 1);

        size_t pos = 0;
        while ((pos = value.find("\\n", pos)) != std::string::npos)
        {
            value.replace(pos, 2, "\n");
            pos += 1;
        }
        pos = 0;
        while ((pos = value.find("\\=", pos)) != std::string::npos)
        {
            value.replace(pos, 2, "=");
            pos += 1;
        }
        pos = 0;
        while ((pos = value.find("\\\\", pos)) != std::string::npos)
        {
            value.replace(pos, 2, "\\");
            pos += 1;
        }

        this->data[key] = value;
    }
}

void Storage::SaveToFile() const
{
    std::fstream file(CONFIG_PATH);

    if (!file.is_open())
    {
        using namespace std::string_literals;
        std::string msg = "AppGuard: Faild to open configuration file "s + CONFIG_PATH;

        ngx_log_error(
            NGX_LOG_ERR,
            ngx_cycle->log,
            0,
            msg.data());

        return;
    }

    for (const auto &pair : this->data)
    {
        std::string escaped_value = pair.second;

        size_t pos = 0;
        while ((pos = escaped_value.find("\\", pos)) != std::string::npos)
        {
            escaped_value.replace(pos, 1, "\\\\");
            pos += 2;
        }
        pos = 0;
        while ((pos = escaped_value.find("\n", pos)) != std::string::npos)
        {
            escaped_value.replace(pos, 1, "\\n");
            pos += 2;
        }
        pos = 0;
        while ((pos = escaped_value.find("=", pos)) != std::string::npos)
        {
            escaped_value.replace(pos, 1, "\\=");
            pos += 2;
        }

        file << pair.first << "=" << escaped_value << "\n";
    }
}