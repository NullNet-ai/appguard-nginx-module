#pragma once

#include "appguard.ipc.mutex.hpp"

#include <string>
#include <unordered_map>
#include <optional>

/**
 * @brief Thread-safe singleton for key-value storage.
 */
class Storage
{
public:
    /**
     * @brief Gets the singleton instance.
     * @return Reference to the singleton instance.
     */
    static Storage &GetInstance();
    /**
     * @brief Initializes the singleton with default configuration.
     * Should be called before first GetInstance() call.
     */
    static void Initialize();
    /**
     * @brief Stores a key-value pair and saves to disk.
     * @param key The key to store.
     * @param value The value to associate with the key.
     */
    void Set(const std::string &key, const std::string &value);
    /**
     * @brief Retrieves a value by key.
     * @param key The key to search for.
     * @return Optional containing the value if found, nullopt otherwise.
     */
    std::optional<std::string> Get(const std::string &key) const;

    /**
     * @brief Removes all data from storage.
     */
    void Clear();

    /**
     * @brief Syncronize with the file contents.
     */
    void Update();
private:
    /**
     * @brief Private constructor for singleton pattern.
     */
    explicit Storage();
    /// Deleted copy constructor for singleton pattern.
    Storage(const Storage &) = delete;
    /// Deleted copy assignment for singleton pattern.
    Storage &operator=(const Storage &) = delete;
    /// Deleted move constructor for singleton pattern.
    Storage(Storage &&) = delete;
    /// Deleted move assignment for singleton pattern.
    Storage &operator=(Storage &&) = delete;
    /// Default destructor.
    ~Storage() = default;

    void SaveToFile() const;
    void LoadFromFile();
private:
    std::unordered_map<std::string, std::string> data;
    mutable appguard::IPCMutex mutex;
};
