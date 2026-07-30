#include "libultraship/bridge/gfxbridge.h"

#include "fast/Fast3dWindow.h"
#include "fast/interpreter.h"
#include <atomic>

// Dependency: requires Ship::Window (specifically Fast::Fast3dWindow) to be present in Ship::Context.

static std::atomic<std::shared_ptr<Fast::Fast3dWindow>> sFast3dWindow;

void GfxSetFast3dWindow(std::shared_ptr<Fast::Fast3dWindow> window) {
    sFast3dWindow.store(std::move(window), std::memory_order_release);
}

std::shared_ptr<Fast::Fast3dWindow> GfxGetFast3dWindow() {
    return sFast3dWindow.load(std::memory_order_acquire);
}

// Set the dimensions for the VI mode that the console would be using
// (Usually 320x240 for lo-res and 640x480 for hi-res)
extern "C" void GfxSetNativeDimensions(uint32_t width, uint32_t height) {
    auto wnd = GfxGetFast3dWindow();
    if (wnd == nullptr) {
        return;
    }

    auto gfx = wnd->GetInterpreterWeak().lock();
    if (gfx == nullptr) {
        return;
    }

    gfx->SetNativeDimensions(width, height);
}

extern "C" void GfxGetPixelDepthPrepare(float x, float y) {
    auto wnd = GfxGetFast3dWindow();
    if (wnd == nullptr) {
        return;
    }
    return wnd->GetPixelDepthPrepare(x, y);
}

extern "C" uint16_t GfxGetPixelDepth(float x, float y) {
    auto wnd = GfxGetFast3dWindow();
    if (wnd == nullptr) {
        return 0;
    }
    return wnd->GetPixelDepth(x, y);
}
