#include "AdvancedResolution.h"

#include <libultraship/libultraship.h>
#include <graphic/Fast3D/gfx_pc.h>

namespace LUS {
void ApplyResolutionChanges() {
    ImVec2 size = ImGui::GetContentRegionAvail();

    const float aspectRatioX = CVarGetFloat("gAdvancedResolution_aspectRatioX", 16.0f);
    const float aspectRatioY = CVarGetFloat("gAdvancedResolution_aspectRatioY", 9.0f);
    const uint32_t verticalPixelCount = CVarGetInteger("gAdvancedResolution_verticalPixelCount", 480);
    const bool verticalResolutionToggle = CVarGetInteger("gAdvancedResolution_verticalResolutionToggle", (int)false);

    const bool aspectRatioIsEnabled = (aspectRatioX > 0.0f) && (aspectRatioY > 0.0f);

    const uint32_t minResolutionWidth = 320;
    const uint32_t minResolutionHeight = 240;
    const uint32_t maxResolutionWidth = 8096;  // the renderer's actual limit is 16384
    const uint32_t maxResolutionHeight = 4320; // on either axis. if you have the VRAM for it.
    uint32_t newWidth = gfx_current_dimensions.width;
    uint32_t newHeight = gfx_current_dimensions.height;

    if (verticalResolutionToggle) { // Use fixed vertical resolution
        if (aspectRatioIsEnabled) {
            newWidth = uint32_t(float(verticalPixelCount / aspectRatioY) * aspectRatioX);
        } else {
            newWidth = uint32_t(float(verticalPixelCount * size.x / size.y));
        }
        newHeight = verticalPixelCount;
    } else { // Use the window's resolution
        if (aspectRatioIsEnabled) {
            if (((float)gfx_current_game_window_viewport.width / gfx_current_game_window_viewport.height) >
                (aspectRatioX / aspectRatioY)) {
                // when pillarboxed
                newWidth = uint32_t(float(gfx_current_dimensions.height / aspectRatioY) * aspectRatioX);
            } else { // when letterboxed
                newHeight = uint32_t(float(gfx_current_dimensions.width / aspectRatioX) * aspectRatioY);
            }
        } // else, having both options turned off does nothing.
    }
    // clamp values to prevent renderer crash
    if (newWidth < minResolutionWidth) {
        newWidth = minResolutionWidth;
    }
    if (newHeight < minResolutionHeight) {
        newHeight = minResolutionHeight;
    }
    if (newWidth > maxResolutionWidth) {
        newWidth = maxResolutionWidth;
    }
    if (newHeight > maxResolutionHeight) {
        newHeight = maxResolutionHeight;
    }
    // apply new dimensions
    gfx_current_dimensions.width = newWidth;
    gfx_current_dimensions.height = newHeight;
    // centring the image is done in Gui::StartFrame().
}
} // namespace LUS
