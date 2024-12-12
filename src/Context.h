#pragma once

#include <string>
#include <memory>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include "config/Config.h"
#include "resource/ResourceManager.h"
#include "controller/controldeck/ControlDeck.h"
#include "debug/CrashHandler.h"
#include "audio/Audio.h"
#include "window/Window.h"
#include "config/ConsoleVariable.h"
#include "debug/Console.h"
#include "graphic/Fast3D/debug/GfxDebugger.h"

namespace Ship {

class Context {
  public:
    static std::shared_ptr<Context> GetInstance();
    static std::shared_ptr<Context> CreateInstance(const std::string name, const std::string shortName,
                                                   const std::string configFilePath,
                                                   const std::vector<std::string>& archivePaths = {},
                                                   const std::unordered_set<uint32_t>& validHashes = {},
                                                   uint32_t reservedThreadCount = 1, AudioSettings audioSettings = {});
    static std::shared_ptr<Context> CreateUninitializedInstance(const std::string name, const std::string shortName,
                                                                const std::string configFilePath);
    static std::string GetAppBundlePath();
    static std::string GetAppDirectoryPath(std::string appName = "");
    static std::string GetPathRelativeToAppDirectory(const std::string path, std::string appName = "");
    static std::string GetPathRelativeToAppBundle(const std::string path);
    static std::string LocateFileAcrossAppDirs(const std::string path, std::string appName = "");

    Context(std::string name, std::string shortName, std::string configFilePath);
    ~Context();

    void Init(const std::vector<std::string>& archivePaths, const std::unordered_set<uint32_t>& validHashes,
              uint32_t reservedThreadCount, AudioSettings audioSettings);

    std::shared_ptr<spdlog::logger> GetLogger();
    std::shared_ptr<Config> GetConfig();
    std::shared_ptr<ConsoleVariable> GetConsoleVariables();
    std::shared_ptr<ResourceManager> GetResourceManager();
    std::shared_ptr<ControlDeck> GetControlDeck();
    std::shared_ptr<CrashHandler> GetCrashHandler();
    std::shared_ptr<Window> GetWindow();
    std::shared_ptr<Console> GetConsole();
    std::shared_ptr<Audio> GetAudio();
    std::shared_ptr<Fast::GfxDebugger> GetGfxDebugger();

    std::string GetName();
    std::string GetShortName();

    void InitLogging();
    void InitConfiguration();
    void InitConsoleVariables();
    void InitResourceManager(const std::vector<std::string>& archivePaths = {},
                             const std::unordered_set<uint32_t>& validHashes = {}, uint32_t reservedThreadCount = 1);
    void InitControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks = {});
    void InitCrashHandler();
    void InitAudio(AudioSettings settings);
    void InitGfxDebugger();
    void InitConsole();
    void InitWindow(std::vector<std::shared_ptr<GuiWindow>> guiWindows = {});

  protected:
    Context() = default;

  private:
    static std::weak_ptr<Context> mContext;

    std::shared_ptr<spdlog::logger> mLogger;
    std::shared_ptr<Config> mConfig;
    std::shared_ptr<ConsoleVariable> mConsoleVariables;
    std::shared_ptr<ResourceManager> mResourceManager;
    std::shared_ptr<ControlDeck> mControlDeck;
    std::shared_ptr<CrashHandler> mCrashHandler;
    std::shared_ptr<Window> mWindow;
    std::shared_ptr<Console> mConsole;
    std::shared_ptr<Audio> mAudio;
    std::shared_ptr<Fast::GfxDebugger> mGfxDebugger;

    std::string mConfigFilePath;
    std::string mMainPath;
    std::string mPatchesPath;

    std::string mName;
    std::string mShortName;
};
} // namespace Ship
