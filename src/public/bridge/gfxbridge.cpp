#include "gfxbridge.h"

#include "graphic/Fast3D/gfx_pc.h"

// Set the dimensions for the VI mode that the console would be using
// (Usually 320x240 for lo-res and 640x480 for hi-res)
extern "C" void GfxSetNativeDimensions(uint32_t width, uint32_t height) {
    gfx_native_dimensions.width = width;
    gfx_native_dimensions.height = height;
}
