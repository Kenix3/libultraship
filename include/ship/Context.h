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

namespace Ship {

class TickableComponent;

/**
 * @brief Central singleton context for the libultraship engine.
 *
 * Context is the root Component that owns all subsystems as children. Consumers
 * should create their own components and pass them in via AddChild(). Use
 * CreateDefaultInstance() for a default set of components matching the original
 * initialization order.
 *
 * Subsystems are retrieved via GetChildren().GetFirst<T>().
 */
class Context : public Component, public Tickable {
  public:
    static std::shared_ptr<Context> GetInstance();

    /**
     * @brief Creates and stores the global Context instance with the default set of components.
     *
     * This is the convenience factory that replicates the original initialization order:
     * Logging, Config, ConsoleVariables, ThreadPool, ResourceManager, ControlDeck,
     * CrashHandler, Console, Window, Audio, GfxDebugger, EventSystem, FileDropMgr,
     * ScriptLoader (if enabled), and Keystore.
     */
    static std::shared_ptr<Context>
    CreateDefaultInstance(const std::string& name, const std::string& shortName, const std::string& configFilePath,
                          const std::vector<std::string>& archivePaths = {},
                          const std::unordered_set<uint32_t>& validHashes = {}, uint32_t reservedThreadCount = 1,
                          AudioSettings audioSettings = {}, std::shared_ptr<Component> window = nullptr,
                          std::shared_ptr<Component> controlDeck = nullptr);

    /**
     * @brief Creates and stores the global Context instance without adding any default components.
     *
     * Consumers should add their own components via AddChild() after creation.
     */
    static std::shared_ptr<Context> CreateInstance(const std::string& name, const std::string& shortName,
                                                   const std::string& configFilePath);

    static std::string GetAppBundlePath();
    static std::string GetAppDirectoryPath(const std::string& appName = "");
    static std::string GetPathRelativeToAppDirectory(const std::string& path, const std::string& appName = "");
    static std::string GetPathRelativeToAppBundle(const std::string& path);
    static std::string LocateFileAcrossAppDirs(const std::string& path, const std::string& appName = "");

    Context(std::string name, std::string shortName, std::string configFilePath);
    ~Context();

    std::string GetName() const;
    std::string GetShortName() const;
    std::string GetConfigFilePath() const;

    // ---- TickableComponent list ----
    PartList<TickableComponent>& GetTickableComponents();
    const PartList<TickableComponent>& GetTickableComponents() const;
    Context& SortTickableComponents();

  protected:
    Context() = default;

  private:
    static std::weak_ptr<Context> mContext;

    std::string mConfigFilePath;
    std::string mName;
    std::string mShortName;

    PartList<TickableComponent> mTickableComponents;
    bool mIsTickableComponentsOrderStale = false;
    mutable std::mutex mTickableMutex;
};

} // namespace Ship
