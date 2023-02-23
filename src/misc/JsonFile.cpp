#include "JsonFile.h"
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>
#include <Utils/StringHelper.h>

namespace fs = std::filesystem;

JsonFile::JsonFile(std::string path) : mPath(path) {
    // check if path exists and that it's a file
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        mIsNewFile = true;
        mJson = json::object();
        return;
    }

    try {
        std::ifstream ifs(path);
        mJson = json::parse(ifs);
    }  catch (json::parse_error& e) {
        SPDLOG_ERROR("Failed to parse JSON file: {}", e.what());

        // If failure to parse, we will create a new file
        mIsNewFile = true;
        mJson = json::object();
    }
}

void JsonFile::SetArbitraryType(std::string key, json::value_type value) {
    auto keyParts = StringHelper::Split(key, ".");

    if (keyParts.size() > 1) {
        // nested key
        json* currentJson = &mJson;
        for (size_t i = 0; i < keyParts.size() - 1; i++) {
            if (!currentJson->contains(keyParts[i])) {
                // create new object
                (*currentJson)[keyParts[i]] = json::object();
            }
            currentJson = &(*currentJson)[keyParts[i]];
        }
        (*currentJson)[keyParts[keyParts.size() - 1]] = value;
    } else {
        // not nested
        mJson[key] = value;
    }
}

json::value_type JsonFile::GetArbitraryType(std::string key) {
    auto keyParts = StringHelper::Split(key, ".");
    
    // find the deepest nested key if it exists
    json* currentJson = &mJson;
    for (size_t i = 0; i < keyParts.size() - 1; i++) {
        if (!currentJson->contains(keyParts[i])) {
            return nullptr;
        }
        currentJson = &(*currentJson)[keyParts[i]];
    }

    // extract the value if possible
    if (currentJson->contains(keyParts[keyParts.size() - 1])) {
        return (*currentJson)[keyParts[keyParts.size() - 1]];
    } else {
        return nullptr;
    }
}

// MARK: - Public API

void JsonFile::DeleteEntry(std::string key) {
    auto keyParts = StringHelper::Split(key, ".");

    // clear nested key, if after deletion parent is empty, delete it too
    if (keyParts.size() > 1) {
        json* currentJson = &mJson;
        for (size_t i = 0; i < keyParts.size() - 1; i++) {
            if (!currentJson->contains(keyParts[i])) {
                return;
            }
            currentJson = &(*currentJson)[keyParts[i]];
        }
        (*currentJson).erase(keyParts[keyParts.size() - 1]);

        // delete empty parents
        for (size_t i = keyParts.size() - 1; i > 0; i--) {
            if ((*currentJson).empty()) {
                currentJson = &mJson;
                for (size_t j = 0; j < i - 1; j++) {
                    currentJson = &(*currentJson)[keyParts[j]];
                }
                (*currentJson).erase(keyParts[i - 1]);
            }
        }
    } else {
        // not nested
        mJson.erase(key);
    }
}

json::value_type JsonFile::GetRawEntry(std::string key) {
    return GetArbitraryType(key);
}

void JsonFile::SetInteger(std::string key, int32_t value) {
    return SetArbitraryType(key, value);
}

int32_t JsonFile::GetInteger(std::string key, int32_t defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_number_integer()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonFile::SetUInteger(std::string key, uint32_t value) {
    return SetArbitraryType(key, value);
}

uint32_t JsonFile::GetUInteger(std::string key, uint32_t defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_number_unsigned()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonFile::SetFloat(std::string key, float value) {
    return SetArbitraryType(key, value);
}

float JsonFile::GetFloat(std::string key, float defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_number_float()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonFile::SetString(std::string key, std::string value) {
    return SetArbitraryType(key, value);
}

std::string JsonFile::GetString(std::string key, std::string defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_string() && !value.get<std::string>().empty()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonFile::SetBoolean(std::string key, bool value) {
    return SetArbitraryType(key, value);
}

bool JsonFile::GetBoolean(std::string key, bool defaultValue) {
    json::value_type value = GetArbitraryType(key);
    if (value.is_boolean()) {
        return value;
    } else {
        return defaultValue;
    }
}

void JsonFile::PersistToDisk() {
    std::ofstream file(mPath);
    file << mJson.dump(4);
    file.close();
    
    mIsNewFile = false;
}
