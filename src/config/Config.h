#pragma once

#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "audio/Audio.h"
#include "window/Window.h"

namespace LUS {
class ConfigVersionUpdater {
  public:
    virtual void Update(Config* conf) = 0;
};
class Config {
  public:
    Config(std::string path);
    ~Config();

    std::string GetString(const std::string& key, const std::string& defaultValue = "");
    float GetFloat(const std::string& key, float defaultValue = 0.0f);
    bool GetBool(const std::string& key, bool defaultValue = false);
    int32_t GetInt(const std::string& key, int32_t defaultValue = 0);
    uint32_t GetUInt(const std::string& key, uint32_t defaultValue = 0);
    void SetString(const std::string& key, const std::string& value);
    void SetFloat(const std::string& key, float value);
    void SetBool(const std::string& key, bool value);
    void SetInt(const std::string& key, int32_t value);
    void SetUInt(const std::string& key, uint32_t value);
    void Erase(const std::string& key);
    bool Contains(const std::string& key);
    void Reload();
    void Save();
    nlohmann::json GetNestedJson();
    nlohmann::json GetFlattenedJson();
    bool IsNewInstance();

    AudioBackend GetAudioBackend();
    void SetAudioBackend(AudioBackend backend);
    WindowBackend GetWindowBackend();
    void SetWindowBackend(WindowBackend backend);

    void RegisterConfigVersion(std::shared_ptr<ConfigVersionUpdater> version);
    void RunVersionUpdates();

  protected:
    nlohmann::json Nested(const std::string& key);
    static std::string FormatNestedKey(const std::string& key);
    template <typename T> void SetArray(const std::string& key, std::vector<T> array);
    template <typename T> std::vector<T> GetArray(const std::string& key);

  private:
    nlohmann::json mFlattenedJson;
    nlohmann::json mNestedJson;
    std::string mPath;
    bool mIsNewInstance;
    std::vector<std::shared_ptr<ConfigVersionUpdater>> mVersions;
};
} // namespace LUS
