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

/**
 * @brief Central singleton context for the libultraship engine.
 *
 * Context owns and provides access to every major subsystem (resource management,
 * window, audio, controller, logging, configuration, scripting, etc.). Exactly one
 * Context should be alive at a time; use GetInstance() to retrieve it after creation.
 *
 * Typical usage:
 * @code
 * auto ctx = Ship::Context::CreateInstance("MyApp", "app", "config.json", archivePaths);
 * // ... use ctx->GetResourceManager(), ctx->GetWindow(), etc.
 * @endcode
 */
class Context {
  public:
    /**
     * @brief Returns the currently active global Context instance.
     * @return Shared pointer to the Context, or an empty pointer if none exists.
     */
    static std::shared_ptr<Context> GetInstance();

    /**
     * @brief Creates, initializes, and stores the global Context instance.
     *
     * Convenience factory that calls CreateUninitializedInstance() followed by Init().
     *
     * @param name              Human-readable application name.
     * @param shortName         Short application identifier used for paths and config keys.
     * @param configFilePath    Path to the JSON configuration file.
     * @param archivePaths      List of archive file or directory paths to mount.
     * @param validHashes       Set of acceptable game-version hash values (empty = all allowed).
     * @param reservedThreadCount Number of threads to keep free from the resource thread-pool.
     * @param audioSettings     Initial audio backend and channel configuration.
     * @param window            Optional pre-constructed Window to use; if nullptr a default is created.
     * @param controlDeck       Optional pre-constructed ControlDeck; if nullptr a default is created.
     * @return Shared pointer to the fully initialized Context.
     */
    static std::shared_ptr<Context> CreateInstance(const std::string name, const std::string shortName,
                                                   const std::string configFilePath,
                                                   const std::vector<std::string>& archivePaths = {},
                                                   const std::unordered_set<uint32_t>& validHashes = {},
                                                   uint32_t reservedThreadCount = 1, AudioSettings audioSettings = {},
                                                   std::shared_ptr<Window> window = nullptr,
                                                   std::shared_ptr<ControlDeck> controlDeck = nullptr);

    /**
     * @brief Creates a Context that has not yet been initialized.
     *
     * Use this when you need finer control over initialization order; call Init() manually afterwards.
     *
     * @param name           Human-readable application name.
     * @param shortName      Short application identifier.
     * @param configFilePath Path to the JSON configuration file.
     * @return Shared pointer to the uninitialized Context.
     */
    static std::shared_ptr<Context> CreateUninitializedInstance(const std::string name, const std::string shortName,
                                                                const std::string configFilePath);

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
    static std::string GetAppDirectoryPath(std::string appName = "");

    /**
     * @brief Resolves a path relative to the application data directory.
     * @param path    Relative path to resolve.
     * @param appName Override the application name used to build the base path.
     * @return Absolute path string.
     */
    static std::string GetPathRelativeToAppDirectory(const std::string path, std::string appName = "");

    /**
     * @brief Resolves a path relative to the application bundle directory.
     * @param path Relative path to resolve.
     * @return Absolute path string.
     */
    static std::string GetPathRelativeToAppBundle(const std::string path);

    /**
     * @brief Searches common application directories for a file and returns its absolute path.
     * @param path    Filename or relative path to locate.
     * @param appName Override the application name used to search.
     * @return Absolute path to the first match found, or an empty string if not found.
     */
    static std::string LocateFileAcrossAppDirs(const std::string path, std::string appName = "");

    /**
     * @brief Constructs a Context with the given identifiers but does not initialize subsystems.
     * @param name           Human-readable application name.
     * @param shortName      Short application identifier.
     * @param configFilePath Path to the JSON configuration file.
     */
    Context(std::string name, std::string shortName, std::string configFilePath);
    ~Context();

    /**
     * @brief Initializes all subsystems in the correct order.
     *
     * Called automatically by CreateInstance(). When using CreateUninitializedInstance(),
     * call this method manually after any custom pre-initialization setup.
     *
     * @param archivePaths        List of archive paths to mount.
     * @param validHashes         Acceptable game-version hashes.
     * @param reservedThreadCount Threads to reserve outside the resource pool.
     * @param audioSettings       Audio configuration.
     * @param window              Optional Window override.
     * @param controlDeck         Optional ControlDeck override.
     * @return true on success, false if any subsystem failed to initialize.
     */
    bool Init(const std::vector<std::string>& archivePaths, const std::unordered_set<uint32_t>& validHashes,
              uint32_t reservedThreadCount, AudioSettings audioSettings, std::shared_ptr<Window> window = nullptr,
              std::shared_ptr<ControlDeck> controlDeck = nullptr);

    /** @brief Returns the application-wide spdlog logger. */
    std::shared_ptr<spdlog::logger> GetLogger();
    /** @brief Returns the Config subsystem. */
    std::shared_ptr<Config> GetConfig();
    /** @brief Returns the ConsoleVariable subsystem (CVars). */
    std::shared_ptr<ConsoleVariable> GetConsoleVariables();
    /** @brief Returns the ResourceManager subsystem. */
    std::shared_ptr<ResourceManager> GetResourceManager();
    /** @brief Returns the ControlDeck subsystem. */
    std::shared_ptr<ControlDeck> GetControlDeck();
    /** @brief Returns the CrashHandler subsystem. */
    std::shared_ptr<CrashHandler> GetCrashHandler();
    /** @brief Returns the Window subsystem. */
    std::shared_ptr<Window> GetWindow();
    /** @brief Returns the developer Console subsystem. */
    std::shared_ptr<Console> GetConsole();
    /** @brief Returns the Audio subsystem. */
    std::shared_ptr<Audio> GetAudio();
    /** @brief Returns the graphics debugger. */
    std::shared_ptr<Fast::GfxDebugger> GetGfxDebugger();
    /** @brief Returns the FileDropMgr subsystem for handling drag-and-drop file events. */
    std::shared_ptr<FileDropMgr> GetFileDropMgr();
    /** @brief Returns the EventSystem subsystem. */
    std::shared_ptr<EventSystem> GetEventSystem();
    /** @brief Returns the ScriptLoader subsystem. */
    std::shared_ptr<ScriptLoader> GetScriptLoader();
    /** @brief Returns the Keystore subsystem used for archive signature verification. */
    std::shared_ptr<Keystore> GetKeystore();

    /** @brief Returns the human-readable application name. */
    std::string GetName();
    /** @brief Returns the short application identifier. */
    std::string GetShortName();

    /**
     * @brief Initializes the spdlog logging backend.
     * @param debugBuildLogLevel   Log level used for debug builds.
     * @param releaseBuildLogLevel Log level used for release builds.
     * @return true on success.
     */
    bool InitLogging(spdlog::level::level_enum debugBuildLogLevel = spdlog::level::debug,
                     spdlog::level::level_enum releaseBuildLogLevel = spdlog::level::warn);

    /** @brief Initializes the Config subsystem, loading the config file from disk. */
    bool InitConfiguration();

    /** @brief Initializes the ConsoleVariable (CVar) subsystem and loads persisted values. */
    bool InitConsoleVariables();

    /**
     * @brief Initializes the ResourceManager and mounts the given archives.
     * @param archivePaths      Paths to archives or directories to mount.
     * @param validHashes       Acceptable game-version hashes; empty means all are valid.
     * @param reservedThreadCount Number of threads reserved outside the resource pool.
     * @param allowEmptyPaths   If true, initialization succeeds even when archivePaths is empty.
     * @return true on success.
     */
    bool InitResourceManager(const std::vector<std::string>& archivePaths = {},
                             const std::unordered_set<uint32_t>& validHashes = {}, uint32_t reservedThreadCount = 1,
                             const bool allowEmptyPaths = false);

    /**
     * @brief Initializes the ControlDeck subsystem.
     * @param controlDeck Optional pre-constructed ControlDeck; if nullptr a default is created.
     * @return true on success.
     */
    bool InitControlDeck(std::shared_ptr<ControlDeck> controlDeck = nullptr);

    /** @brief Initializes the CrashHandler subsystem. @return true on success. */
    bool InitCrashHandler();

    /**
     * @brief Initializes the Audio subsystem with the given settings.
     * @param settings Audio backend and channel configuration.
     * @return true on success.
     */
    bool InitAudio(AudioSettings settings);

    /** @brief Initializes the graphics debugger. @return true on success. */
    bool InitGfxDebugger();

    /** @brief Initializes the developer Console window. @return true on success. */
    bool InitConsole();

    /**
     * @brief Initializes the Window subsystem.
     * @param window Optional pre-constructed Window; if nullptr a default backend is selected.
     * @return true on success.
     */
    bool InitWindow(std::shared_ptr<Window> window = nullptr);

    /** @brief Initializes the FileDropMgr subsystem. @return true on success. */
    bool InitFileDropMgr();

    /** @brief Initializes the EventSystem subsystem. @return true on success. */
    bool InitEventSystem();

    /**
     * @brief Initializes the ScriptLoader subsystem for runtime script compilation.
     * @param compileDefines  Preprocessor defines passed to the compiler.
     * @param codeVersion     Version tag embedded in compiled modules.
     * @param buildOptions    Raw compiler flags string.
     * @param includePaths    Additional include directories.
     * @param libraryPaths    Additional library search directories.
     * @param libraries       Libraries to link against compiled scripts.
     * @return true on success.
     */
    bool InitScriptLoader(std::unordered_map<std::string, std::string> compileDefines = {}, int codeVersion = 1,
                          std::string buildOptions = "-g -Wl", std::vector<std::string> includePaths = {},
                          std::vector<std::string> libraryPaths = {}, std::vector<std::string> libraries = {});

    /** @brief Initializes the Keystore used for verifying signed archives. @return true on success. */
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
