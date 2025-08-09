#pragma once

#include "controller/physicaldevice/gc/GCAdapter.h"
#include "controller/controldevice/controller/mapping/ControllerInputMapping.h"

#ifndef CONTROLLERBUTTONS_T
#define CONTROLLERBUTTONS_T uint16_t
#endif

namespace Ship {
class GCAdapterButtonToAnyMapping : virtual public ControllerInputMapping {
  public:
    GCAdapterButtonToAnyMapping(CONTROLLERBUTTONS_T gcButton);
    virtual ~GCAdapterButtonToAnyMapping();
    std::string GetPhysicalInputName() override;
    std::string GetPhysicalDeviceName() override;

  protected:
    CONTROLLERBUTTONS_T mGcButton;

  private:
    std::string GetGenericButtonName();
};
} // namespace Ship
