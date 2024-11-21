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
 * Widescreen alignment functions
 *
 * These are used in gDPFillWideRectangle() and gSPWideTextureRectangle(),
 * or any situation where a screen coordinate outside the normal N64 bounds of 320x240 are required.
 *
 * How to use:
 * LeftEdgeAlign(0) --> Returns the left most edge of the game render target area
 * LeftEdgeAlign(320) --> Returns the right most edge of the game render target area
 * RightEdgeAlign(320) --> Returns the right most edge of the game render target area
 * 
 * Align a rectangle with the left of the screen:
 * gDPFillWideRectangle(displayListHead++, TranslateXRelativeToLeftEdge(ulx), uly, TranslateXRelativeToRightEdge(lrx), lry);
 *
 * Align a texture rectangle with the left of the screen (Note the `<< 2` is required):
 * gSPWideTextureRectangle(gDisplayListHead++, TranslateXRelativeToLeftEdge(ulx) << 2, yl, TranslateXRelativeToRightEdge(lrx) << 2, yh, G_TX_RENDERTILE, arg4 << 5, (arg5 << 5), 4 << 10, 1 << 10);
 *
 * UI Elements may be stickied to the left or right of the screen with a relatively simple check:
 * // Calculate the center of the UI element and check if it's on the left or right half of the screen.
 * if ( ( ulx - ( width / 2 ) ) < ( SCREEN_WIDTH / 2 ) ) {
 *   gDPFillWideRectangle(displayListHead++, TranslateXRelativeToLeftEdge(ulx), uly, TranslateXRelativeToLeftEdge(lrx), lry); // Align left
 * } else {
 *   gDPFillWideRectangle(displayListHead++, TranslateXRelativeToRightEdge(ulx), uly, TranslateXRelativeToRightEdge(lrx), lry); // Align right
 * }
 */

float ScreenGetAspectRatio() {
    return gfx_current_dimensions.aspect_ratio;
}

// AdjustXFromLeftEdge 
float TranslateXRelativeToLeftEdge(float v) {
    return (SCREEN_WIDTH / 2 - SCREEN_HEIGHT / 2 * OTRGetAspectRatio() + (v));
}

int16_t TranslateRectXRelativeToLeftEdge(float v) {
    return ((int) floorf(OTRGetDimensionFromLeftEdge(v)));
}

float TranslateXRelativeToRightEdge(float v) {
    return (SCREEN_WIDTH / 2 + SCREEN_HEIGHT / 2 * OTRGetAspectRatio() - (SCREEN_WIDTH - v));
}

int16_t TranslateRectXRelativeToRightEdge(float v) {
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
