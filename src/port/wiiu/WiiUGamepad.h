#pragma once
#include "controller/Controller.h"
#include <string>

namespace Ship {
class WiiUGamepad : public Controller {
  public:
    WiiUGamepad(std::shared_ptr<ControlDeck> controlDeck, int32_t deviceIndex);

    bool Open();
    void Close();

    void ReadFromSource(int32_t portIndex) override;
    void WriteToSource(int32_t portIndex, ControllerCallback* controller) override;
    bool Connected() const;
    bool CanGyro() const;
    bool CanRumble() const;

    void ClearRawPress() override;
    int32_t ReadRawPress() override;

    const std::string GetButtonName(int32_t portIndex, int n64Button) override;

  protected:
    void CreateDefaultBinding(int32_t portIndex) override;

  private:
    bool mConnected = true;
    float mRumblePatternStrength;
    uint8_t mRumblePattern[15];
};
} // namespace Ship
