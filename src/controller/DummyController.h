#pragma once
#include <vector>
#include <optional>

#include "Controller.h"

namespace Ship {
class DummyController final : public Controller {
  public:
    DummyController(std::shared_ptr<ControlDeck> controlDeck, int32_t deviceIndex, const std::string& guid, const std::string& keyName, bool connected);
    std::map<std::vector<std::string>, int32_t> ReadButtonPress();
    void ReadFromSource(int32_t portIndex) override;
    const std::string GetButtonName(int32_t portIndex, int32_t n64Button) override;
    void WriteToSource(int32_t portIndex, ControllerCallback* controller) override;
    bool Connected() const override;
    bool CanRumble() const override;
    bool CanGyro() const override;
    void ClearRawPress() override;
    int32_t ReadRawPress() override;
    bool HasPadConf() const;
    std::optional<std::string> GetPadConfSection();
    void CreateDefaultBinding(int32_t portIndex) override;

  protected:
    std::string mButtonName;
    bool mIsConnected = false;
    std::string GetControllerType();
    std::string GetConfSection();
    std::string GetBindingConfSection();
};
} // namespace Ship