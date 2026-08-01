#include <gtest/gtest.h>

#include <string>

#include "ship/controller/physicaldevice/ConnectedPhysicalDeviceManager.h"

namespace Ship {
namespace {

class ConnectedPhysicalDeviceManagerTest : public testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(SDL_InitSubSystem(SDL_INIT_GAMEPAD));

        SDL_VirtualJoystickDesc desc;
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_UNKNOWN;
        desc.vendor_id = 0x1209;
        desc.product_id = 0x0001;
        desc.naxes = 2;
        desc.nbuttons = 1;
        desc.name = "LUS virtual gamepad";

        mInstanceId = SDL_AttachVirtualJoystick(&desc);
        ASSERT_NE(mInstanceId, 0u) << SDL_GetError();
        ASSERT_TRUE(SetMapping("LUS virtual gamepad")) << SDL_GetError();
        ASSERT_TRUE(SDL_IsGamepad(mInstanceId));
    }

    void TearDown() override {
        if (mInstanceId != 0) {
            SDL_DetachVirtualJoystick(mInstanceId);
        }
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    }

    SDL_JoystickID mInstanceId = 0;

    bool SetMapping(const char* name) const {
        char guid[33] = "";
        SDL_GUIDToString(SDL_GetJoystickGUIDForID(mInstanceId), guid, sizeof(guid));
        const std::string mapping = std::string(guid) + "," + name + ",a:b0,leftx:a0,lefty:a1,";
        return SDL_SetGamepadMapping(mInstanceId, mapping.c_str());
    }
};

TEST_F(ConnectedPhysicalDeviceManagerTest, RefreshPreservesHandleAndUpdatesNameForConnectedGamepad) {
    ConnectedPhysicalDeviceManager manager;

    manager.RefreshConnectedSDLGamepads();
    const auto firstScan = manager.GetConnectedSDLGamepadsForPort(0);
    ASSERT_TRUE(firstScan.contains(mInstanceId));
    SDL_Gamepad* const firstHandle = firstScan.at(mInstanceId);
    ASSERT_NE(firstHandle, nullptr);

    ASSERT_TRUE(SetMapping("Remapped LUS virtual gamepad")) << SDL_GetError();
    manager.RefreshConnectedSDLGamepads();
    const auto secondScan = manager.GetConnectedSDLGamepadsForPort(0);
    ASSERT_TRUE(secondScan.contains(mInstanceId));
    EXPECT_EQ(secondScan.at(mInstanceId), firstHandle);
    EXPECT_EQ(manager.GetConnectedSDLGamepadNames().at(mInstanceId), "Remapped LUS virtual gamepad");
}

TEST_F(ConnectedPhysicalDeviceManagerTest, RefreshClosesHandleWhenJoystickIsDetached) {
    ConnectedPhysicalDeviceManager manager;

    manager.RefreshConnectedSDLGamepads();
    ASSERT_TRUE(manager.GetConnectedSDLGamepadsForPort(0).contains(mInstanceId));
    ASSERT_NE(SDL_GetGamepadFromID(mInstanceId), nullptr);

    const SDL_JoystickID detachedInstanceId = mInstanceId;
    ASSERT_TRUE(SDL_DetachVirtualJoystick(detachedInstanceId));
    mInstanceId = 0;
    manager.RefreshConnectedSDLGamepads();
    EXPECT_FALSE(manager.GetConnectedSDLGamepadsForPort(0).contains(detachedInstanceId));
    EXPECT_EQ(SDL_GetGamepadFromID(detachedInstanceId), nullptr);
}

} // namespace
} // namespace Ship
