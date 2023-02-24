#include <stdint.h>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JsonConfig {
  private:
    /// @brief The path to the JSON file
    std::string mPath;

    /// @brief The backing JSON object (in memory)
    json mJson;

    /// @brief Sets a value of arbitrary type to the JSON object
    /// @param key The dot-separated key
    /// @param value The value to set
    void SetArbitraryType(std::string key, json::value_type value);

    /// @brief Gets a value of arbitrary type from the JSON object
    /// @param key The dot-separated key
    /// @return The value at the key or nullptr if does not exist
    json::value_type GetArbitraryType(std::string key);

  public:
    explicit JsonConfig(std::string path);

    /// @brief Indicates whether the config file is new or not
    /// @return True when inited with an invalid path. False otherwise.
    bool IsNewConfig();

    /// @brief Grab the underlying JSON object at the key or nullptr if does not exist
    /// @param key The dot-separated key
    /// @return The value at the key or nullptr if does not exist
    json::value_type GetRawEntry(std::string key);

    /// @brief Deletes the entry at the key
    /// @param key The dot-separated key
    /// @details If the key is a parent key, all children will be deleted as well.
    void DeleteEntry(std::string key);

    void SetInteger(std::string key, int32_t value);
    int32_t GetInteger(std::string key, int32_t defaultValue = 0);

    void SetUInteger(std::string key, uint32_t value);
    uint32_t GetUInteger(std::string key, uint32_t defaultValue = 0);

    void SetFloat(std::string key, float value);
    float GetFloat(std::string key, float defaultValue = 0.0f);

    void SetString(std::string key, std::string value);
    std::string GetString(std::string key, std::string defaultValue = "");

    void SetBoolean(std::string key, bool value);
    bool GetBoolean(std::string key, bool defaultValue = false);

    /// @brief Saves the JSON object to disk
    void PersistToDisk();

  private:
    /// @brief Indicates whether the config file is new or not
    /// @details This is set to true when inited with an invalid path and will be created upon save.
    bool mIsNewConfig = false;

    /// @brief Converts a dot-separated key to a JSON pointer
    /// @param key The dot-separated key
    /// @return The JSON pointer
    json::json_pointer DotKeyToPointer(std::string key);
};
