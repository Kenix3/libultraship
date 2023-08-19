// Date created: 2023 08 15
// Description: Advanced Resolution Editor - LUS GFX Functions
#include "advancedResolution.h"

#include <libultraship/libultraship.h>
#include <graphic/Fast3D/gfx_pc.h>

namespace AdvancedResolutionSettings {
    /*
    void InitResolutionCvars() {
        CVarSetFloat("gInternalResolution", 1.0f);
        CVarSetInteger("gLowResMode", 0);
    }*/

    void ApplyResolutionChanges() {
        ImVec2 size = ImGui::GetContentRegionAvail(); // size value, same as it's generated in Gui.cpp

        const float aspectRatio_X            = CVarGetFloat("gAdvancedResolution_aspectRatio_X", 16.0f);
        const float aspectRatio_Y            = CVarGetFloat("gAdvancedResolution_aspectRatio_Y", 9.0f);
        const int   verticalPixelCount       = CVarGetInteger("gAdvancedResolution_verticalPixelCount", 480);
        const bool  verticalResolutionToggle =
            CVarGetInteger("gAdvancedResolution_verticalResolutionToggle", (int)false);

        const bool aspectRatioIsEnabled = (aspectRatio_X > 0.0f) && (aspectRatio_Y > 0.0f);

        if (verticalResolutionToggle) { // Use fixed vertical resolution
            uint32_t myHorizontalPixelCount;
            if (aspectRatioIsEnabled) {
                myHorizontalPixelCount = uint32_t(float(verticalPixelCount / aspectRatio_Y) * aspectRatio_X);
            } else {
                myHorizontalPixelCount = uint32_t(float(verticalPixelCount * size.x / size.y));
            }
            gfx_current_dimensions.width = myHorizontalPixelCount;
            gfx_current_dimensions.height = verticalPixelCount;
        } else { // Use the graphics window's vertical resolution
        if (aspectRatioIsEnabled) {
                if ((float(gfx_current_game_window_viewport.width) / gfx_current_game_window_viewport.height) >
                (aspectRatio_X / aspectRatio_Y)) { // when pillarboxed
                gfx_current_dimensions.width = uint32_t(float(gfx_current_dimensions.height / aspectRatio_Y) * aspectRatio_X);
            } else { // when letterboxed
                gfx_current_dimensions.height = uint32_t(float(gfx_current_dimensions.width / aspectRatio_X) * aspectRatio_Y);
            }
        } // else, having both options turned off does nothing.
        }
        // centring the image is done in Gui::StartFrame().
    }
}