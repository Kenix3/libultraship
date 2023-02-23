#include <stdint.h>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class JsonConfig {
  private:
    std::string mPath;
    json mJson;

    void SetArbitraryType(std::string key, json::value_type value);
    json::value_type GetArbitraryType(std::string key);

  public:
    explicit JsonConfig(std::string path);

    /// Set to `true` when JsonFile is inited with an invalid path.
    /// Indicating that this is considered a "fresh file" and will be created upon save.
    bool mIsNewFile = false;

    /// Grab the underlying JSON object at the key or nullptr if does not exist
    json::value_type GetRawEntry(std::string key);

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

    void PersistToDisk();
};
