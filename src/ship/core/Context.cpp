#include "ship/core/Context.h"
#include "ship/core/TickableComponent.h"
#include <cstring>
#include <iostream>
#include <algorithm>
#include <queue>
#include "ship/install_config.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/debug/Console.h"
#include "ship/debug/CrashHandler.h"
#include "ship/resource/ResourceManager.h"
#include "ship/window/FileDrop.h"
#include "ship/log/Logger.h"
#include "ship/thread/ThreadPool.h"
#include "ship/events/Events.h"
#ifdef ENABLE_SCRIPTING
#include "ship/scripting/ScriptLoader.h"
#include "ship/security/Keystore.h"
#endif

namespace Ship {
namespace {
using ContextHook = std::function<void(const std::shared_ptr<Context>&)>;

ContextHook& GetDefaultComponentInstallerHook() {
    static ContextHook hook;
    return hook;
}

ContextHook& GetBridgeCacheUpdateHook() {
    static ContextHook hook;
    return hook;
}

std::function<void()>& GetBridgeCacheClearHook() {
    static std::function<void()> hook;
    return hook;
}
} // namespace

// Release all bridge-cache shared_ptrs so components are not kept alive past this
// context's lifetime by the bridge statics. If bridge statics were the only extra
// reference, the components will be destroyed here (during normal stack unwinding)
// rather than during process-exit static destruction where logging infrastructure
// may already have been torn down.

Context::~Context() {
    auto& clearHook = GetBridgeCacheClearHook();
    if (clearHook) {
        clearHook();
    }

    if (spdlog::default_logger()) {
        SPDLOG_TRACE("destruct context");
    }
    auto window = GetChildren().GetFirst<Window>();
    if (window) {
        window->SaveWindowToConfig();
    }

    auto config = GetChildren().GetFirst<Config>();

#ifdef ENABLE_SCRIPTING
    auto scriptLoader = GetChildren().GetFirst<ScriptLoader>();
    if (scriptLoader) {
        scriptLoader->UnloadAll();
    }
#endif

    // Tear down all children before logging shuts down. The Logger is (intentionally)
    // the first child added in CreateDefaultInstance, so a plain Remove(true) would
    // destroy it first and shut spdlog down while the remaining components' destructors
    // are still running (several of them log). Hold the Logger aside, remove everything
    // else, then remove the Logger last so it — and spdlog — outlive the other teardown.
    auto logger = GetChildren().GetFirst<Logger>();
    if (logger) {
        GetChildren().Remove(logger, true);
    }
    GetChildren().Remove(true);

    if (config) {
        config->Save();
    }

    // Finally drop the Logger, which shuts spdlog down, after everything else is gone.
    logger.reset();
    // spdlog shutdown is now owned by the Logger component which was destroyed above.
}

std::shared_ptr<Context> Context::CreateDefaultInstance(const std::string& name, const std::string& shortName,
                                                        const std::string& configFilePath,
                                                        const std::vector<std::string>& archivePaths,
                                                        const std::unordered_set<uint32_t>& validHashes,
                                                        uint32_t reservedThreadCount, AudioSettings audioSettings,
                                                        std::shared_ptr<Component> window,
                                                        std::shared_ptr<Component> controlDeck) {
    auto shared = std::make_shared<Context>(name, shortName);
    // The Context is the root of the hierarchy; set itself as its own Context so that
    // all children added later can find it via GetContext().
    shared->SetContext(shared);

    // ---- Logging ----
    try {
        auto logPath = GetPathRelativeToAppDirectory("logs/" + name + ".log");
        auto logger = std::make_shared<Logger>(name, logPath);
        shared->GetChildren().Add(logger);
        logger->Init();
    } catch (const std::exception& ex) {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
        return nullptr;
    }

    // ---- Configuration ----
    auto config = std::make_shared<Config>(GetPathRelativeToAppDirectory(configFilePath),
                                           std::dynamic_pointer_cast<Window>(window));
    shared->GetChildren().Add(config);

    // Read config values needed for component construction
    auto mainPath = config->GetString("Game.Main Archive", GetAppDirectoryPath());
    auto patchesPath = config->GetString("Game.Patches Archive", GetAppDirectoryPath() + "/mods");
    size_t threadCount = std::max(1, (int32_t)(std::thread::hardware_concurrency() - reservedThreadCount - 1));

    // ---- Console Variables ----
    shared->GetChildren().Add(std::make_shared<ConsoleVariable>(config));

    // ---- Thread Pool ----
    auto threadPool = std::make_shared<ThreadPool>(threadCount);
    shared->GetChildren().Add(threadPool);

#ifdef ENABLE_SCRIPTING
    // ---- Keystore ----
    auto keystore = std::make_shared<Keystore>(config);
    shared->GetChildren().Add(keystore);
#endif

    // ---- Resource Manager ----
    auto resourceManager =
#ifdef ENABLE_SCRIPTING
        std::make_shared<ResourceManager>(threadPool, keystore);
#else
        std::make_shared<ResourceManager>(threadPool);
#endif
    shared->GetChildren().Add(resourceManager);

    // ---- Control Deck ----
    if (controlDeck != nullptr) {
        shared->GetChildren().Add(controlDeck);
    } else {
        SPDLOG_ERROR("Failed to initialize control deck");
        return nullptr;
    }

    // ---- Crash Handler ----
    shared->GetChildren().Add(std::make_shared<CrashHandler>());

    // ---- Console ----
    auto console = std::make_shared<Console>();
    shared->GetChildren().Add(console);

    // ---- Window ----
    if (window == nullptr) {
        SPDLOG_ERROR("Failed to initialize window");
        return nullptr;
    }
    shared->GetChildren().Add(window);

    // ---- Audio ----
    auto audio = std::make_shared<Audio>(audioSettings, config);
    shared->GetChildren().Add(audio);

    auto& installerHook = GetDefaultComponentInstallerHook();
    if (installerHook) {
        installerHook(shared);
    }

    // ---- Events ----
    shared->GetChildren().Add(std::make_shared<Events>());

    // ---- File Drop Manager ----
    auto fileDropMgr = std::make_shared<FileDrop>(std::dynamic_pointer_cast<Window>(window));
    shared->GetChildren().Add(fileDropMgr);

#ifdef ENABLE_SCRIPTING
    // ---- Script Loader ----
    shared->GetChildren().Add(std::make_shared<ScriptLoader>(
        std::unordered_map<std::string, std::string>{}, 1, "-g -Wl", std::vector<std::string>{},
        std::vector<std::string>{}, std::vector<std::string>{}, resourceManager));
#endif

    // ---- Init all components that need it ----
    try {
        nlohmann::json rmArgs;
        rmArgs["archivePaths"] =
            archivePaths.empty() ? std::vector<std::string>{ mainPath, patchesPath } : archivePaths;
        rmArgs["validHashes"] = std::vector<uint32_t>(validHashes.begin(), validHashes.end());
        resourceManager->Init(rmArgs);
    } catch (const std::exception& e) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OTR file not found",
                                 "Main OTR file not found. Please generate one", nullptr);
        SPDLOG_ERROR("Failed to initialize ResourceManager: {}", e.what());
#ifdef __IOS__
        exit(0);
#endif
        return nullptr;
    }

    if (!resourceManager->GetArchiveManager()->IsInitialized()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OTR file not found",
                                 "Main OTR file not found. Please generate one", nullptr);
        SPDLOG_ERROR("Main OTR file not found!");
#ifdef __IOS__
        exit(0);
#endif
        return nullptr;
    }

    console->Init();
    window->Init();
    fileDropMgr->Init();
    audio->Init();

    auto& updateHook = GetBridgeCacheUpdateHook();
    if (updateHook) {
        updateHook(shared);
    }

    return shared;
}

std::shared_ptr<Context> Context::CreateInstance(const std::string& name, const std::string& shortName) {
    auto shared = std::make_shared<Context>(name, shortName);
    shared->SetContext(shared);
    return shared;
}

std::shared_ptr<Context> Context::CreateInstance(const std::string& name, const std::string& shortName,
                                                 std::vector<std::shared_ptr<Component>> components) {
    auto ctx = CreateInstance(name, shortName);
    for (auto& component : components) {
        ctx->GetChildren().Add(component);
        component->Init();
    }

    auto& updateHook = GetBridgeCacheUpdateHook();
    if (updateHook) {
        updateHook(ctx);
    }

    return ctx;
}

void Context::SetDefaultComponentInstaller(const std::function<void(const std::shared_ptr<Context>&)>& installer) {
    GetDefaultComponentInstallerHook() = installer;
}

void Context::SetBridgeCacheHandlers(const std::function<void(const std::shared_ptr<Context>&)>& update,
                                     const std::function<void()>& clear) {
    GetBridgeCacheUpdateHook() = update;
    GetBridgeCacheClearHook() = clear;
}

Context::Context(std::string name, std::string shortName)
    : Component(std::move(name)), mShortName(std::move(shortName)), mInitTime(std::chrono::steady_clock::now()) {
}

const std::string& Context::GetShortName() const {
    return mShortName;
}

std::string Context::GetAppBundlePath() {
#if defined(__ANDROID__)
    const char* externaldir = SDL_AndroidGetExternalStoragePath();
    if (externaldir != NULL) {
        return externaldir;
    }
#endif

#ifdef __IOS__
    const char* home = getenv("HOME");
    return std::string(home) + "/Documents";
#endif

#ifdef NON_PORTABLE
    return CMAKE_INSTALL_PREFIX;
#else
#ifdef __APPLE__
    FolderManager folderManager;
    return folderManager.getMainBundlePath();
#endif

#ifdef __linux__
    std::string progpath(PATH_MAX, '\0');
    int len = readlink("/proc/self/exe", &progpath[0], progpath.size() - 1);
    if (len != -1) {
        progpath.resize(len);

        // Find the last '/' and remove everything after it
        long unsigned int lastSlash = progpath.find_last_of("/");
        if (lastSlash != std::string::npos) {
            progpath.erase(lastSlash);
        }

        return progpath;
    }
#endif

#ifdef _WIN32
    std::wstring progpath(MAX_PATH, '\0');

    int len = GetModuleFileNameW(NULL, &progpath[0], progpath.size());
    if (len != 0 && len < progpath.size()) {
        progpath.resize(len);

        // Find the last '\' and remove everything after it
        long unsigned int lastSlash = progpath.find_last_of('\\');
        if (lastSlash != std::string::npos) {
            progpath.erase(lastSlash);
        }

        // Convert wstring to string
        len = WideCharToMultiByte(CP_UTF8, 0, progpath.data(), (int)progpath.size(), nullptr, 0, nullptr, nullptr);
        std::string newProgpath(len, 0);
        WideCharToMultiByte(CP_UTF8, 0, progpath.data(), (int)progpath.size(), &newProgpath[0], len, nullptr, nullptr);

        return newProgpath;
    }
#endif

    return ".";
#endif
}

std::string Context::GetAppDirectoryPath(const std::string& appName) {
#if defined(__ANDROID__)
    const char* externaldir = SDL_AndroidGetExternalStoragePath();
    if (externaldir != NULL) {
        return externaldir;
    }
#endif

#ifdef __IOS__
    const char* home = getenv("HOME");
    return std::string(home) + "/Documents";
#endif

#if defined(__APPLE__)
    FolderManager foldermanager;
    if (char* fpath = std::getenv("SHIP_HOME")) {
        const char* appBundleID = strrchr(fpath, '/');
        if (appBundleID != nullptr) {
            foldermanager.CreateAppSupportDirectory(appBundleID + 1);
        }
        if (fpath[0] == '~') {
            const char* home = getenv("HOME") ? getenv("HOME") : getpwuid(getuid())->pw_dir;
            return std::string(home) + std::string(fpath).substr(1);
        }
        return std::string(fpath);
    }
#endif

#if defined(__linux__)
    char* fpath = std::getenv("SHIP_HOME");
    if (fpath != NULL) {
        return std::string(fpath);
    }
#endif

#ifdef NON_PORTABLE
    const std::string effectiveAppName = appName.empty() ? "libultraship" : appName;
    char* prefpath = SDL_GetPrefPath(NULL, effectiveAppName.c_str());
    if (prefpath != NULL) {
        std::string ret(prefpath);
        SDL_free(prefpath);
        return ret;
    }
#endif

    return ".";
}

std::string Context::GetPathRelativeToAppBundle(const std::string& path) {
    return GetAppBundlePath() + "/" + path;
}

std::string Context::GetPathRelativeToAppDirectory(const std::string& path, const std::string& appName) {
    return GetAppDirectoryPath(appName) + "/" + path;
}

std::string Context::LocateFileAcrossAppDirs(const std::string& path, const std::string& appName) {
    std::string fpath;

    // app configuration dir
    fpath = GetPathRelativeToAppDirectory(path, appName);
    if (std::filesystem::exists(fpath)) {
        return fpath;
    }
    // app install dir
    fpath = GetPathRelativeToAppBundle(path);
    if (std::filesystem::exists(fpath)) {
        return fpath;
    }
    // current dir
    return "./" + std::string(path);
}

// ---- TickableComponent list ----

TickableList& Context::GetTickableComponents() {
    return mTickableComponents;
}

const TickableList& Context::GetTickableComponents() const {
    return mTickableComponents;
}

double Context::GetElapsedTimeSeconds() const {
    return mElapsedTimeSeconds;
}

void Context::UpdateElapsedTimeSeconds() {
    mElapsedTimeSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - mInitTime).count();
}

void Context::Tick(EventID eventId) {
    for (const auto& tickable : *mTickableComponents.Get()) {
        tickable->Tick(eventId);
    }
}

void Context::Tick() {
    UpdateElapsedTimeSeconds();
    Tick(TICK_EVENT_UPDATE);
    Tick(TICK_EVENT_LATE_UPDATE);
    Tick(TICK_EVENT_DRAW);
}

} // namespace Ship
