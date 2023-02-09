#pragma once
#include <any>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

static const std::string mercuryRGBAObjectType = "RGBA";
static const std::string mercuryRGBObjectType = "RGB";

typedef enum {
    String,
    Float,
    Bool,
    Int,
    UInt,
    ArrayString,
    ArrayFloat,
    ArrayBool,
    ArrayInt,
    ArrayUInt
} MercuryType;

class Mercury {
protected:
    std::string mPath;
public:
    explicit Mercury(std::string mPath);
    nlohmann::json rjson;
    std::unordered_map<std::string, std::pair<std::any, MercuryType>> mJsonData;
    std::string getString(const std::string& key, const std::string& def = "");
    float getFloat(const std::string& key, float defValue = 0.0f);
    bool getBool(const std::string& key, bool defValue = false);
    int getInt(const std::string& key, int defValue = 0);
    bool contains(const std::string& key);
    template< typename T > std::vector<T> getArray(const std::string& key);
    void setString(const std::string& key, const std::string& value);
    void setFloat(const std::string& key, float value);
    void setBool(const std::string& key, bool value);
    void setInt(const std::string& key, int value);
    void setUInt(const std::string& key, uint32_t value);
    void erase(const std::string& key);
    void setArray(const std::string& key, std::vector<std::string> array);
    void setArray(const std::string& key, std::vector<float> array);
    void setArray(const std::string& key, std::vector<bool> array);
    void setArray(const std::string& key, std::vector<int> array);
    void setArray(const std::string& key, std::vector<uint32_t> array);

    void reload();
    void save();
    bool isNewInstance = false;

    void scanJsonObject(const std::string& parent, const nlohmann::json &j);
    void writeDataType(nlohmann::json& obj, const std::pair<std::any, MercuryType>& value);

    void setValue(const std::string &keyName, const nlohmann::json &value);
};

template< typename T >
std::vector<T> Mercury::getArray(const std::string& key) {
    if (this->mJsonData.contains(key))
        return std::any_cast<std::vector<T>>(this->mJsonData[key]);
    return std::vector<T>();
};
