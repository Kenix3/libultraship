#pragma once
#include "controller/Controller.h"
#include <string>

namespace Ship {
class WiiUGamepad : public Controller {
  public:
    WiiUGamepad(std::shared_ptr<ControlDeck> controlDeck, int32_t deviceIndex);

    bool Open();
    void Close();

    void ReadDevice(int32_t portIndex) override;
    bool Connected() const;
    bool CanGyro() const;
    bool CanRumble() const;
    bool CanSetLed() const override;
    void ClearRawPress() override;
    int32_t ReadRawPress() override;
    int32_t SetRumble(int32_t portIndex, bool rumble) override;
    int32_t SetLed(int32_t portIndex, int8_t r, int8_t g, int8_t b) override;
    const std::string GetButtonName(int32_t portIndex, int n64Button) override;

  protected:
    void CreateDefaultBinding(int32_t portIndex) override;

  private:
    bool mConnected = true;
    float mRumblePatternStrength;
    uint8_t mRumblePattern[15];
};
} // namespace Ship
