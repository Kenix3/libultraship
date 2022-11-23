#pragma once

#include <memory>
#include <filesystem>
#include <unordered_set>
#include <spdlog/spdlog.h>
#include "controller/ControlDeck.h"
#include "core/ConsoleVariable.h"
#include "audio/AudioPlayer.h"
#include "graphic/Fast3D/gfx_window_manager_api.h"
#include "Mercury.h"

struct GfxRenderingAPI;
struct GfxWindowManagerAPI;

namespace Ship {
class ResourceMgr;

class Window {
  public:
    static std::shared_ptr<Window> GetInstance();
    static std::shared_ptr<Window> CreateInstance(const std::string name, const std::vector<std::string>& otrFiles = {},
                                                  const std::unordered_set<uint32_t>& validHashes = {});
    static std::string GetAppDirectoryPath();
    static std::string GetPathRelativeToAppDirectory(const char* path);

    Window(std::string Name);
    ~Window();
    void WriteSaveFile(const std::filesystem::path& savePath, uintptr_t addr, void* dramAddr, size_t size);
    void ReadSaveFile(std::filesystem::path savePath, uintptr_t addr, void* dramAddr, size_t size);
    void CreateDefaults();
    void MainLoop(void (*MainFunction)(void));
    void Initialize(const std::vector<std::string>& otrFiles = {},
                    const std::unordered_set<uint32_t>& validHashes = {});
    void StartFrame();
    void SetTargetFps(int32_t fps);
    void SetMaximumFrameLatency(int32_t latency);
    void GetPixelDepthPrepare(float x, float y);
    uint16_t GetPixelDepth(float x, float y);
    void ToggleFullscreen();
    void SetFullscreen(bool isFullscreen);
    void SetCursorVisibility(bool visible);
    uint32_t GetCurrentWidth();
    uint32_t GetCurrentHeight();
    float GetCurrentAspectRatio();
    bool IsFullscreen();
    uint32_t GetMenuBar();
    void SetMenuBar(uint32_t menuBar);
    std::string GetName();
    std::shared_ptr<ControlDeck> GetControlDeck();
    std::shared_ptr<AudioPlayer> GetAudioPlayer();
    std::shared_ptr<ResourceMgr> GetResourceManager();
    std::shared_ptr<Mercury> GetConfig();
    std::shared_ptr<spdlog::logger> GetLogger();
    std::shared_ptr<ConsoleVariable> GetConsoleVariables();
    const char* GetKeyName(int32_t scancode);
    int32_t GetLastScancode();
    void SetLastScancode(int32_t scanCode);
    void InitializeAudioPlayer(std::string_view audioBackend);
    void InitializeWindowManager(std::string_view gfxBackend);

  protected:
    Window() = default;

  private:
    static bool KeyDown(int32_t scancode);
    static bool KeyUp(int32_t scancode);
    static void AllKeysUp(void);
    static void OnFullscreenChanged(bool isNowFullscreen);
    static std::weak_ptr<Window> mContext;

    void InitializeConsoleVariables();
    void InitializeConfiguration();
    void InitializeControlDeck();
    void InitializeLogging();
    void InitializeResourceManager(const std::vector<std::string>& otrFiles = {},
                                   const std::unordered_set<uint32_t>& validHashes = {});

    std::shared_ptr<spdlog::logger> mLogger;
    std::shared_ptr<Mercury> mConfig;
    std::shared_ptr<ResourceMgr> mResourceManager;
    std::shared_ptr<AudioPlayer> mAudioPlayer;
    std::shared_ptr<ControlDeck> mControlDeck;
    std::shared_ptr<ConsoleVariable> mConsoleVariables;

    std::string mGfxBackend;
    std::string mAudioBackend;
    GfxRenderingAPI* mRenderingApi;
    GfxWindowManagerAPI* mWindowManagerApi;
    bool mIsFullscreen;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mMenuBar;
    int32_t mLastScancode;
    std::string mName;
    std::string mMainPath;
    std::string mBasePath;
    std::string mPatchesPath;
};
} // namespace Ship
