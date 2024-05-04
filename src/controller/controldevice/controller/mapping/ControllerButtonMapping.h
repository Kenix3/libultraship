#pragma once

#include <cstdint>
#include <string>

#include "ControllerInputMapping.h"

namespace Ship {

#ifndef CONTROLLERBUTTONS_T
#define CONTROLLERBUTTONS_T uint16_t
#endif

class ControllerButtonMapping : virtual public ControllerInputMapping {
  public:
    ControllerButtonMapping(ShipDeviceIndex shipDeviceIndex, uint8_t portIndex, CONTROLLERBUTTONS_T bitmask);
    ~ControllerButtonMapping();

    virtual std::string GetButtonMappingId() = 0;

    CONTROLLERBUTTONS_T GetBitmask();
    virtual void UpdatePad(CONTROLLERBUTTONS_T& padButtons) = 0;
    virtual uint8_t GetMappingType();
    void SetPortIndex(uint8_t portIndex);

    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;

  protected:
    uint8_t mPortIndex;
    CONTROLLERBUTTONS_T mBitmask;
};
} // namespace Ship
