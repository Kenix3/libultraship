#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace Ship {

struct KeystoreEntry {
    std::string name;
    std::vector<uint8_t> data;
    bool defaultKey;
};

class Keystore {
  public:
    Keystore();
    ~Keystore() = default;

    void Load();
    void Save();

    bool AddKey(const std::string& keyName, const std::vector<uint8_t>& keyData, bool defaultKey = false);
    bool RemoveKey(const std::string& keyName);
    bool HasKey(const std::vector<uint8_t>& keyData) const;
    std::vector<KeystoreEntry> GetKey(const std::string& keyName) const;
    std::vector<KeystoreEntry> GetAllKeys() const;

  private:
    std::unordered_map<std::string, KeystoreEntry> mKeys;
};

} // namespace Ship