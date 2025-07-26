#include "appguard.storage.hpp"
#include "appguard.uclient.exception.hpp"

#define STORAGE_FILE_PATH "/var/run/nullnet/nginx.conf"

static std::string EscapeString(const std::string &str)
{
    std::string escaped;
    escaped.reserve(str.length() * 2);

    for (char c : str)
    {
        switch (c)
        {
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '=':
            escaped += "\\=";
            break;
        default:
            escaped += c;
            break;
        }
    }
    return escaped;
}

static std::string UnescapeString(const std::string &str)
{
    std::string unescaped;
    unescaped.reserve(str.length());

    for (size_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == '\\' && i + 1 < str.length())
        {
            switch (str[i + 1])
            {
            case 'n':
                unescaped += '\n';
                ++i;
                break;
            case 'r':
                unescaped += '\r';
                ++i;
                break;
            case 't':
                unescaped += '\t';
                ++i;
                break;
            case '\\':
                unescaped += '\\';
                ++i;
                break;
            case '=':
                unescaped += '=';
                ++i;
                break;
            default:
                unescaped += str[i];
                break;
            }
        }
        else
        {
            unescaped += str[i];
        }
    }
    return unescaped;
}

PersistentStorage::PersistentStorage(const std::string &filename) : filename(filename)
{
    LoadInternal();
}

PersistentStorage &PersistentStorage::GetInstance()
{
    static PersistentStorage instance(STORAGE_FILE_PATH);
    return instance;
}

void PersistentStorage::Initialize()
{
    static std::once_flag initialized;
    std::call_once(initialized, [&]()
                   { GetInstance(); });
}

void PersistentStorage::SaveInternal() const
{
    std::filesystem::path file_path(this->filename);
    if (file_path.has_parent_path())
    {
        std::filesystem::create_directories(file_path.parent_path());
    }

    std::ofstream file(this->filename);
    THROW_IF_CUSTOM(!file.is_open(), AppGuardStatusCode::APPGUARD_STORAGE_OPERATION_FAILURE);

    for (const auto &pair : this->data)
    {
        file << EscapeString(pair.first) << "=" << EscapeString(pair.second) << "\n";
    }

    THROW_IF_CUSTOM(file.fail(), AppGuardStatusCode::APPGUARD_STORAGE_OPERATION_FAILURE);
}

void PersistentStorage::LoadInternal()
{
    this->data.clear();

    std::ifstream file(this->filename);
    if (!file.is_open())
    {
        // File doesn't exist yet, that's okay
        return;
    }

    std::string line;
    size_t line_number = 0;

    while (std::getline(file, line))
    {
        ++line_number;

        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        size_t pos = 0;
        while (pos < line.length())
        {
            if (line[pos] == '=' && (pos == 0 || line[pos - 1] != '\\'))
            {
                break;
            }
            ++pos;
        }

        THROW_IF_CUSTOM(pos == line.length(), AppGuardStatusCode::APPGUARD_STORAGE_OPERATION_FAILURE);

        std::string key = UnescapeString(line.substr(0, pos));
        std::string value = UnescapeString(line.substr(pos + 1));

        this->data[key] = value;
    }
}

void PersistentStorage::Set(const std::string &key, const std::string &value)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->data[key] = value;
    SaveInternal();
}

std::optional<std::string> PersistentStorage::Get(const std::string &key) const
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

bool PersistentStorage::Exists(const std::string &key) const
{
    std::lock_guard<std::mutex> lock(this->mutex);
    return this->data.find(key) != this->data.end();
}

void PersistentStorage::Clear()
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->data.clear();
    SaveInternal();
}