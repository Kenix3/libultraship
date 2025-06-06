#pragma once
#if defined(ENABLE_DX11) || defined(ENABLE_DX12)

#include "gfx_rendering_api.h"

#include <functional>

#include <dxgi1_2.h>

namespace Fast {

class GfxWindowBackendDXGI final : public GfxWindowBackend {
  public:
    GfxWindowBackendDXGI() = default;
    ~GfxWindowBackendDXGI() override;

    void Init(const char* gameName, const char* apiName, bool startFullScreen, uint32_t width, uint32_t height,
              int32_t posX, int32_t posY) override;
    void Close() override;
    void SetKeyboardCallbacks(bool (*onKeyDown)(int scancode), bool (*onKeyUp)(int scancode),
                              void (*onnAllKeysUp)()) override;
    void SetMouseCallbacks(bool (*onMouseButtonDown)(int btn), bool (*onnMouseButtonUp)(int btn)) override;
    void SetFullscreenChangedCallback(void (*onnFullscreenChanged)(bool is_now_fullscreen)) override;
    void SetFullscreen(bool fullscreen) override;
    void GetActiveWindowRefreshRate(uint32_t* refreshRate) override;
    void SetCursorVisability(bool visability) override;
    void SetMousePos(int32_t posX, int32_t posY) override;
    void GetMousePos(int32_t* x, int32_t* y) override;
    void GetMouseDelta(int32_t* x, int32_t* y) override;
    void GetMouseWheel(float* x, float* y) override;
    bool GetMouseState(uint32_t btn) override;
    void SetMouseCapture(bool capture) override;
    bool IsMouseCaptured() override;
    void GetDimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY) override;
    void HandleEvents() override;
    bool IsFrameReady() override;
    void SwapBuffersBegin() override;
    void SwapBuffersEnd() override;
    double GetTime() override;
    void SetTargetFPS(int fps) override;
    void SetMaxFrameLatency(int latency) override;
    const char* GetKeyName(int scancode) override;
    bool CanDisableVsync() override;
    bool IsRunning() override;
    void Destroy() override;
    bool IsFullscreen() override;

    HWND GetWindowHandle();
    IDXGISwapChain1* GetSwapChain();
    // These need to be public to be accessible in the window callback
    void CreateSwapChain(IUnknown* mDevice, std::function<void()>&& before_destroy_fn);
    void CreateFactoryAndDevice(bool debug, int d3d_version, class GfxRenderingAPIDX11* self,
                                bool (*createFunc)(class GfxRenderingAPIDX11* self, IDXGIAdapter1* adapter,
                                                   bool test_only));
    void OnKeydown(WPARAM wParam, LPARAM lParam);
    void OnKeyup(WPARAM wParam, LPARAM lParam);
    void OnMouseButtonDown(int btn);
    void OnMouseButtonUp(int btn);
    void HandleRawInputBuffered();
    void UpdateMousePrevPos();
    void ApplyMouseCaptureClip();

    std::tuple<HMONITOR, RECT, BOOL> mMonitor; // 0: Handle, 1: Display Monitor Rect, 2: Is_Primary
    uint32_t current_width, current_height;    // Width and height of client areas
    std::vector<std::tuple<HMONITOR, RECT, BOOL>> monitor_list;
    int32_t mPosX, mPosY; // Screen coordinates
    double mDetectedHz;
    double mDisplayPeriod; // (1000 / dxgi.mDetectedHz) in ms
    void (*mOnAllKeysUp)(void);
    POINT mRawMouseDeltaBuf;
    float mMouseWheel[2];
    bool mIsMouseCaptured;
    bool mIsMouseHovered;
    bool mInFocus;
    bool mHasMousePosition;

  private:
    void LoadDxgi();
    void ApplyMaxFrameLatency(bool first);
    void ToggleBorderlessWindowFullScreen(bool enable, bool callCallback);

    HWND h_wnd;
    // These four only apply in windowed mode.

    HMODULE dxgi_module;
    HRESULT(__stdcall* CreateDXGIFactory1)(REFIID riid, void** factory);
    HRESULT(__stdcall* CreateDXGIFactory2)(UINT flags, REFIID iid, void** factory);

    bool mLastMaximizedState;

    bool mDXGI11_4;
    Microsoft::WRL::ComPtr<IDXGIFactory2> mFactory;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain;
    HANDLE mWaitableObject;
    Microsoft::WRL::ComPtr<IUnknown> mSwapChainDevice; // D3D11 Device or D3D12 Command Queue
    std::function<void()> mBeforeDestroySwapChainFn;
    uint64_t mFrameTimeStamp; // in units of 1/FRAME_INTERVAL_NS_DENOMINATOR nanoseconds
    std::map<UINT, DXGI_FRAME_STATISTICS> mFrameStats;
    std::set<std::pair<UINT, UINT>> mPendingFrameStats;
    bool mDroppedFrame;
    bool mZeroLatency;
    UINT mLengthInVsyncFrames;
    uint32_t mMaxFrameLatency;
    uint32_t mAppliedMaxFrameLatency;
    HANDLE mTimer;
    bool mUseTimer;
    bool mTearingSupport;
    bool mMousePressed[5];
    LARGE_INTEGER mPreviousPresentTime;

    RAWINPUTDEVICE mRawInputDevice[1];
    POINT mPrevMouseCursorPos;
};

} // namespace Fast

#ifdef DECLARE_GFX_DXGI_FUNCTIONS
void ThrowIfFailed(HRESULT res);
void ThrowIfFailed(HRESULT res, HWND h_wnd, const char* message);
#endif

#endif
