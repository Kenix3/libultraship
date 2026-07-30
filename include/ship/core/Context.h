#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <stdint.h>
#include <functional>
#include "ship/audio/Audio.h"
#include "ship/core/Component.h"
#include "ship/core/TickableList.h"

/**
 * @namespace Ship
 * @brief Core namespace for the libultraship engine framework.
 *
 * The Ship namespace contains all public types that form the libultraship
 * engine — a component-based framework for running N64 game re-implementations
 * on modern desktop and mobile hardware.
 *
 * Key subsystems inside Ship include:
 * - **Context** — the root singleton that owns every other component.
 * - **Resource** — asset loading, archive management, and caching.
 * - **Window / Gui** — platform-agnostic windowing, input, and ImGui overlay.
 * - **Audio** — mixing and playback through platform audio backends.
 * - **Controller** — unified controller input with configurable mappings.
 * - **Console** — developer console with console variables and commands.
 * - **Events** — a publish/subscribe event bus for decoupled communication.
 * - **Scripting** — optional dynamic library loading for game-specific plugins.
 *
 * Ship is intentionally platform-neutral; platform-specific code lives behind
 * compile-time feature guards (e.g. `ENABLE_DX11`, `ENABLE_OPENGL`) and is
 * abstracted by the subsystem interfaces listed above.
 */
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
 * - **FileDrop → child of Window**: file-drop events originate from the OS
 *   window; FileDrop has no reason to be a direct Context child.
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
     * @brief Creates and stores the global Context instance with the default set of components.
     *
     * This is the convenience factory that replicates the original initialization order:
     * Logging, Config, ConsoleVariables, ThreadPool, Keystore, ResourceManager, ControlDeck,
     * CrashHandler, Console, Window, Audio, Events, FileDrop,
     * and ScriptLoader (if enabled).

     * **All components are added to the hierarchy before any Init() is called.**
     * This ensures that every component can safely look up siblings during its own
     * Init() without requiring a specific add-order dependency.
     *
     * **Init-order dependencies within this factory:**
     * - ResourceManager::Init() — ThreadPool must be present (it self-initializes
     *   on construction, so it is always ready).
     * - Window construction/use — Config must be present and initialized before
     *   any Window code path reads persisted settings.
     * - Audio construction/use — Config must be present and initialized before
     *   any Audio code path reads or writes persisted backend settings.
     * - Config::Init(window) — Window must already be initialized before Config
     *   caches it for later validated use.
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
    static std::shared_ptr<Context> CreateInstance(const std::string& name, const std::string& shortName);

    /**
     * @brief Creates and stores the global Context instance with the given set of components.
     *
     * Each component in @p components is added via GetChildren().Add() and then initialized
     * by calling Component::Init() on it. Components should be provided in dependency order
     * (e.g. Config before Window).
     *
     * @param name           Human-readable application name.
     * @param shortName      Short application identifier.
     * @param configFilePath Path to the JSON configuration file.
     * @param components     Ordered list of components to add and initialize.
     * @return The newly created Context.
     */
    static std::shared_ptr<Context> CreateInstance(const std::string& name, const std::string& shortName,
                                                   std::vector<std::shared_ptr<Component>> components);

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
     */
    Context(std::string name, std::string shortName);
    ~Context();

    /** @brief Returns the short application identifier. */
    const std::string& GetShortName() const;

    // ---- TickableComponent list ----

    TickableList& GetTickableComponents();
    const TickableList& GetTickableComponents() const;

    /** @brief Returns elapsed time in seconds since this Context was initialized. */
    double GetElapsedTimeSeconds() const;

    /** @brief Recomputes elapsed time in seconds since this Context was initialized. */
    void UpdateElapsedTimeSeconds();

    /**
     * @brief Drives one frame of all registered TickableComponents for a specific EventID.
     *
     * Runs all tickables in list order for the provided EventID only.
     */
    void Tick(EventID eventId);

    /**
     * @brief Runs the default frame sequence: Update, LateUpdate, then Draw.
     *
     * Calls UpdateElapsedTimeSeconds() first, then dispatches Tick(Update),
     * Tick(LateUpdate), and Tick(Draw).
     */
    void Tick();

    /**
     * @brief Registers an optional callback that can install additional default components.
     */
    static void SetDefaultComponentInstaller(const std::function<void(const std::shared_ptr<Context>&)>& installer);

    /**
     * @brief Registers optional callbacks for bridge cache update and clear lifecycle hooks.
     */
    static void SetBridgeCacheHandlers(const std::function<void(const std::shared_ptr<Context>&)>& update,
                                       const std::function<void()>& clear);

  protected:
    Context() = default;

  private:
    std::string mShortName;
    std::chrono::steady_clock::time_point mInitTime{};
    double mElapsedTimeSeconds = 0.0;

    TickableList mTickableComponents;
#ifdef COMPONENT_THREAD_SAFE
    mutable std::mutex mTickableMutex;
#endif
};

} // namespace Ship
