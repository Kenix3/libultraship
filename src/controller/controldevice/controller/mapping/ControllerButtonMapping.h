#pragma once

#include <cstdint>
#include <string>
#include "MappingTypes.h"

namespace LUS {

class ControllerButtonMapping {
  public:
    ControllerButtonMapping(uint16_t bitmask);
    ControllerButtonMapping(std::string uuid, uint16_t bitmask);
    ~ControllerButtonMapping();

    std::string GetUuid();

    uint16_t GetBitmask();
    virtual void UpdatePad(uint16_t& padButtons) = 0;
    virtual uint8_t GetMappingType();
    virtual std::string GetButtonName();

    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;

  protected:
    uint16_t mBitmask;
    std::string mUuid;

  private:
    void GenerateUuid();
};
} // namespace LUS
