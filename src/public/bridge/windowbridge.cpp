#include "public/bridge/windowbridge.h"
#include "window/Window.h"
#include "Context.h"

extern "C" {

uint32_t WindowGetWidth(void) {
    return Ship::Context::GetInstance()->GetWindow()->GetWidth();
}

uint32_t WindowGetHeight(void) {
    return Ship::Context::GetInstance()->GetWindow()->GetHeight();
}

float WindowGetAspectRatio(void) {
    return Ship::Context::GetInstance()->GetWindow()->GetCurrentAspectRatio();
}

bool WindowIsRunning(void) {
    return Ship::Context::GetInstance()->GetWindow()->IsRunning();
}

int32_t WindowGetPosX(void) {
    return Ship::Context::GetInstance()->GetWindow()->GetPosX();
}

int32_t WindowGetPosY(void) {
    return Ship::Context::GetInstance()->GetWindow()->GetPosY();
}

bool WindowIsFullscreen(void) {
    return Ship::Context::GetInstance()->GetWindow()->IsFullscreen();
}

/**
 * Widescreen functions
 *
 */

float ScreenGetAspectRatio() {
    return gfx_current_dimensions.aspect_ratio;
}

// AdjustXFromLeftEdge 
float LeftEdgeAlign(float v) {
    return (SCREEN_WIDTH / 2 - SCREEN_HEIGHT / 2 * OTRGetAspectRatio() + (v));
}

int16_t LeftEdgeRectAlign(float v) {
    return ((int) floorf(OTRGetDimensionFromLeftEdge(v)));
}

float RightEdgeAlign(float v) {
    return (SCREEN_WIDTH / 2 + SCREEN_HEIGHT / 2 * OTRGetAspectRatio() - (SCREEN_WIDTH - v));
}

int16_t RightEdgeRectAlign(float v) {
    return ((int) ceilf(OTRGetDimensionFromRightEdge(v)));
}

// Gets the width of the current render target area
uint32_t WindowGetGameRenderWidth() {
    return gfx_current_dimensions.width;
}

// Gets the height of the current render target area
uint32_t WindowGetGameRenderHeight() {
    return gfx_current_dimensions.height;
}

}
