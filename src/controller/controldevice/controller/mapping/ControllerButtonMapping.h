#pragma once

#include <cstdint>
#include <string>

#include "ControllerInputMapping.h"

namespace LUS {

class ControllerButtonMapping : virtual public ControllerInputMapping {
  public:
    ControllerButtonMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, uint16_t bitmask);
    ~ControllerButtonMapping();

    virtual std::string GetButtonMappingId() = 0;

    uint16_t GetBitmask();
    virtual void UpdatePad(uint16_t& padButtons) = 0;
    virtual uint8_t GetMappingType();
    void SetPortIndex(uint8_t portIndex);

    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;

  protected:
    uint8_t mPortIndex;
    uint16_t mBitmask;
};
} // namespace LUS
