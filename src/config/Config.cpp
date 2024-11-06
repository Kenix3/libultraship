#include "Config.h"

#include <fstream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <any>
#include "utils/StringHelper.h"
#include "Context.h"

#ifdef __APPLE__
#include "graphic/Fast3D/gfx_metal.h"
#endif

namespace fs = std::filesystem;

namespace Ship {
Config::Config(std::string path) : mPath(std::move(path)), mIsNewInstance(false) {
    Reload();
}

Config::~Config() {
    SPDLOG_TRACE("destruct config");
}

std::string Config::FormatNestedKey(const std::string& key) {
    std::vector<std::string> dots = StringHelper::Split(key, ".");

    std::string tmp;
    if (dots.size() > 1) {
        for (const auto& dot : dots) {
            tmp += "/" + dot;
        }
    } else {
        tmp = "/" + dots[0];
    }

    return tmp;
}

nlohmann::json Config::Nested(const std::string& key) {
    std::vector<std::string> dots = StringHelper::Split(key, ".");
    if (!mFlattenedJson.is_object()) {
        return mFlattenedJson;
    }

    nlohmann::json gjson = mFlattenedJson.unflatten();

    if (dots.size() > 1) {
        for (auto& dot : dots) {
            if (dot == "*" || gjson.contains(dot)) {
                gjson = gjson[dot];
            }
        }
        return gjson;
    }

    return gjson[key];
}

std::string Config::GetString(const std::string& key, const std::string& defaultValue) {
    nlohmann::json n = Nested(key);
    if (n.is_string() && !n.get<std::string>().empty()) {
        return n;
    }

    return defaultValue;
}

float Config::GetFloat(const std::string& key, float defaultValue) {
    nlohmann::json n = Nested(key);
    if (n.is_number_float()) {
        return n;
    }

    return defaultValue;
}

bool Config::GetBool(const std::string& key, bool defaultValue) {
    nlohmann::json n = Nested(key);
    if (n.is_boolean()) {
        return n;
    }

    return defaultValue;
}

int32_t Config::GetInt(const std::string& key, int32_t defaultValue) {
    nlohmann::json n = Nested(key);
    if (n.is_number_integer()) {
        return n;
    }

    return defaultValue;
}

uint32_t Config::GetUInt(const std::string& key, uint32_t defaultValue) {
    nlohmann::json n = Nested(key);
    if (n.is_number_unsigned()) {
        return n;
    }

    return defaultValue;
}

bool Config::Contains(const std::string& key) {
    return !Nested(key).is_null();
}

void Config::SetString(const std::string& key, const std::string& value) {
    mFlattenedJson[FormatNestedKey(key)] = value;
}

void Config::SetFloat(const std::string& key, float value) {
    mFlattenedJson[FormatNestedKey(key)] = value;
}

void Config::SetBool(const std::string& key, bool value) {
    mFlattenedJson[FormatNestedKey(key)] = value;
}

void Config::SetInt(const std::string& key, int32_t value) {
    mFlattenedJson[FormatNestedKey(key)] = value;
}

void Config::SetUInt(const std::string& key, uint32_t value) {
    mFlattenedJson[FormatNestedKey(key)] = value;
}

void Config::Erase(const std::string& key) {
    mFlattenedJson.erase(FormatNestedKey(key));
}

void Config::EraseBlock(const std::string& key) {
    nlohmann::json gjson = mFlattenedJson.unflatten();
    if (key.find(".") != std::string::npos) {
        nlohmann::json* gjson2 = &gjson;
        std::vector<std::string> dots = StringHelper::Split(key, ".");
        if (dots.size() > 1) {
            size_t curDot = 0;
            for (auto& dot : dots) {
                if (gjson2->contains(dot)) {
                    if (curDot == dots.size() - 1) {
                        gjson2->at(dot).clear();
                        gjson2->erase(dot);
                    } else {
                        gjson2 = &gjson2->at(dot);
                        curDot++;
                    }
                }
            }
        }
    } else {
        if (gjson.contains(key)) {
            gjson.erase(key);
        }
    }
    mFlattenedJson = gjson.flatten();
    Save();
}

void Config::Copy(const std::string& fromKey, const std::string& toKey) {
    auto nestedFromKey = FormatNestedKey(fromKey);
    auto nestedToKey = FormatNestedKey(toKey);
    if (mFlattenedJson.contains(nestedFromKey)) {
        mFlattenedJson[nestedToKey] = mFlattenedJson[nestedFromKey];
    }
}

void Config::Reload() {
    if (mPath == "None" || !fs::exists(mPath) || !fs::is_regular_file(mPath)) {
        mIsNewInstance = true;
        mFlattenedJson = nlohmann::json::object();
        return;
    }
    std::ifstream ifs(mPath);

    try {
        mNestedJson = nlohmann::json::parse(ifs);
        mFlattenedJson = mNestedJson.flatten();
    } catch (...) { mFlattenedJson = nlohmann::json::object(); }
}

void Config::Save() {
    std::ofstream file(mPath);
    file << mFlattenedJson.unflatten().dump(4);
}

template <typename T> std::vector<T> Config::GetArray(const std::string& key) {
    if (nlohmann::json tmp = Nested(key); tmp.is_array()) {
        return tmp.get<std::vector<T>>();
    }
    return std::vector<T>();
};

template <typename T> void Config::SetArray(const std::string& key, std::vector<T> array) {
    mFlattenedJson[FormatNestedKey(key)] = nlohmann::json(array);
}

nlohmann::json Config::GetNestedJson() {
    return mNestedJson;
}

nlohmann::json Config::GetFlattenedJson() {
    return mFlattenedJson;
}

bool Config::IsNewInstance() {
    return mIsNewInstance;
}

AudioBackend Config::GetCurrentAudioBackend() {
    std::string backendName = GetString("Window.AudioBackend");
    if (backendName == "wasapi") {
        return AudioBackend::WASAPI;
    }

    // Migrate pulse player in config to sdl
    if (backendName == "pulse") {
        SetCurrentAudioBackend(AudioBackend::SDL);
        return AudioBackend::SDL;
    }

    if (backendName == "sdl") {
        return AudioBackend::SDL;
    }

    SPDLOG_TRACE("Could not find AudioBackend matching value from config file ({}). Returning default AudioBackend.",
                 backendName);
#ifdef _WIN32
    return AudioBackend::WASAPI;
#endif

    return AudioBackend::SDL;
}

void Config::SetCurrentAudioBackend(AudioBackend backend) {
    switch (backend) {
        case AudioBackend::WASAPI:
            SetString("Window.AudioBackend", "wasapi");
            break;
        case AudioBackend::SDL:
            SetString("Window.AudioBackend", "sdl");
            break;
        default:
            SetString("Window.AudioBackend", "");
    }
}

WindowBackend Config::GetWindowBackend() {
    WindowBackend backend;
    auto backendId = GetInt("Window.Backend.Id", -1);
    if (Context::GetInstance()->GetWindow()->IsAvailableWindowBackend(backendId)) {
        return static_cast<WindowBackend>(backendId);
    }

    SPDLOG_TRACE(
        "Could not find available WindowBackend matching id from config file ({}). Returning default WindowBackend.",
        backendId);
#ifdef ENABLE_DX11
    return WindowBackend::FAST3D_DXGI_DX11;
#endif
#ifdef __APPLE__
    if (Metal_IsSupported()) {
        return WindowBackend::FAST3D_SDL_METAL;
    }
#endif
    return WindowBackend::FAST3D_SDL_OPENGL;
}

void Config::SetWindowBackend(WindowBackend backend) {
    SetInt("Window.Backend.Id", static_cast<int>(backend));

    switch (backend) {
        case WindowBackend::FAST3D_DXGI_DX11:
            SetString("Window.Backend.Name", "DirectX 11");
            break;
        case WindowBackend::FAST3D_SDL_OPENGL:
            SetString("Window.Backend.Name", "OpenGL");
            break;
        case WindowBackend::FAST3D_SDL_METAL:
            SetString("Window.Backend.Name", "Metal");
            break;
        default:
            SetString("Window.Backend.Name", "");
    }
}

bool Config::RegisterConfigVersionUpdater(std::shared_ptr<ConfigVersionUpdater> versionUpdater) {
    auto [_, emplaced] = mVersionUpdaters.emplace(versionUpdater->GetVersion(), versionUpdater);
    return emplaced;
}

void Config::RunVersionUpdates() {
    for (auto [_, versionUpdater] : mVersionUpdaters) {
        uint32_t version = GetUInt("ConfigVersion", 0);
        if (version < versionUpdater->GetVersion()) {
            versionUpdater->Update(this);
            SetUInt("ConfigVersion", versionUpdater->GetVersion());
        }
    }
    Save();
}

ConfigVersionUpdater::ConfigVersionUpdater(uint32_t toVersion) : mVersion(toVersion) {
}

uint32_t ConfigVersionUpdater::GetVersion() {
    return mVersion;
}

} // namespace Ship
