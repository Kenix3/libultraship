#ifdef __SWITCH__
#pragma once

#include "controller/Controller.h"
#include <string>
#include <switch.h>

namespace Ship {

struct NXControllerState {
    PadState State;
    HidVibrationDeviceHandle Handles[2][2];
    HidSixAxisSensorHandle Sensors[4];
};

class SwitchController : public Controller {
  public:
    SwitchController(int32_t physicalSlot);
    bool Open();
    void Close();

    void ReadFromSource(int32_t virtualSlot) override;
    void WriteToSource(int32_t virtualSlot, ControllerCallback* controller) override;
    bool Connected() const override {
        return mConnected;
    };
    bool CanGyro() const override {
        return true;
    }
    bool CanRumble() const override {
        return true;
    };

    void ClearRawPress() override {
    }
    int32_t ReadRawPress() override;

    const std::string GetButtonName(int32_t virtualSlot, int n64Button) override;
    const std::string GetControllerName() override;

  protected:
    void NormalizeStickAxis(int32_t virtualSlot, float x, float y, int16_t axisThreshold, bool isRightStick);
    void CreateDefaultBinding(int32_t virtualSlot) override;

  private:
    NXControllerState* mController;
    std::string GetControllerExtensionName();
    void UpdateSixAxisSensor(HidSixAxisSensorState& state);
    int32_t mPhysicalSlot;

    bool mConnected;
};
} // namespace Ship
#endif