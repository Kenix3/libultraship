#pragma once
#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "controller/physicaldevice/gc/GCAdapter.h"
#include "GCAdapterButtonToAnyMapping.h"

namespace Ship {
class GCAdapterButtonToButtonMapping :  public GCAdapterButtonToAnyMapping, public ControllerButtonMapping {
  public:
    GCAdapterButtonToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, CONTROLLERBUTTONS_T gcButton);
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;

    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;

  protected:
    CONTROLLERBUTTONS_T mGcButton;
};
} // namespace Ship