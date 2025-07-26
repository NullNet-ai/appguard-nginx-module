#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <mutex>
#include <optional>

/**
 * @brief Thread-safe singleton for persistent key-value storage.
 * Automatically saves data to disk on each modification.
 */
class PersistentStorage
{
public:
    /**
     * @brief Gets the singleton instance.
     * @return Reference to the singleton instance.
     */
    static PersistentStorage &GetInstance();
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
     * @brief Checks if a key exists in storage.
     * @param key The key to check.
     * @return true if key exists, false otherwise.
     */
    bool Exists(const std::string &key) const;
    /**
     * @brief Removes all data from storage and disk.
     * This operation cannot be undone.
     */
    void Clear();

private:
    /**
     * @brief Private constructor for singleton pattern.
     * @param filename Path to the storage file.
     */
    explicit PersistentStorage(const std::string &filename);
    /// Deleted copy constructor for singleton pattern.
    PersistentStorage(const PersistentStorage &) = delete;
    /// Deleted copy assignment for singleton pattern.
    PersistentStorage &operator=(const PersistentStorage &) = delete;
    /// Deleted move constructor for singleton pattern.
    PersistentStorage(PersistentStorage &&) = delete;
    /// Deleted move assignment for singleton pattern.
    PersistentStorage &operator=(PersistentStorage &&) = delete;
    /// Default destructor.
    ~PersistentStorage() = default;
    /**
     * @brief Saves current data to disk (mutex must be locked).
     * @throws AppGuardClientException on file operation failure.
     */
    void SaveInternal() const;
    /**
     * @brief Loads data from disk (mutex must be locked).
     * @throws AppGuardClientException on parse failure.
     */
    void LoadInternal();

private:
    std::string filename;
    std::unordered_map<std::string, std::string> data;
    mutable std::mutex mutex;
};
