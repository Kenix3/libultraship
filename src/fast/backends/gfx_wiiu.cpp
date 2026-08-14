#ifdef __WIIU__

#include "fast/backends/gfx_wiiu.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <malloc.h>

#include <coreinit/memory.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/time.h>
#include <gx2/display.h>
#include <gx2/mem.h>
#include <gx2/state.h>
#include <gx2/swap.h>
#include <proc_ui/memory.h>
#include <proc_ui/procui.h>
#include <whb/proc.h>
#include <SDL2/SDL.h>

#include "ship/Context.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/controller/physicaldevice/ConnectedPhysicalDeviceManager.h"
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"
#include "port/wiiu/WiiUImpl.h"

namespace Fast {
namespace {
GfxWindowBackendWiiU* sBackend = nullptr;

uint32_t OnForegroundAcquired(void*) {
    if (sBackend != nullptr) {
        sBackend->AcquireForeground();
    }
    return 0;
}

uint32_t OnForegroundReleased(void*) {
    if (sBackend != nullptr) {
        sBackend->ReleaseForeground();
    }
    return 0;
}
} // namespace

void GfxWiiUSetContextState() {
    if (sBackend != nullptr) sBackend->SetContextState();
}

void GfxWindowBackendWiiU::Init(const char*, const char*, bool, uint32_t, uint32_t, int32_t, int32_t) {
    assert(!mInitialized);
    sBackend = this;
    WHBProcInit();
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR);

    uint32_t mem1Address = 0;
    uint32_t mem1Size = 0;
    OSGetMemBound(OS_MEM1, &mem1Address, &mem1Size);
    mMem1Storage = memalign(0x40, mem1Size);
    assert(mMem1Storage != nullptr);
    ProcUISetMEM1Storage(mMem1Storage, mem1Size);

    mCommandBuffer = memalign(GX2_COMMAND_BUFFER_ALIGNMENT, 0x400000);
    assert(mCommandBuffer != nullptr);
    uint32_t initAttributes[] = { GX2_INIT_CMD_BUF_BASE, (uintptr_t)mCommandBuffer, GX2_INIT_CMD_BUF_POOL_SIZE,
                                  0x400000, GX2_INIT_ARGC, 0, GX2_INIT_ARGV, 0, GX2_INIT_END };
    GX2Init(initAttributes);

    switch (GX2GetSystemTVScanMode()) {
        case GX2_TV_SCAN_MODE_480I:
        case GX2_TV_SCAN_MODE_480P:
            mTvRenderMode = GX2_TV_RENDER_MODE_WIDE_480P;
            break;
        case GX2_TV_SCAN_MODE_1080I:
        case GX2_TV_SCAN_MODE_1080P:
            mTvRenderMode = GX2_TV_RENDER_MODE_WIDE_1080P;
            break;
        default:
            break;
    }
    mDrcRenderMode = GX2GetSystemDRCScanMode();
    uint32_t unused = 0;
    GX2CalcTVSize(mTvRenderMode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE, &mTvScanBufferSize,
                  &unused);
    GX2CalcDRCSize(mDrcRenderMode, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE,
                   &mDrcScanBufferSize, &unused);

    ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, OnForegroundAcquired, nullptr, 100);
    ProcUIRegisterCallback(PROCUI_CALLBACK_RELEASE, OnForegroundReleased, nullptr, 100);
    AcquireForeground();

    mContextState = static_cast<GX2ContextState*>(memalign(GX2_CONTEXT_STATE_ALIGNMENT, sizeof(GX2ContextState)));
    assert(mContextState != nullptr);
    GX2SetupContextStateEx(mContextState, TRUE);
    SetContextState();
    GX2SetTVScale(WIIU_DEFAULT_FB_WIDTH, WIIU_DEFAULT_FB_HEIGHT);
    GX2SetDRCScale(WIIU_DEFAULT_FB_WIDTH, WIIU_DEFAULT_FB_HEIGHT);
    GX2SetSwapInterval(mFrameDivisor);
    Ship::GuiWindowInitData guiWindow{};
    guiWindow.Gx2 = { WIIU_DEFAULT_FB_WIDTH, WIIU_DEFAULT_FB_HEIGHT };
    guiWindow.Backend = WindowBackend::FAST3D_WIIU_GX2;
    Ship::Context::GetInstance()->GetWindow()->GetGui()->Init(guiWindow);
    Ship::Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager()->RefreshConnectedSDLGamepads();
    mInitialized = true;
}

void GfxWindowBackendWiiU::AcquireForeground() {
    if (mForeground) return;
    mTvScanBuffer = memalign(GX2_SCAN_BUFFER_ALIGNMENT, mTvScanBufferSize);
    mDrcScanBuffer = memalign(GX2_SCAN_BUFFER_ALIGNMENT, mDrcScanBufferSize);
    assert(mTvScanBuffer != nullptr && mDrcScanBuffer != nullptr);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU, mTvScanBuffer, mTvScanBufferSize);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU, mDrcScanBuffer, mDrcScanBufferSize);
    GX2SetTVBuffer(mTvScanBuffer, mTvScanBufferSize, mTvRenderMode,
                   GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE);
    GX2SetDRCBuffer(mDrcScanBuffer, mDrcScanBufferSize, mDrcRenderMode,
                    GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_BUFFERING_MODE_DOUBLE);
    mForeground = true;
}

void GfxWindowBackendWiiU::ReleaseForeground() {
    if (!mForeground) return;
    free(mTvScanBuffer);
    free(mDrcScanBuffer);
    mTvScanBuffer = nullptr;
    mDrcScanBuffer = nullptr;
    mForeground = false;
}

void GfxWindowBackendWiiU::Close() { mIsRunning = false; }
void GfxWindowBackendWiiU::SetKeyboardCallbacks(bool (*down)(int), bool (*up)(int), void (*allUp)()) { mOnKeyDown = down; mOnKeyUp = up; mOnAllKeysUp = allUp; }
void GfxWindowBackendWiiU::SetMouseCallbacks(bool (*down)(int), bool (*up)(int)) { mOnMouseButtonDown = down; mOnMouseButtonUp = up; }
void GfxWindowBackendWiiU::SetFullscreenChangedCallback(void (*callback)(bool)) { mOnFullscreenChanged = callback; }
void GfxWindowBackendWiiU::SetFullscreen(bool) {}
void GfxWindowBackendWiiU::GetActiveWindowRefreshRate(uint32_t* rate) { *rate = 60; }
void GfxWindowBackendWiiU::SetCursorVisibility(bool) {}
void GfxWindowBackendWiiU::SetMousePos(int32_t, int32_t) {}
void GfxWindowBackendWiiU::GetMousePos(int32_t* x, int32_t* y) { *x = 0; *y = 0; }
void GfxWindowBackendWiiU::GetMouseDelta(int32_t* x, int32_t* y) { *x = 0; *y = 0; }
void GfxWindowBackendWiiU::GetMouseWheel(float* x, float* y) { *x = 0; *y = 0; }
bool GfxWindowBackendWiiU::GetMouseState(uint32_t) { return false; }
void GfxWindowBackendWiiU::SetMouseCapture(bool) {}
bool GfxWindowBackendWiiU::IsMouseCaptured() { return false; }
void GfxWindowBackendWiiU::GetDimensions(uint32_t* width, uint32_t* height, int32_t* x, int32_t* y) { *width = WIIU_DEFAULT_FB_WIDTH; *height = WIIU_DEFAULT_FB_HEIGHT; *x = 0; *y = 0; }
void GfxWindowBackendWiiU::HandleEvents() {
    SDL_PumpEvents();
    mIsRunning = WHBProcIsRunning();
}
bool GfxWindowBackendWiiU::IsFrameReady() { return mForeground && mIsRunning; }
void GfxWindowBackendWiiU::SwapBuffersBegin() { GX2SwapScanBuffers(); GX2Flush(); SetContextState(); GX2SetTVEnable(TRUE); GX2SetDRCEnable(TRUE); }
void GfxWindowBackendWiiU::SwapBuffersEnd() {}
double GfxWindowBackendWiiU::GetTime() { return OSTicksToSeconds(OSGetTime()); }
int GfxWindowBackendWiiU::GetTargetFps() { return mTargetFps; }
void GfxWindowBackendWiiU::SetTargetFps(int fps) { mTargetFps = fps; mFrameDivisor = std::max(1, 60 / std::max(1, fps)); GX2SetSwapInterval(mFrameDivisor); }
void GfxWindowBackendWiiU::SetMaxFrameLatency(int) {}
const char* GfxWindowBackendWiiU::GetKeyName(int) { return ""; }
bool GfxWindowBackendWiiU::CanDisableVsync() { return false; }
bool GfxWindowBackendWiiU::IsRunning() { return mIsRunning; }
void GfxWindowBackendWiiU::Destroy() { Close(); ReleaseForeground(); if (mContextState) free(mContextState); mContextState = nullptr; if (mCommandBuffer) free(mCommandBuffer); mCommandBuffer = nullptr; if (mInitialized) GX2Shutdown(); mInitialized = false; ProcUISetMEM1Storage(nullptr, 0); if (mMem1Storage) free(mMem1Storage); mMem1Storage = nullptr; SDL_Quit(); Ship::WiiU::Exit(); WHBProcShutdown(); sBackend = nullptr; }
bool GfxWindowBackendWiiU::IsFullscreen() { return true; }
void GfxWindowBackendWiiU::SetContextState() { if (mContextState) GX2SetContextState(mContextState); }
bool GfxWindowBackendWiiU::HasForeground() const { return mForeground; }

} // namespace Fast

#endif
