#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace Ship {

class Keystore {
  public:
    Keystore();
    ~Keystore() = default;

    void Load();
    void Save();

    bool AddKey(const std::string& keyName, const std::vector<uint8_t>& keyData);
    bool RemoveKey(const std::string& keyName);
    bool HasKey(const std::vector<uint8_t>& keyData) const;
    std::vector<uint8_t> GetKey(const std::string& keyName) const;
    std::vector<std::vector<uint8_t>> GetAllKeys() const;

  private:
    std::unordered_map<std::string, std::vector<uint8_t>> mKeys;
};

} // namespace Ship