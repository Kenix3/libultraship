#include "ship/security/Keystore.h"

#include "ship/utils/StringHelper.h"
#include "ship/config/Config.h"
#include "ship/Context.h"

#include <vector>

namespace Ship {

Keystore::Keystore() {
    Load();
}

bool Keystore::AddKey(const std::string& keyName, const std::vector<uint8_t>& keyData) {
    mKeys[keyName] = keyData;
    Save();
    return true;
}

bool Keystore::RemoveKey(const std::string& keyName) {
    bool result = mKeys.erase(keyName) > 0;
    if (result) {
        Save();
    }
    return result;
}

bool Keystore::HasKey(const std::vector<uint8_t>& keyData) const {
    for (const auto& [keyName, storedKeyData] : mKeys) {
        if (storedKeyData == keyData) {
            return true;
        }
    }
    return false;
}

std::vector<uint8_t> Keystore::GetKey(const std::string& keyName) const {
    auto it = mKeys.find(keyName);
    if (it != mKeys.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::vector<uint8_t>> Keystore::GetAllKeys() const {
    std::vector<std::vector<uint8_t>> allKeys;
    for (const auto& [keyName, keyData] : mKeys) {
        allKeys.push_back(keyData);
    }
    return allKeys;
}

void Keystore::Load() {
    std::shared_ptr<Config> conf = Context::GetInstance()->GetConfig();
    conf->Reload();
    auto jsonKeys = conf->GetNestedJson()["Keystore"].items();

    for (const auto& [keyName, keyData] : jsonKeys) {
        if (keyData.is_string()) {
            AddKey(keyName, StringHelper::HexToBytes(keyData.get<std::string>()));
        }
    }
}

void Keystore::Save() {
    std::shared_ptr<Config> conf = Context::GetInstance()->GetConfig();
    for (const auto& [keyName, keyData] : mKeys) {
        std::string hexString = StringHelper::BytesToHex(keyData);
        conf->SetString(StringHelper::Sprintf("Keystore.%s", keyName.c_str()), hexString);
    }
    conf->Save();
}

} // namespace Ship