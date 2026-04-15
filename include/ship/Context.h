#pragma once

#include <string>
#include <memory>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <spdlog/async.h>
#include "ship/audio/Audio.h"

namespace spdlog {
class logger;
}

namespace Fast {
class GfxDebugger;
}

namespace Ship {

class Console;
class ConsoleVariable;
class ControlDeck;
class CrashHandler;
class Window;
class Config;
class ResourceManager;
class FileDropMgr;
class EventSystem;
class ScriptLoader;
class Keystore;

class Context {
  public:
    static std::shared_ptr<Context> GetInstance();
    static std::shared_ptr<Context> CreateInstance(const std::string& name, const std::string& shortName,
                                                   const std::string& configFilePath,
                                                   const std::vector<std::string>& archivePaths = {},
                                                   const std::unordered_set<uint32_t>& validHashes = {},
                                                   uint32_t reservedThreadCount = 1, AudioSettings audioSettings = {},
                                                   std::shared_ptr<Window> window = nullptr,
                                                   std::shared_ptr<ControlDeck> controlDeck = nullptr);
    static std::shared_ptr<Context> CreateUninitializedInstance(const std::string& name, const std::string& shortName,
                                                                const std::string& configFilePath);
    static std::string GetAppBundlePath();
    static std::string GetAppDirectoryPath(const std::string& appName = "");
    static std::string GetPathRelativeToAppDirectory(const std::string& path, const std::string& appName = "");
    static std::string GetPathRelativeToAppBundle(const std::string& path);
    static std::string LocateFileAcrossAppDirs(const std::string& path, const std::string& appName = "");

    Context(std::string name, std::string shortName, std::string configFilePath);
    ~Context();

    bool Init(const std::vector<std::string>& archivePaths, const std::unordered_set<uint32_t>& validHashes,
              uint32_t reservedThreadCount, AudioSettings audioSettings, std::shared_ptr<Window> window = nullptr,
              std::shared_ptr<ControlDeck> controlDeck = nullptr);

    std::shared_ptr<spdlog::logger> GetLogger() const;
    std::shared_ptr<Config> GetConfig() const;
    std::shared_ptr<ConsoleVariable> GetConsoleVariables() const;
    std::shared_ptr<ResourceManager> GetResourceManager() const;
    std::shared_ptr<ControlDeck> GetControlDeck() const;
    std::shared_ptr<CrashHandler> GetCrashHandler() const;
    std::shared_ptr<Window> GetWindow() const;
    std::shared_ptr<Console> GetConsole() const;
    std::shared_ptr<Audio> GetAudio() const;
    std::shared_ptr<Fast::GfxDebugger> GetGfxDebugger() const;
    std::shared_ptr<FileDropMgr> GetFileDropMgr() const;
    std::shared_ptr<EventSystem> GetEventSystem() const;
    std::shared_ptr<ScriptLoader> GetScriptLoader() const;
    std::shared_ptr<Keystore> GetKeystore() const;

    std::string GetName() const;
    std::string GetShortName() const;

    bool InitLogging(spdlog::level::level_enum debugBuildLogLevel = spdlog::level::debug,
                     spdlog::level::level_enum releaseBuildLogLevel = spdlog::level::warn);
    bool InitConfiguration();
    bool InitConsoleVariables();
    bool InitResourceManager(const std::vector<std::string>& archivePaths = {},
                             const std::unordered_set<uint32_t>& validHashes = {}, uint32_t reservedThreadCount = 1,
                             const bool allowEmptyPaths = false);
    bool InitControlDeck(std::shared_ptr<ControlDeck> controlDeck = nullptr);
    bool InitCrashHandler();
    bool InitAudio(AudioSettings settings);
    bool InitGfxDebugger();
    bool InitConsole();
    bool InitWindow(std::shared_ptr<Window> window = nullptr);
    bool InitFileDropMgr();
    bool InitEventSystem();
    bool InitScriptLoader(std::unordered_map<std::string, std::string> compileDefines = {}, int codeVersion = 1,
                          std::string buildOptions = "-g -Wl", std::vector<std::string> includePaths = {},
                          std::vector<std::string> libraryPaths = {}, std::vector<std::string> libraries = {});
    bool InitKeystore();

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
    std::shared_ptr<FileDropMgr> mFileDropMgr;
    std::shared_ptr<EventSystem> mEventSystem;
    std::shared_ptr<ScriptLoader> mScriptLoader;
    std::shared_ptr<Keystore> mKeystore;

    std::string mConfigFilePath;
    std::string mMainPath;
    std::string mPatchesPath;

    std::string mName;
    std::string mShortName;
};
} // namespace Ship
