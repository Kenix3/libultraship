#include <gtest/gtest.h>

#include "ship/controller/controldevice/controller/mapping/factories/RumbleMappingFactory.h"

namespace Ship {
namespace {

TEST(RumbleMappingFactoryTest, CreatesDefaultMappingOnlyForSDLGamepads) {
    EXPECT_TRUE(RumbleMappingFactory::CreateDefaultSDLRumbleMappings(PhysicalDeviceType::Keyboard, 0, nullptr, nullptr)
                    .empty());
    EXPECT_TRUE(
        RumbleMappingFactory::CreateDefaultSDLRumbleMappings(PhysicalDeviceType::Mouse, 0, nullptr, nullptr).empty());

    const auto mappings =
        RumbleMappingFactory::CreateDefaultSDLRumbleMappings(PhysicalDeviceType::SDLGamepad, 0, nullptr, nullptr);
    ASSERT_EQ(mappings.size(), 1);
    EXPECT_EQ(mappings.front()->GetPhysicalDeviceType(), PhysicalDeviceType::SDLGamepad);
}

} // namespace
} // namespace Ship
