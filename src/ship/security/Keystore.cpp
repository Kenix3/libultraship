#include "ship/security/Keystore.h"

#include "ship/utils/StringHelper.h"
#include "ship/config/Config.h"
#include "ship/Context.h"
#include "ship/DefaultKeys.h"

#include <vector>

namespace Ship {

Keystore::Keystore() {
    Load();
    for (const auto entry : AllDefaultKeys) {
        AddKey(std::string(entry.name), StringHelper::HexToBytes(std::string(entry.data)), KeyOrigin::System);
    }
}

bool Keystore::AddKey(const std::string& keyName, const std::vector<uint8_t>& keyData, KeyOrigin origin) {
    mKeys[keyName] = KeystoreEntry{ keyName, keyData, origin };
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
    for (const auto& [keyName, entry] : mKeys) {
        if (entry.Data == keyData) {
            return true;
        }
    }
    return false;
}

std::vector<KeystoreEntry> Keystore::GetKey(const std::string& keyName) const {
    std::vector<KeystoreEntry> entries;
    auto it = mKeys.find(keyName);
    if (it != mKeys.end()) {
        entries.push_back(it->second);
    }
    return entries;
}

std::vector<KeystoreEntry> Keystore::GetAllKeys() const {
    std::vector<KeystoreEntry> allKeys;
    for (const auto& [keyName, entry] : mKeys) {
        allKeys.push_back(entry);
    }
    return allKeys;
}

void Keystore::Load() {
    std::shared_ptr<Config> conf = Context::GetInstance()->GetChildren().GetFirst<Config>();
    conf->Reload();

    nlohmann::json rootJson = conf->GetNestedJson();

    if (!rootJson.contains("Keystore") || !rootJson["Keystore"].is_object()) {
        SPDLOG_WARN("Keystore not found in config or is empty.");
        return;
    }

    nlohmann::json keystoreNode = rootJson["Keystore"];
    for (const auto& [keyName, keyData] : keystoreNode.items()) {
        if (keyData.is_string()) {
            AddKey(keyName, StringHelper::HexToBytes(keyData.get<std::string>()), KeyOrigin::User);
        } else {
            SPDLOG_WARN("Skipping key {}: Data is not a string.", keyName);
        }
    }
}

void Keystore::Save() {
    std::shared_ptr<Config> conf = Context::GetInstance()->GetChildren().GetFirst<Config>();
    for (const auto& [keyName, entry] : mKeys) {
        std::string hexString = StringHelper::BytesToHex(entry.Data);
        conf->SetString(StringHelper::Sprintf("Keystore.%s", keyName.c_str()), hexString);
    }
    conf->Save();
}

} // namespace Ship