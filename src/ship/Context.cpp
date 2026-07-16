#include "ship/Context.h"
#include "ship/TickableComponent.h"
#include <cstring>
#include <functional>
#include <iostream>
#include <algorithm>
#include <queue>
#include "ship/install_config.h"
#include "ship/default_context_json.h" // Auto-generated from default_context.json by CMake
#include "fast/debug/GfxDebugger.h"
#include "fast/Fast3dWindow.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/debug/Console.h"
#include "ship/debug/CrashHandler.h"
#include "ship/resource/ResourceManager.h"
#include "ship/window/FileDrop.h"
#include "ship/log/Logger.h"
#include "ship/thread/ThreadPool.h"
#include "ship/events/Events.h"
#include "libultraship/bridge/audiobridge.h"
#include "libultraship/bridge/consolevariablebridge.h"
#include "libultraship/bridge/controllerbridge.h"
#include "libultraship/bridge/crashhandlerbridge.h"
#include "libultraship/bridge/eventsbridge.h"
#include "libultraship/bridge/gfxbridge.h"
#include "libultraship/bridge/gfxdebuggerbridge.h"
#include "libultraship/bridge/resourcebridge.h"
#include "libultraship/bridge/windowbridge.h"
#ifdef ENABLE_SCRIPTING
#include "libultraship/bridge/scriptingbridge.h"
#include "ship/scripting/ScriptLoader.h"
#include "ship/security/Keystore.h"
#endif

#ifdef _WIN32
#include <libloaderapi.h>
#include <tchar.h>
#include <windows.h>
#include <stringapiset.h>
#endif

#ifdef __APPLE__
#include "ship/utils/AppleFolderManager.h"
#include <unistd.h>
#include <pwd.h>
#endif

namespace Ship {
static void UpdateBridgeCaches(const std::shared_ptr<Context>& context) {
    ResourceSetResourceManager(context->GetChildren().GetFirst<ResourceManager>());
    CVarSetConsoleVariable(context->GetChildren().GetFirst<ConsoleVariable>());
    WindowSetWindowComponent(context->GetChildren().GetFirst<Window>());
    ControllerSetControlDeck(context->GetChildren().GetFirst<ControlDeck>());
    EventSystemSetEvents(context->GetChildren().GetFirst<Events>());
    AudioSetAudioComponent(context->GetChildren().GetFirst<Audio>());
    CrashHandlerSetComponent(context->GetChildren().GetFirst<CrashHandler>());
    GfxDebuggerSetComponent(context->GetChildren().GetFirst<Fast::GfxDebugger>());
    GfxSetFast3dWindow(std::dynamic_pointer_cast<Fast::Fast3dWindow>(context->GetChildren().GetFirst<Window>()));
#ifdef ENABLE_SCRIPTING
    ScriptSetLoader(context->GetChildren().GetFirst<ScriptLoader>());
#endif
}

Context::~Context() {
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

    // Remove children in order to allow explicit teardown before logging shuts down.
    GetChildren().Remove(true);

    if (config) {
        config->Save();
    }
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

    // ---- Gfx Debugger ----
    shared->GetChildren().Add(std::make_shared<Fast::GfxDebugger>());

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
    UpdateBridgeCaches(shared);

    return shared;
}

bool Context::BuildComponentsFromJson(std::shared_ptr<Context> context, const nlohmann::json& json,
                                      const nlohmann::json& initArgs,
                                      const std::unordered_map<std::string, std::shared_ptr<Component>>& overrides) {
    if (!json.contains("components") || !json["components"].is_array()) {
        SPDLOG_ERROR("BuildComponentsFromJson: missing or invalid 'components' array");
        return false;
    }

    // Set of compile-time conditions that are active.
    std::unordered_set<std::string> activeConditions;
#ifdef ENABLE_SCRIPTING
    activeConditions.insert("ENABLE_SCRIPTING");
#endif
#ifdef ENABLE_DX11
    activeConditions.insert("ENABLE_DX11");
#endif

    // Helper: create a component by type name.
    auto createComponent = [&](const std::string& type, const nlohmann::json& compArgs) -> std::shared_ptr<Component> {
        if (type == "Config") {
            std::string configPath = compArgs.value("path", "");
            return std::make_shared<Config>(configPath);
        } else if (type == "ConsoleVariable") {
            return std::make_shared<ConsoleVariable>(context->GetChildren().GetFirst<Config>());
        } else if (type == "ThreadPool") {
            size_t threadCount = compArgs.value("threadCount", static_cast<size_t>(1));
            return std::make_shared<ThreadPool>(threadCount);
        } else if (type == "ResourceManager") {
            auto threadPool = context->GetChildren().GetFirst<ThreadPool>();
#ifdef ENABLE_SCRIPTING
            return std::make_shared<ResourceManager>(threadPool, context->GetChildren().GetFirst<Keystore>());
#else
            return std::make_shared<ResourceManager>(threadPool);
#endif
        } else if (type == "CrashHandler") {
            return std::make_shared<CrashHandler>();
        } else if (type == "Console") {
            return std::make_shared<Console>();
        } else if (type == "Audio") {
            AudioSettings settings;
            if (compArgs.contains("channelSetting")) {
                settings.ChannelSetting = static_cast<AudioChannelsSetting>(compArgs["channelSetting"].get<int>());
            }
            return std::make_shared<Audio>(settings, context->GetChildren().GetFirst<Config>());
        } else if (type == "GfxDebugger") {
            return std::make_shared<Fast::GfxDebugger>();
        } else if (type == "Events") {
            return std::make_shared<Events>();
        } else if (type == "FileDrop") {
            return std::make_shared<FileDrop>(context->GetChildren().GetFirst<Window>());
        } else if (type == "Logger") {
            // Logger requires a logger - skip if not provided in overrides.
            return nullptr;
#ifdef ENABLE_SCRIPTING
        } else if (type == "Keystore") {
            return std::make_shared<Keystore>(context->GetChildren().GetFirst<Config>());
        } else if (type == "ScriptLoader") {
            return std::make_shared<ScriptLoader>(std::unordered_map<std::string, std::string>{}, 1, "-g -Wl",
                                                  std::vector<std::string>{}, std::vector<std::string>{},
                                                  std::vector<std::string>{},
                                                  context->GetChildren().GetFirst<ResourceManager>());
#endif
        }
        SPDLOG_WARN("BuildComponentsFromJson: unknown component type '{}'", type);
        return nullptr;
    };

    // Recursive helper: process a component entry and its children.
    std::function<void(const nlohmann::json&, std::shared_ptr<Component>)> processEntry =
        [&](const nlohmann::json& entry, std::shared_ptr<Component> parent) {
            if (!entry.contains("type") || !entry["type"].is_string()) {
                return;
            }
            std::string type = entry["type"].get<std::string>();
            std::string name = entry.value("name", type);

            // Check compile-time condition.
            if (entry.contains("condition") && entry["condition"].is_string()) {
                std::string condition = entry["condition"].get<std::string>();
                if (activeConditions.find(condition) == activeConditions.end()) {
                    return;
                }
            }

            std::shared_ptr<Component> component = nullptr;

            // Merge initArgs: inline "initArgs" from the entry takes precedence,
            // then the top-level initArgs keyed by name.
            nlohmann::json compArgs = initArgs.contains(name) ? initArgs[name] : nlohmann::json::object();
            if (entry.contains("initArgs") && entry["initArgs"].is_object()) {
                // Inline initArgs override top-level ones.
                for (auto& [key, value] : entry["initArgs"].items()) {
                    compArgs[key] = value;
                }
            }

            // Check if an override is provided.
            if (overrides.count(type)) {
                component = overrides.at(type);
            } else {
                component = createComponent(type, compArgs);
            }

            if (component) {
                parent->GetChildren().Add(component);

                // Recursively process children specified in the JSON hierarchy.
                if (entry.contains("children") && entry["children"].is_array()) {
                    for (const auto& childEntry : entry["children"]) {
                        processEntry(childEntry, component);
                    }
                }
            }
        };

    // Phase 1: Create and add all components respecting hierarchy.
    for (const auto& entry : json["components"]) {
        processEntry(entry, context);
    }

    // Phase 2: Initialize components in declaration order.
    // Components that self-initialize (MarkInitialized in constructor) are skipped.
    // Iterate in the same order as the components array, including children depth-first.
    std::function<void(const nlohmann::json&)> initEntry = [&](const nlohmann::json& entry) {
        if (!entry.contains("type") || !entry["type"].is_string()) {
            return;
        }
        std::string name = entry.value("name", entry["type"].get<std::string>());

        // Skip if condition not met.
        if (entry.contains("condition") && entry["condition"].is_string()) {
            std::string condition = entry["condition"].get<std::string>();
            if (activeConditions.find(condition) == activeConditions.end()) {
                return;
            }
        }

        // Merge initArgs for this component.
        nlohmann::json compArgs = initArgs.contains(name) ? initArgs[name] : nlohmann::json::object();
        if (entry.contains("initArgs") && entry["initArgs"].is_object()) {
            for (auto& [key, value] : entry["initArgs"].items()) {
                compArgs[key] = value;
            }
        }

        // BFS through the full hierarchy to find the component by name.
        std::queue<Component*> searchQueue;
        searchQueue.push(context.get());
        std::unordered_set<uint64_t> visited;
        visited.insert(context->GetId());

        while (!searchQueue.empty()) {
            Component* current = searchQueue.front();
            searchQueue.pop();
            auto children = current->GetChildren().Get();
            for (const auto& child : *children) {
                if (visited.insert(child->GetId()).second) {
                    if (child->GetName() == name && !child->IsInitialized()) {
                        child->Init(compArgs);
                        goto done;
                    }
                    searchQueue.push(child.get());
                }
            }
        }
    done:

        // Recursively init children in declared order.
        if (entry.contains("children") && entry["children"].is_array()) {
            for (const auto& childEntry : entry["children"]) {
                initEntry(childEntry);
            }
        }
    };

    for (const auto& entry : json["components"]) {
        initEntry(entry);
    }

    UpdateBridgeCaches(context);
    return true;
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
    UpdateBridgeCaches(ctx);
    return ctx;
}

Context::Context(std::string name, std::string shortName)
    : Component(std::move(name)), mShortName(std::move(shortName)) {
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

void Context::Tick() {
    const auto now = std::chrono::steady_clock::now();
    double durationSinceLastTick = 0.0;
    if (mLastTickTime != std::chrono::steady_clock::time_point{}) {
        durationSinceLastTick = std::chrono::duration<double>(now - mLastTickTime).count();
    }
    mLastTickTime = now;

    for (const auto& tickable : *mTickableComponents.Get()) {
        tickable->Run(durationSinceLastTick);
    }
}

} // namespace Ship
