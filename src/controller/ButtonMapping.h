#pragma once

#include <cstdint>
#include <string>

namespace LUS {

#define MAPPING_TYPE_GAMEPAD 0
#define MAPPING_TYPE_KEYBOARD 1
#define MAPPING_TYPE_UNKNOWN 2

class ButtonMapping {
  public:
    ButtonMapping(uint16_t bitmask);
    ButtonMapping(uint16_t bitmask, std::string uuid);
    ~ButtonMapping();

    std::string GetUuid();

    uint16_t GetBitmask();
    virtual void UpdatePad(uint16_t& padButtons) = 0;
    virtual uint8_t GetMappingType();
    virtual std::string GetButtonName();

  protected:
    uint16_t mBitmask;
    std::string mUuid;
  
  private:
    void GenerateUuid();
};
} // namespace LUS
