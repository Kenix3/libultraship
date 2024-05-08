#pragma once

#include <memory>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "window/gui/Gui.h"

namespace Ship {
enum class WindowBackend { DX11, DX12, GLX_OPENGL, SDL_OPENGL, SDL_METAL, GX2, BACKEND_COUNT };

class Config;

class Window {
    friend class Context;

  public:
    Window();
    Window(std::vector<std::shared_ptr<GuiWindow>> guiWindows);
    ~Window();

    virtual void Init() = 0;
    virtual void Close() = 0;
    virtual void StartFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void SetCursorVisibility(bool visible) = 0;
    virtual uint32_t GetWidth() = 0;
    virtual uint32_t GetHeight() = 0;
    virtual int32_t GetPosX() = 0;
    virtual int32_t GetPosY() = 0;
    virtual uint32_t GetCurrentRefreshRate() = 0;
    virtual bool SupportsWindowedFullscreen() = 0;
    virtual bool CanDisableVerticalSync() = 0;
    virtual void SetResolutionMultiplier(float multiplier) = 0;
    virtual void SetMsaaLevel(uint32_t value) = 0;
    virtual void SetFullscreen(bool isFullscreen) = 0;
    virtual bool IsFullscreen() = 0;
    virtual bool IsRunning() = 0;
    virtual const char* GetKeyName(int32_t scancode) = 0;

    WindowBackend GetWindowBackend();
    std::shared_ptr<std::vector<WindowBackend>> GetAvailableWindowBackends();
    int32_t GetLastScancode();
    void SetLastScancode(int32_t scanCode);
    void ToggleFullscreen();
    float GetCurrentAspectRatio();
    void SaveWindowToConfig();
    std::shared_ptr<Gui> GetGui();

  protected:
    void SetWindowBackend(WindowBackend backend);
    void AddAvailableWindowBackend(WindowBackend backend);

  private:
    std::shared_ptr<Gui> mGui;
    int32_t mLastScancode;
    WindowBackend mWindowBackend;
    std::shared_ptr<std::vector<WindowBackend>> mAvailableWindowBackends;
    // Hold a reference to Config because Window has a Save function called on Context destructor, where the singleton
    // is no longer available.
    std::shared_ptr<Config> mConfig;
};
} // namespace Ship
