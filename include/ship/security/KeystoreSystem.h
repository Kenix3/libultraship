#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Ship {

class KeystoreSystem {
  public:
    KeystoreSystem() = default;
    ~KeystoreSystem() = default;

    bool Load();

    bool AddKey(const std::string& keyName, const std::vector<uint8_t>& keyData);
    bool RemoveKey(const std::string& keyName);
    std::vector<uint8_t> GetKey(const std::string& keyName) const;
    std::vector<std::vector<uint8_t>> GetAllKeys() const;

  private:
    std::unordered_map<std::string, std::vector<uint8_t>> mKeys;
};

} // namespace Ship