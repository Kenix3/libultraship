#pragma once

#include <string>
#include <memory>
#include <mutex>
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

    bool HasTickableComponent(std::shared_ptr<TickableComponent> tickableComponent);
    size_t CountTickableComponent();
    bool AddTickableComponent(std::shared_ptr<TickableComponent> tickableComponent, const bool force = false);
    bool RemoveTickableComponent(std::shared_ptr<TickableComponent> tickableComponent, const bool force = false);
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>> GetTickableComponents();
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>> GetTickableComponents();
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>>
    GetTickableComponents(const std::vector<std::string>& componentNames);
    template <typename T>
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>>
    GetTickableComponents(const std::vector<std::string>& componentNames);
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>>
    GetTickableComponents(const std::string& componentName);
    template <typename T>
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>>
    GetTickableComponents(const std::string& componentName);
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>>
    GetTickableComponent(const std::vector<int32_t>& componentIds);
    std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>> GetTickableComponent(const int32_t componentId);

    Context& SetTickableComponentsOrderStale();
    Context& UnsetTickableComponentsOrderStale();
    Context& SortTickableComponents();

    virtual bool CanAddTickableComponent(std::shared_ptr<TickableComponent> tickableComponent);
    virtual bool CanRemoveTickableComponent(std::shared_ptr<TickableComponent> tickableComponent);
    virtual void AddedTickableComponent(std::shared_ptr<TickableComponent> tickableComponent, const bool forced);
    virtual void RemovedTickableComponent(std::shared_ptr<TickableComponent> tickableComponent, const bool forced);

  protected:
    Context() = default;

  private:
    static std::weak_ptr<Context> mContext;

    std::shared_ptr<spdlog::logger> mLogger;

    std::string mConfigFilePath;
    std::string mMainPath;
    std::string mPatchesPath;

    std::string mName;
    std::string mShortName;

    std::vector<std::shared_ptr<TickableComponent>> mTickableComponents;
    bool mIsTickableComponentsOrderStale = false;
    mutable std::mutex mTickableMutex;
};

} // namespace Ship

// Template method implementations — requires TickableComponent to be complete.
#include "ship/TickableComponent.h"

namespace Ship {

template <typename T>
std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>> Context::GetTickableComponents() {
    const std::lock_guard<std::mutex> lock(mTickableMutex);
    auto result = std::make_shared<std::vector<std::shared_ptr<TickableComponent>>>();
    for (const auto& tc : mTickableComponents) {
        if (std::dynamic_pointer_cast<T>(tc)) {
            result->push_back(tc);
        }
    }
    return result;
}

template <typename T>
std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>>
Context::GetTickableComponents(const std::vector<std::string>& componentNames) {
    const std::lock_guard<std::mutex> lock(mTickableMutex);
    auto result = std::make_shared<std::vector<std::shared_ptr<TickableComponent>>>();
    for (const auto& tc : mTickableComponents) {
        if (std::dynamic_pointer_cast<T>(tc) &&
            std::find(componentNames.begin(), componentNames.end(), tc->GetName()) != componentNames.end()) {
            result->push_back(tc);
        }
    }
    return result;
}

template <typename T>
std::shared_ptr<std::vector<std::shared_ptr<TickableComponent>>>
Context::GetTickableComponents(const std::string& componentName) {
    const std::lock_guard<std::mutex> lock(mTickableMutex);
    auto result = std::make_shared<std::vector<std::shared_ptr<TickableComponent>>>();
    for (const auto& tc : mTickableComponents) {
        if (std::dynamic_pointer_cast<T>(tc) && tc->GetName() == componentName) {
            result->push_back(tc);
        }
    }
    return result;
}

} // namespace Ship
