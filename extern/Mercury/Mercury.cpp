#include "Mercury.h"

#include <fstream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <any>
#include <Utils/StringHelper.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

Mercury::Mercury(std::string mPath) : mPath(std::move(mPath)) {
	this->reload();
}

std::string Mercury::getString(const std::string& key, const std::string& def) {
    if (this->mJsonData.contains(key)) {
        return std::any_cast<std::string>(this->mJsonData[key].first);
    }
    return def;
}

float Mercury::getFloat(const std::string& key, float def) {
    if (this->mJsonData.contains(key)) {
        return std::any_cast<float>(this->mJsonData[key].first);
    }
    return def;
}

bool Mercury::getBool(const std::string& key, bool def) {
    if (this->mJsonData.contains(key)) {
        return std::any_cast<bool>(this->mJsonData[key].first);
    }
    return def;
}

int Mercury::getInt(const std::string& key, int def) {
    if (this->mJsonData.contains(key)) {
        return std::any_cast<int>(this->mJsonData[key].first);
    }
    return def;
}

bool Mercury::contains(const std::string& key) {
    return this->mJsonData.contains(key);
}

void Mercury::setString(const std::string& key, const std::string& value) {
    this->mJsonData[key] = std::make_pair(value, MercuryType::String);
}

void Mercury::setFloat(const std::string& key, float value) {
    this->mJsonData[key] = std::make_pair(value, MercuryType::Float);
}

void Mercury::setBool(const std::string& key, bool value) {
    this->mJsonData[key] = std::make_pair(value, MercuryType::Bool);
}

void Mercury::setInt(const std::string& key, int value) {
    this->mJsonData[key] = std::make_pair(value, MercuryType::Int);
}

void Mercury::setUInt(const std::string& key, uint32_t value) {
    this->mJsonData[key] = std::make_pair(value, MercuryType::UInt);
}

void Mercury::setArray(const std::string& key, std::vector<std::string> array) {
    this->mJsonData[key] = std::make_pair(array, MercuryType::ArrayString);
}

void Mercury::setArray(const std::string& key, std::vector<float> array) {
    this->mJsonData[key] = std::make_pair(array, MercuryType::ArrayFloat);
}

void Mercury::setArray(const std::string& key, std::vector<bool> array) {
    this->mJsonData[key] = std::make_pair(array, MercuryType::ArrayBool);
}

void Mercury::setArray(const std::string& key, std::vector<int> array) {
    this->mJsonData[key] = std::make_pair(array, MercuryType::ArrayInt);
}

void Mercury::setArray(const std::string& key, std::vector<uint32_t> array) {
    this->mJsonData[key] = std::make_pair(array, MercuryType::ArrayUInt);
}

void Mercury::erase(const std::string& key) {
    this->mJsonData.erase(key);
}

void Mercury::reload() {
    if (this->mPath.empty() || !fs::exists(this->mPath) || !fs::is_regular_file(this->mPath)) {
        return;
    }
    try {
        this->rjson = json::parse(std::ifstream(this->mPath));
        scanJsonObject("", this->rjson);
    } catch (json::parse_error& ex) {
        // OTRTODO: Log error
    }
}

void Mercury::save() {
    if (this->mPath.empty()) {
        return;
    }

    for (auto& [key, value] : this->mJsonData) {
        std::vector<std::string> split = StringHelper::Split(key, ".");
        if(split.size() == 1) {
            writeDataType(this->rjson[split[0]], value);
        } else {
            json* current = &this->rjson;
            for (int i = 0; i < split.size() - 1; i++) {
                if (current->contains(split[i])) {
                    current = &(*current)[split[i]];
                } else {
                    (*current)[split[i]] = json::object();
                    current = &(*current)[split[i]];
                }
            }
            writeDataType((*current)[split[split.size() - 1]], value);
        }
    }

    try {
        std::ofstream file(this->mPath);
        file << this->rjson.dump(4);
    } catch (...) {
        // OTRTODO: Log error
    }
}

void Mercury::scanJsonObject(const std::string& parent, const nlohmann::json& j) {
    for (auto& [key, value] : j.items()) {
        std::string keyName = parent.empty() ? key : parent + "." + key;
        if (value.is_string()) {
            this->mJsonData[keyName] = std::make_pair(value.get<std::string>(), MercuryType::String);
        } else if (value.is_number_float()) {
            this->mJsonData[keyName] = std::make_pair(value.get<float>(), MercuryType::Float);
        } else if (value.is_boolean()) {
            this->mJsonData[keyName] = std::make_pair(value.get<bool>(), MercuryType::Bool);
        } else if (value.is_number_integer()) {
            this->mJsonData[keyName] = std::make_pair(value.get<int>(), MercuryType::Int);
        } else if (value.is_array()) {
            for(auto& item : value) {
                this->scanJsonObject(keyName, item);
            }
        } else if (value.is_object()) {
            scanJsonObject(keyName, value);
        } else {
            int bp = 0;
        }
    }
}

void Mercury::writeDataType(nlohmann::json& obj, const std::pair<std::any, MercuryType>& value) {
    switch (value.second) {
        case MercuryType::String:
            obj = std::any_cast<std::string>(value.first);
            break;
        case MercuryType::Float:
            obj = std::any_cast<float>(value.first);
            break;
        case MercuryType::Bool:
            obj = std::any_cast<bool>(value.first);
            break;
        case MercuryType::Int:
            obj = std::any_cast<int>(value.first);
            break;
        case MercuryType::UInt:
            obj = std::any_cast<uint32_t>(value.first);
            break;
        case MercuryType::ArrayBool:
            obj = std::any_cast<std::vector<bool>>(value.first);
            break;
        case MercuryType::ArrayFloat:
            obj = std::any_cast<std::vector<float>>(value.first);
            break;
        case MercuryType::ArrayInt:
            obj = std::any_cast<std::vector<int>>(value.first);
            break;
        case MercuryType::ArrayString:
            obj = std::any_cast<std::vector<std::string>>(value.first);
            break;
        case MercuryType::ArrayUInt:
            obj = std::any_cast<std::vector<uint32_t>>(value.first);
            break;
        default:
            break;
    }
}