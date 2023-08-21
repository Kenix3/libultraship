#include "AdvancedResolution.h"

#include <libultraship/libultraship.h>
#include <graphic/Fast3D/gfx_pc.h>

namespace LUS {
void ApplyResolutionChanges() {
    ImVec2 size = ImGui::GetContentRegionAvail();

    const float aspectRatio_X = CVarGetFloat("gAdvancedResolution_aspectRatio_X", 16.0f);
    const float aspectRatio_Y = CVarGetFloat("gAdvancedResolution_aspectRatio_Y", 9.0f);
    const uint32_t verticalPixelCount = CVarGetInteger("gAdvancedResolution_verticalPixelCount", 480);
    const bool verticalResolutionToggle = CVarGetInteger("gAdvancedResolution_verticalResolutionToggle", (int)false);

    const bool aspectRatioIsEnabled = (aspectRatio_X > 0.0f) && (aspectRatio_Y > 0.0f);

    const uint32_t min_ResolutionWidth = 320;
    const uint32_t min_ResolutionHeight = 240;
    const uint32_t max_ResolutionWidth = 8096;  // the renderer's actual limit is 16384
    const uint32_t max_ResolutionHeight = 4320; // on either axis. if you have the VRAM for it.
    uint32_t newWidth = gfx_current_dimensions.width;
    uint32_t newHeight = gfx_current_dimensions.height;

    if (verticalResolutionToggle) { // Use fixed vertical resolution
        if (aspectRatioIsEnabled) {
            newWidth = uint32_t(float(verticalPixelCount / aspectRatio_Y) * aspectRatio_X);
        } else {
            newWidth = uint32_t(float(verticalPixelCount * size.x / size.y));
        }
        newHeight = verticalPixelCount;
    } else { // Use the window's resolution
        if (aspectRatioIsEnabled) {
            if (((float)gfx_current_game_window_viewport.width / gfx_current_game_window_viewport.height) >
                (aspectRatio_X / aspectRatio_Y)) {
                // when pillarboxed
                newWidth = uint32_t(float(gfx_current_dimensions.height / aspectRatio_Y) * aspectRatio_X);
            } else { // when letterboxed
                newHeight = uint32_t(float(gfx_current_dimensions.width / aspectRatio_X) * aspectRatio_Y);
            }
        } // else, having both options turned off does nothing.
    }
    // clamp values to prevent renderer crash
    if (newWidth < min_ResolutionWidth)
        newWidth = min_ResolutionWidth;
    if (newHeight < min_ResolutionHeight)
        newHeight = min_ResolutionHeight;
    if (newWidth > max_ResolutionWidth)
        newWidth = max_ResolutionWidth;
    if (newHeight > max_ResolutionHeight)
        newHeight = max_ResolutionHeight;
    // apply new dimensions
    gfx_current_dimensions.width = newWidth;
    gfx_current_dimensions.height = newHeight;
    // centring the image is done in Gui::StartFrame().
}
} // namespace LUS
