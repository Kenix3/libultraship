#include "gfxbridge.h"

#include "Context.h"
#include "graphic/Fast3D/Fast3dWindow.h"
#include "graphic/Fast3D/interpreter.h"

// Set the dimensions for the VI mode that the console would be using
// (Usually 320x240 for lo-res and 640x480 for hi-res)
extern "C" void GfxSetNativeDimensions(uint32_t width, uint32_t height) {
    Fast::Interpreter* gfx = static_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow())
                                 ->GetInterpreterWeak()
                                 .lock()
                                 .get();
    gfx->SetNativeDimensions(width, height);
}

extern "C" void GfxGetPixelDepthPrepare(float x, float y) {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
    if (wnd == nullptr) {
        return;
    }
    return wnd->GetPixelDepthPrepare(x, y);
}

extern "C" uint16_t GfxGetPixelDepth(float x, float y) {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
    if (wnd == nullptr) {
        return 0;
    }
    return wnd->GetPixelDepth(x, y);
}
