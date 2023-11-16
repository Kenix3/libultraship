#pragma once
#include <vector>
#include <optional>

#include "Controller.h"

namespace LUS {
class TASController final : public Controller {
  public:
    TASController(int32_t deviceIndex);
    void ReadToPad(OSContPad* pad, int32_t portIndex) override;

    void ReadDevice(int32_t portIndex) override;
    const std::string GetButtonName(int32_t portIndex, int32_t n64bitmask) override;
    bool Connected() const override;
    bool CanRumble() const override;
    bool CanSetLed() const override;
    bool CanGyro() const override;
    void ClearRawPress() override;
    int32_t ReadRawPress() override;
    int32_t SetRumble(int32_t portIndex, bool rumble) override;
    int32_t SetLedColor(int32_t portIndex, Color_RGB8 color) override;
    void CreateDefaultBinding(int32_t portIndex) override;

  protected:
    std::string mButtonName;
    bool mIsConnected = false;
};
} // namespace LUS