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
#include "ship/TickableList.h"

namespace Ship {

/**
 * @brief Central singleton context for the libultraship engine.
 *
 * Context is the root Component that owns all subsystems as children. Consumers
 * should create their own components and pass them in via GetChildren().Add(). Use
 * CreateDefaultInstance() for a default set of components matching the original
 * initialization order.
 *
 * Subsystems are retrieved via GetChildren().GetFirst<T>().
 *
 * **Suggested future hierarchy changes (not yet implemented):**
 * The following reorganizations would better reflect logical ownership and reduce
 * cross-component dependencies. Implementing them requires migrating call sites:
 * - **FileDropMgr → child of Window**: file-drop events originate from the OS
 *   window; FileDropMgr has no reason to be a direct Context child.
 * - **ControlDeck → child of Window**: game input is driven by and scoped to
 *   the active window surface. Moving it under Window makes the ownership clear.
 * - **Audio → child of Window**: audio is part of the game presentation layer
 *   and logically belongs alongside the window.
 * - **Console → child of Window**: the developer console is rendered inside
 *   the GUI layer (ConsoleWindow); placing Console under Window co-locates it
 *   with its presentation layer.
 */
class Context : public Component {
  public:
    /**
     * @brief Returns the currently active global Context instance.
     * @return Shared pointer to the Context, or an empty pointer if none exists.
     */
    static std::shared_ptr<Context> GetInstance();

    /**
     * @brief Creates and stores the global Context instance with the default set of components.
     *
     * This is the convenience factory that replicates the original initialization order:
     * Logging, Config, ConsoleVariables, ThreadPool, Keystore, ResourceManager, ControlDeck,
     * CrashHandler, Console, Window, Audio, GfxDebugger, Events, FileDropMgr,
     * and ScriptLoader (if enabled).
     *
     * **All components are added to the hierarchy before any Init() is called.**
     * This ensures that every component can safely look up siblings during its own
     * Init() without requiring a specific add-order dependency.
     *
     * **Init-order dependencies within this factory:**
     * - ResourceManager::Init() — ThreadPoolComponent must be present (it self-initializes
     *   on construction, so it is always ready).
     * - Window::OnInit() — Config must be present and initialized (Config self-initializes
     *   on construction).
     * - Audio::OnInit() — Config must be present and initialized (same as above).
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
     * Consumers should add their own components via GetChildren().Add() after creation.
     */
    static std::shared_ptr<Context> CreateInstance(const std::string& name, const std::string& shortName,
                                                   const std::string& configFilePath);

    /**
     * @brief Returns the platform-specific application bundle directory (e.g. the .app bundle on macOS).
     * @return Absolute path string, or an empty string on platforms without the concept of a bundle.
     */
    static std::string GetAppBundlePath();
    /**
     * @brief Returns the platform-specific directory where the application stores its data.
     * @param appName Override the application name used to build the path; defaults to the current app name.
     * @return Absolute path string.
     */
    static std::string GetAppDirectoryPath(const std::string& appName = "");
    /**
     * @brief Resolves a path relative to the application data directory.
     * @param path    Relative path to resolve.
     * @param appName Override the application name used to build the base path.
     * @return Absolute path string.
     */
    static std::string GetPathRelativeToAppDirectory(const std::string& path, const std::string& appName = "");
    /**
     * @brief Resolves a path relative to the application bundle directory.
     * @param path Relative path to resolve.
     * @return Absolute path string.
     */
    static std::string GetPathRelativeToAppBundle(const std::string& path);
    /**
     * @brief Searches common application directories for a file and returns its absolute path.
     * @param path    Filename or relative path to locate.
     * @param appName Override the application name used to search.
     * @return Absolute path to the first match found, or an empty string if not found.
     */
    static std::string LocateFileAcrossAppDirs(const std::string& path, const std::string& appName = "");

    /**
     * @brief Constructs a Context with the given identifiers but does not initialize subsystems.
     * @param name           Human-readable application name.
     * @param shortName      Short application identifier.
     * @param configFilePath Path to the JSON configuration file.
     */
    Context(std::string name, std::string shortName, std::string configFilePath);
    ~Context();

    /** @brief Returns the human-readable application name. */
    std::string GetName() const;
    /** @brief Returns the short application identifier. */
    std::string GetShortName() const;
    /** @brief Returns the path to the JSON configuration file. */
    std::string GetConfigFilePath() const;

    // ---- TickableComponent list ----
    TickableList& GetTickableComponents();
    const TickableList& GetTickableComponents() const;

  protected:
    Context() = default;

  private:
    static std::weak_ptr<Context> mContext;

    bool mOwnsLogger = false;
    std::string mConfigFilePath;
    std::string mName;
    std::string mShortName;

    TickableList mTickableComponents;
    mutable std::mutex mTickableMutex;
};

} // namespace Ship
