#include "JsonConfig.h"
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

JsonConfig::JsonConfig(std::string path) : mPath(path) {
    // check if path exists and that it's a file
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        mIsNewConfig = true;
        mJson = json::object();
        return;
    }

    try {
        std::ifstream ifs(path);
        mJson = json::parse(ifs);
    } catch (json::parse_error& e) {
        SPDLOG_ERROR("Failed to parse JSON file: {}", e.what());

        // If failure to parse, we will create a new file
        mIsNewConfig = true;
        mJson = json::object();
    }
}

json::json_pointer JsonConfig::DotKeyToPointer(std::string key) {
    std::replace(key.begin(), key.end(), '.', '/');
    key = "/" + key;

    return json::json_pointer(key);
}

void JsonConfig::SetArbitraryType(std::string key, json::value_type value) {
    auto ptr = DotKeyToPointer(key);
    mJson[ptr] = value;
}

json::value_type JsonConfig::GetArbitraryType(std::string key) {
    auto ptr = DotKeyToPointer(key);

    try {
        return mJson.at(ptr);
    } catch (json::out_of_range& e) { return nullptr; }
}

// MARK: - Public API

bool JsonConfig::IsNewConfig() {
    return mIsNewConfig;
}

void JsonConfig::DeleteEntry(std::string key) {
    // nlohmann::json doesn't have a way to delete a json pointer so
    // we'll work around it by grabbing the parent object of the key
    // we want to delete and deleting the key

    auto currentKey = key;
    auto parentKey = key;

    try {
        bool shouldStop = false;
        while (!shouldStop) {
            // grab parent of current key
            parentKey = currentKey.substr(0, currentKey.find_last_of('.'));
            auto parentPtr = DotKeyToPointer(parentKey);

            // grab parent object at key and delete the current key
            auto& parent = mJson.at(parentPtr);
            parent.erase(currentKey.substr(currentKey.find_last_of('.') + 1));

            // set variable for whether we should keep recursing parents
            shouldStop = parent.empty() || parentKey == currentKey;
            currentKey = parentKey;
        }
    } catch (json::out_of_range& e) {
        // at can throw, showing that a parent object doesn't exist
        // ignore and don't do anything
    }
}

json::value_type JsonConfig::GetRawEntry(std::string key) {
    return GetArbitraryType(key);
}

void JsonConfig::SetInteger(std::string key, int32_t value) {
    return SetArbitraryType(key, value);
}

int32_t JsonConfig::GetInteger(std::string key, int32_t defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_number_integer()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonConfig::SetUInteger(std::string key, uint32_t value) {
    return SetArbitraryType(key, value);
}

uint32_t JsonConfig::GetUInteger(std::string key, uint32_t defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_number_unsigned()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonConfig::SetFloat(std::string key, float value) {
    return SetArbitraryType(key, value);
}

float JsonConfig::GetFloat(std::string key, float defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_number_float()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonConfig::SetString(std::string key, std::string value) {
    return SetArbitraryType(key, value);
}

std::string JsonConfig::GetString(std::string key, std::string defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_string() && !value.get<std::string>().empty()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonConfig::SetBoolean(std::string key, bool value) {
    return SetArbitraryType(key, value);
}

bool JsonConfig::GetBoolean(std::string key, bool defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_boolean()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonConfig::PersistToDisk() {
#if defined(__SWITCH__) || defined(__WIIU__)
    FILE* w = fopen(mPath.c_str(), "w");
    std::string jsonStr = mJson.dump(4);
    fwrite(jsonStr.c_str(), sizeof(char), jsonStr.length(), w);
    fclose(w);
#else
    std::ofstream file(mPath);
    file << mJson.dump(4);
    file.close();
#endif

    mIsNewConfig = false;
}
