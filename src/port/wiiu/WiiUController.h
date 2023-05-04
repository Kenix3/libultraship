#pragma once
#include "controller/Controller.h"
#include <string>

// since clang-tidy doesn't build Wii U, this file is not found
// this error is benign, so we're supressing it
// NOLINTNEXTLINE
#include <padscore/wpad.h>

namespace Ship {
class WiiUController : public Controller {
  public:
    WiiUController(std::shared_ptr<ControlDeck> controlDeck, int32_t deviceIndex, WPADChan chan);
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
    std::string GetControllerExtensionName();

    bool mConnected;
    WPADChan mChan;
    WPADExtensionType mExtensionType;
};
} // namespace Ship
