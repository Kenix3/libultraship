#pragma once
#include <vector>
#include <optional>

#include "Controller.h"

namespace Ship {
class DummyController final : public Controller {
  public:
    DummyController(std::shared_ptr<ControlDeck> controlDeck, int32_t deviceIndex, const std::string& guid, const std::string& keyName, bool connected);
    std::map<std::vector<std::string>, int32_t> ReadButtonPress();
    void ReadDevice(int32_t portIndex) override;
    const std::string GetButtonName(int32_t portIndex, int32_t n64Button) override;
    bool Connected() const override;
    bool CanRumble() const override;
    bool CanSetLed() const override;
    bool CanGyro() const override;
    void ClearRawPress() override;
    int32_t ReadRawPress() override;
    int32_t SetRumble(int32_t portIndex, bool rumble) override;
    int32_t SetLed(int32_t portIndex, int8_t r, int8_t g, int8_t b) override;
    void CreateDefaultBinding(int32_t portIndex) override;

protected:
    std::string mButtonName;
    bool mIsConnected = false;
};
} // namespace Ship