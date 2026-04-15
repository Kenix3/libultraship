#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <spdlog/async.h>
#include "ship/audio/Audio.h"
#include "ship/Component.h"
#include "ship/Tickable.h"

namespace Fast {
class GfxDebugger;
}

namespace Ship {

class TickableComponent;

class Console;
class ConsoleVariable;
class ControlDeck;
class CrashHandler;
class Window;
class Config;
class ResourceManager;
class FileDropMgr;
class LoggerComponent;
class ThreadPoolComponent;
class EventSystem;
#ifndef DISABLE_SCRIPTING
class ScriptLoader;
#endif
class Keystore;

class Context : public Component, public Tickable {
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

    std::shared_ptr<EventSystem> GetEventSystem() const;
#ifndef DISABLE_SCRIPTING
    std::shared_ptr<ScriptLoader> GetScriptLoader() const;
#endif
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
    bool InitThreadPool(uint32_t reservedThreadCount = 1);
    bool InitEventSystem();
#ifndef DISABLE_SCRIPTING
    bool InitScriptLoader(std::unordered_map<std::string, std::string> compileDefines = {}, int codeVersion = 1,
                          std::string buildOptions = "-g -Wl", std::vector<std::string> includePaths = {},
                          std::vector<std::string> libraryPaths = {}, std::vector<std::string> libraries = {});
#endif
    bool InitKeystore();

    // ---- TickableComponent list ----
    // Access tickable components via this list directly.
    PartList<TickableComponent>& GetTickableComponents();
    const PartList<TickableComponent>& GetTickableComponents() const;
    Context& SortTickableComponents();

  protected:
    Context() = default;

  private:
    static std::weak_ptr<Context> mContext;

    std::shared_ptr<EventSystem> mEventSystem;
#ifndef DISABLE_SCRIPTING
    std::shared_ptr<ScriptLoader> mScriptLoader;
#endif
    std::shared_ptr<Keystore> mKeystore;


    std::string mConfigFilePath;
    std::string mMainPath;
    std::string mPatchesPath;

    std::string mName;
    std::string mShortName;

    PartList<TickableComponent> mTickableComponents;
    bool mIsTickableComponentsOrderStale = false;
    mutable std::mutex mTickableMutex;
};

} // namespace Ship
