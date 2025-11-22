#pragma once

#include <string>
#include <memory>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include "ship/audio/Audio.h"
#include "ship/Component.h"
#include "ship/Tickable.h"

namespace spdlog {
class logger;
}

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

class Context : public Component, public Tickable {
  public:
    static std::shared_ptr<Context> GetInstance();
    static std::shared_ptr<Context> CreateInstance(const std::string name, const std::string shortName,
                                                   const std::string configFilePath,
                                                   const std::vector<std::string>& archivePaths = {},
                                                   const std::unordered_set<uint32_t>& validHashes = {},
                                                   uint32_t reservedThreadCount = 1, AudioSettings audioSettings = {},
                                                   std::shared_ptr<Window> window = nullptr,
                                                   std::shared_ptr<ControlDeck> controlDeck = nullptr);
    static std::shared_ptr<Context> CreateUninitializedInstance(const std::string name, const std::string shortName,
                                                                const std::string configFilePath);
    static std::string GetAppBundlePath();
    static std::string GetAppDirectoryPath(std::string appName = "");
    static std::string GetPathRelativeToAppDirectory(const std::string path, std::string appName = "");
    static std::string GetPathRelativeToAppBundle(const std::string path);
    static std::string LocateFileAcrossAppDirs(const std::string path, std::string appName = "");

    Context(std::string name, std::string shortName, std::string configFilePath);
    ~Context();

    bool Init(const std::vector<std::string>& archivePaths, const std::unordered_set<uint32_t>& validHashes,
              uint32_t reservedThreadCount, AudioSettings audioSettings, std::shared_ptr<Window> window = nullptr,
              std::shared_ptr<ControlDeck> controlDeck = nullptr);

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
    std::shared_ptr<FileDropMgr> GetFileDropMgr();

    std::string GetName();
    std::string GetShortName();

    bool InitLogging();
    bool InitConfiguration();
    bool InitConsoleVariables();
    bool InitResourceManager(const std::vector<std::string>& archivePaths = {},
                             const std::unordered_set<uint32_t>& validHashes = {}, uint32_t reservedThreadCount = 1);
    bool InitControlDeck(std::shared_ptr<ControlDeck> controlDeck = nullptr);
    bool InitCrashHandler();
    bool InitAudio(AudioSettings settings);
    bool InitGfxDebugger();
    bool InitConsole();
    bool InitWindow(std::shared_ptr<Window> window = nullptr);
    bool InitFileDropMgr();


    // TODO: NEw stuff below here
    bool HasTickableComponent(std::shared_ptr<TickableComponent> tickableComponent);
    size_t CountTickableComponent();
    bool AddTickableComponent(std::shared_ptr<TickableComponent> tickableComponent, const bool force = false);
    bool RemoveTickableComponent(std::shared_ptr<TickableComponent> tickableComponent, const bool force = false);
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>> GetTickableComponents();
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>> GetTickableComponents();
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>>
    GetTickableComponents(const std::vector<std::string>& names);
    template <typename T>
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>>
    GetTickableComponents(const std::vector<std::string>& names);
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>> GetTickableComponents(const std::string& name);
    template <typename T>
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>> GetTickableComponents(const std::string& name);
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>>
    GetTickableComponent(const std::vector<int32_t>& ids);
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>> GetTickableComponent(const int32_t id);

    Context& SetTickableComponentsOrderStale();
    Context& UnsetTickableComponentsOrderStale();
    Context& SortTickableComponents();

    bool CanAddTickableComponent(std::shared_ptr<TickableComponent> tickableComponent);
    bool CanRemoveTickableComponent(std::shared_ptr<TickableComponent> tickableComponent);
    void AddedTickableComponent(std::shared_ptr<TickableComponent> tickableComponent, const bool forced);
    void RemovedTickableComponent(std::shared_ptr<TickableComponent> tickableComponent, const bool forced);

    std::vector<std::shared_ptr<TickableComponent>> mTickableComponents;
    bool mIsTickableComponentsOrderStale;
#ifdef INCLUDE_MUTEX
    std::mutex mMutex;
#endif
    // TODO: END NEW!



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

    std::string mConfigFilePath;
    std::string mMainPath;
    std::string mPatchesPath;

    std::string mName;
    std::string mShortName;
};
} // namespace Ship
