#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace Ship {

enum class KeyOrigin { User, Game, System };

struct KeystoreEntry {
    std::string Name;
    std::vector<uint8_t> Data;
    KeyOrigin Origin;
};

class Keystore {
  public:
    Keystore();
    ~Keystore() = default;

    void Load();
    void Save();

    bool AddKey(const std::string& keyName, const std::vector<uint8_t>& keyData, KeyOrigin origin = KeyOrigin::Game);
    bool RemoveKey(const std::string& keyName);
    bool HasKey(const std::vector<uint8_t>& keyData) const;
    std::vector<KeystoreEntry> GetKey(const std::string& keyName) const;
    std::vector<KeystoreEntry> GetAllKeys() const;

  private:
    std::unordered_map<std::string, KeystoreEntry> mKeys;
};

} // namespace Ship