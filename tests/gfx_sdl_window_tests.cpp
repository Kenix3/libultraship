#include <gtest/gtest.h>

#include <SDL3/SDL.h>

#include "fast/backends/gfx_sdl.h"

#if defined(ENABLE_OPENGL) && !defined(__APPLE__)

namespace Fast {
namespace {

class GfxWindowBackendSDLTest : public testing::Test {
  protected:
    void SetUp() override {
        ASSERT_TRUE(SDL_InitSubSystem(SDL_INIT_EVENTS));
        SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    }

    void TearDown() override {
        SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
        SDL_QuitSubSystem(SDL_INIT_EVENTS);
    }
};

TEST_F(GfxWindowBackendSDLTest, HandleEventsLeavesGamepadLifecycleEventsQueued) {
    constexpr SDL_EventType lifecycleEvents[] = {
        SDL_EVENT_GAMEPAD_ADDED,
        SDL_EVENT_GAMEPAD_REMOVED,
        SDL_EVENT_GAMEPAD_REMAPPED,
    };

    for (const SDL_EventType type : lifecycleEvents) {
        SDL_Event event{};
        event.type = type;
        ASSERT_TRUE(SDL_PushEvent(&event));
    }

    GfxWindowBackendSDL2 backend;
    backend.HandleEvents();

    for (const SDL_EventType type : lifecycleEvents) {
        EXPECT_TRUE(SDL_HasEvent(type));
    }
}

} // namespace
} // namespace Fast

#endif
