#include "libultraship/bridge/gfxbridge.h"

#include "ship/Context.h"
#include "fast/Fast3dWindow.h"
#include "fast/interpreter.h"

// Dependency: requires Ship::Window (specifically Fast::Fast3dWindow) to be present in Ship::Context.

static std::shared_ptr<Fast::Fast3dWindow> sFast3dWindow;

static Fast::Fast3dWindow* GetFast3dWindow() {
    if (!sFast3dWindow) {
        sFast3dWindow = std::dynamic_pointer_cast<Fast::Fast3dWindow>(
            Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Window>());
    }
    return sFast3dWindow.get();
}

// Set the dimensions for the VI mode that the console would be using
// (Usually 320x240 for lo-res and 640x480 for hi-res)
extern "C" void GfxSetNativeDimensions(uint32_t width, uint32_t height) {
    Fast::Interpreter* gfx = GetFast3dWindow()->GetInterpreterWeak().lock().get();
    gfx->SetNativeDimensions(width, height);
}

extern "C" void GfxGetPixelDepthPrepare(float x, float y) {
    auto wnd = GetFast3dWindow();
    if (wnd == nullptr) {
        return;
    }
    return wnd->GetPixelDepthPrepare(x, y);
}

extern "C" uint16_t GfxGetPixelDepth(float x, float y) {
    auto wnd = GetFast3dWindow();
    if (wnd == nullptr) {
        return 0;
    }
    return wnd->GetPixelDepth(x, y);
}
