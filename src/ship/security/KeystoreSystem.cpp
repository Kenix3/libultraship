#include "ship/security/KeystoreSystem.h"

namespace Ship {

bool KeystoreSystem::Load() {
    return false;
}

bool KeystoreSystem::AddKey(const std::string& keyName, const std::vector<uint8_t>& keyData) {
    mKeys[keyName] = keyData;
    return true;
}

bool KeystoreSystem::RemoveKey(const std::string& keyName) {
    return mKeys.erase(keyName) > 0;
}

std::vector<uint8_t> KeystoreSystem::GetKey(const std::string& keyName) const {
    auto it = mKeys.find(keyName);
    if (it != mKeys.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::vector<uint8_t>> KeystoreSystem::GetAllKeys() const {
    std::vector<std::vector<uint8_t>> allKeys;
    for (const auto& [keyName, keyData] : mKeys) {
        allKeys.push_back(keyData);
    }
    return allKeys;
}

} // namespace Ship