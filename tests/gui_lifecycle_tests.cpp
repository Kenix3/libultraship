#include <gtest/gtest.h>

#include "ship/window/gui/Gui.h"

#include <imgui.h>

namespace Ship {
namespace {

TEST(GuiLifecycleTest, ShutdownWithoutContextIsSafe) {
    ASSERT_EQ(ImGui::GetCurrentContext(), nullptr);
    Gui gui;

    gui.ShutDownImGui(nullptr);

    EXPECT_EQ(ImGui::GetCurrentContext(), nullptr);
}

} // namespace
} // namespace Ship
