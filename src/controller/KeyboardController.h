#pragma once
#include "Controller.h"
#include <string>

namespace Ship {
class KeyboardController : public Controller {
  public:
    KeyboardController(std::shared_ptr<ControlDeck> controlDeck, int32_t deviceIndex);

    void ReadFromSource(int32_t portIndex) override;
    void WriteToSource(int32_t portIndex, ControllerCallback* controller) override;
    const std::string GetButtonName(int32_t portIndex, int32_t n64Button) override;
    bool PressButton(int32_t scancode);
    bool ReleaseButton(int32_t scancode);
    bool Connected() const override;
    bool CanRumble() const override;
    bool CanGyro() const override;
    void ClearRawPress() override;
    int32_t ReadRawPress() override;
    void ReleaseAllButtons();
    void SetLastScancode(int32_t key);
    int32_t GetLastScancode();
    void CreateDefaultBinding(int32_t portIndex) override;

  protected:
    int32_t mLastScancode;
    int32_t mLastKey = -1;
};
} // namespace Ship
