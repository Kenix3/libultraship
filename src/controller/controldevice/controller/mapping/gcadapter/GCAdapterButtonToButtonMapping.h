#pragma once
#include "controller/controldevice/controller/mapping/ControllerButtonMapping.h"
#include "controller/physicaldevice/gc/GCAdapter.h" // Your driver header
#include "GCAdapterButtonToAnyMapping.h"

namespace Ship {
class GCAdapterButtonToButtonMapping : public GCAdapterButtonToAnyMapping, public ControllerButtonMapping {
  public:
    GCAdapterButtonToButtonMapping(uint8_t portIndex, CONTROLLERBUTTONS_T bitmask, CONTROLLERBUTTONS_T gcButton);
    void UpdatePad(CONTROLLERBUTTONS_T& padButtons) override;
    // Other virtual methods can be added later if needed for the UI

    std::string GetButtonMappingId() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;

  protected:
    CONTROLLERBUTTONS_T mGcButton;
};
} // namespace Ship