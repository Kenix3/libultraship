#include <gtest/gtest.h>

#include "ship/controller/physicaldevice/ConnectedPhysicalDeviceManager.h"

namespace Ship {
namespace {

class ConnectedPhysicalDeviceManagerTest : public testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(SDL_InitSubSystem(SDL_INIT_GAMEPAD));

        SDL_VirtualJoystickDesc desc;
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.vendor_id = 0x1209;
        desc.product_id = 0x0001;
        desc.naxes = 2;
        desc.nbuttons = 1;
        desc.button_mask = 1 << SDL_GAMEPAD_BUTTON_SOUTH;
        desc.axis_mask = (1 << SDL_GAMEPAD_AXIS_LEFTX) | (1 << SDL_GAMEPAD_AXIS_LEFTY);
        desc.name = "LUS virtual gamepad";

        mInstanceId = SDL_AttachVirtualJoystick(&desc);
        ASSERT_NE(mInstanceId, 0u) << SDL_GetError();
    }

    void TearDown() override {
        if (mInstanceId != 0) {
            SDL_DetachVirtualJoystick(mInstanceId);
        }
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    }

    SDL_JoystickID mInstanceId = 0;
};

TEST_F(ConnectedPhysicalDeviceManagerTest, RefreshPreservesHandleForConnectedGamepad) {
    ConnectedPhysicalDeviceManager manager;

    manager.RefreshConnectedSDLGamepads();
    const auto firstScan = manager.GetConnectedSDLGamepadsForPort(0);
    ASSERT_TRUE(firstScan.contains(mInstanceId));
    SDL_Gamepad* const firstHandle = firstScan.at(mInstanceId);
    ASSERT_NE(firstHandle, nullptr);

    manager.RefreshConnectedSDLGamepads();
    const auto secondScan = manager.GetConnectedSDLGamepadsForPort(0);
    ASSERT_TRUE(secondScan.contains(mInstanceId));
    EXPECT_EQ(secondScan.at(mInstanceId), firstHandle);

    const SDL_JoystickID detachedInstanceId = mInstanceId;
    ASSERT_TRUE(SDL_DetachVirtualJoystick(detachedInstanceId));
    mInstanceId = 0;
    manager.RefreshConnectedSDLGamepads();
    EXPECT_FALSE(manager.GetConnectedSDLGamepadsForPort(0).contains(detachedInstanceId));
    EXPECT_EQ(SDL_GetGamepadFromID(detachedInstanceId), nullptr);
}

} // namespace
} // namespace Ship
