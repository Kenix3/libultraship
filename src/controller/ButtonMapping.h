#pragma once

#include <cstdint>
#include <string>

namespace LUS {
class ButtonMapping {
  public:
    ButtonMapping(uint16_t bitmask);
    ButtonMapping(uint16_t bitmask, std::string uuid);
    ~ButtonMapping();

    std::string GetUuid();

    uint16_t GetBitmask();
    virtual void UpdatePad(uint16_t& padButtons) = 0;

  protected:
    uint16_t mBitmask;
    std::string mUuid;
  
  private:
    void GenerateUuid();
};
} // namespace LUS
