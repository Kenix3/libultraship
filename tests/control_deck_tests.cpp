#include <gtest/gtest.h>

#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {
namespace {

class TestControlDeck final : public ControlDeck {
  public:
    TestControlDeck(std::unordered_map<CONTROLLERBUTTONS_T, std::string> buttonNames,
                    std::shared_ptr<ConsoleVariable> consoleVariable)
        : ControlDeck({}, nullptr, std::move(buttonNames), nullptr, std::move(consoleVariable)) {
    }

    void WriteToPad(void*) override {
    }
};

TEST(ControlDeckTest, PreservesGameSpecificButtonNames) {
    constexpr CONTROLLERBUTTONS_T testButton = 0x4000;
    auto consoleVariables = std::make_shared<ConsoleVariable>();
    TestControlDeck controlDeck({ { testButton, "Test" } }, consoleVariables);

    EXPECT_EQ(controlDeck.GetAllButtonNames().at(testButton), "Test");
    EXPECT_EQ(controlDeck.GetButtonNameForBitmask(testButton), "Test");
}

} // namespace
} // namespace Ship
