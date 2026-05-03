#include "ship/Context.h"
#include "ship/TickableComponent.h"
#include <cstring>
#include <functional>
#include <iostream>
#include <algorithm>
#include <queue>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "ship/install_config.h"
#include "ship/default_context_json.h" // Auto-generated from default_context.json by CMake
#include "fast/debug/GfxDebugger.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/debug/Console.h"
#include "ship/debug/CrashHandler.h"
#include "ship/resource/ResourceManager.h"
#include "ship/window/FileDropMgr.h"
#include "ship/log/LoggerComponent.h"
#include "ship/thread/ThreadPoolComponent.h"
#include "ship/events/Events.h"
#ifdef ENABLE_SCRIPTING
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
std::weak_ptr<Context> Context::mContext;

std::shared_ptr<Context> Context::GetInstance() {
    return mContext.lock();
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
    if (mOwnsLogger) {
        spdlog::shutdown();
    }
}

std::shared_ptr<Context> Context::CreateDefaultInstance(const std::string& name, const std::string& shortName,
                                                        const std::string& configFilePath,
                                                        const std::vector<std::string>& archivePaths,
                                                        const std::unordered_set<uint32_t>& validHashes,
                                                        uint32_t reservedThreadCount, AudioSettings audioSettings,
                                                        std::shared_ptr<Component> window,
                                                        std::shared_ptr<Component> controlDeck) {
    if (!mContext.expired()) {
        SPDLOG_DEBUG("Trying to create a context when it already exists. Returning existing.");
        return GetInstance();
    }

    auto shared = std::make_shared<Context>(name, shortName, configFilePath);
    mContext = shared;

    // ---- Logging ----
    try {
        spdlog::init_thread_pool(8192, 1);
        std::vector<spdlog::sink_ptr> sinks;

#if (!defined(_WIN32)) || defined(_DEBUG)
#if defined(_DEBUG) && defined(_WIN32)
        FreeConsole();
        if (AllocConsole() == 0) {
            throw std::system_error(GetLastError(), std::generic_category(), "Failed to create debug console");
        }

        SetConsoleOutputCP(CP_UTF8);

        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONIN$", "r", stdin);
        std::cout.clear();
        std::clog.clear();
        std::cerr.clear();
        std::cin.clear();

        HANDLE hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        HANDLE hConIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
        SetStdHandle(STD_ERROR_HANDLE, hConOut);
        SetStdHandle(STD_INPUT_HANDLE, hConIn);
        std::wcout.clear();
        std::wclog.clear();
        std::wcerr.clear();
        std::wcin.clear();
#endif
        auto systemConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sinks.push_back(systemConsoleSink);
#endif

        auto logPath = GetPathRelativeToAppDirectory(("logs/" + name + ".log"));
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logPath, 1024 * 1024 * 10, 10);
        sinks.push_back(fileSink);

        std::shared_ptr<spdlog::logger> logger;
#ifdef _DEBUG
        logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::trace);
#else
        logger = std::make_shared<spdlog::async_logger>(name, sinks.begin(), sinks.end(), spdlog::thread_pool(),
                                                        spdlog::async_overflow_policy::block);
        logger->set_level(spdlog::level::warn);
        logger->flush_on(spdlog::level::info);
#endif

        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%@] [%l] %v");

        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);
        shared->mOwnsLogger = true;

        shared->GetChildren().Add(std::make_shared<LoggerComponent>(logger));
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
        return nullptr;
    }

    // ---- Configuration ----
    auto config = std::make_shared<Config>(GetPathRelativeToAppDirectory(configFilePath));
    shared->GetChildren().Add(config);

    // Read config values needed for component construction
    auto mainPath = config->GetString("Game.Main Archive", GetAppDirectoryPath());
    auto patchesPath = config->GetString("Game.Patches Archive", GetAppDirectoryPath() + "/mods");
    size_t threadCount = std::max(1, (int32_t)(std::thread::hardware_concurrency() - reservedThreadCount - 1));

    // ---- Console Variables ----
    shared->GetChildren().Add(std::make_shared<ConsoleVariable>());

    // ---- Thread Pool ----
    shared->GetChildren().Add(std::make_shared<ThreadPoolComponent>(threadCount));

#ifdef ENABLE_SCRIPTING
    // ---- Keystore ----
    shared->GetChildren().Add(std::make_shared<Keystore>());
#endif

    // ---- Resource Manager ----
    auto resourceManager = std::make_shared<ResourceManager>();
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
    auto audio = std::make_shared<Audio>(audioSettings);
    shared->GetChildren().Add(audio);

    // ---- Gfx Debugger ----
    shared->GetChildren().Add(std::make_shared<Fast::GfxDebugger>());

    // ---- Events ----
    shared->GetChildren().Add(std::make_shared<Events>());

    // ---- File Drop Manager ----
    auto fileDropMgr = std::make_shared<FileDropMgr>();
    shared->GetChildren().Add(fileDropMgr);

#ifdef ENABLE_SCRIPTING
    // ---- Script Loader ----
    shared->GetChildren().Add(std::make_shared<ScriptLoader>(std::unordered_map<std::string, std::string>{}, 1,
                                                             "-g -Wl", std::vector<std::string>{},
                                                             std::vector<std::string>{}, std::vector<std::string>{}));
#endif

    // ---- Init all components that need it ----
    if (archivePaths.empty()) {
        std::vector<std::string> paths;
        paths.push_back(mainPath);
        paths.push_back(patchesPath);
        resourceManager->Init(paths, validHashes);
    } else {
        resourceManager->Init(archivePaths, validHashes);
    }

    if (!resourceManager->IsLoaded()) {
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
            return std::make_shared<ConsoleVariable>();
        } else if (type == "ThreadPoolComponent") {
            size_t threadCount = compArgs.value("threadCount", static_cast<size_t>(1));
            return std::make_shared<ThreadPoolComponent>(threadCount);
        } else if (type == "ResourceManager") {
            return std::make_shared<ResourceManager>();
        } else if (type == "CrashHandler") {
            return std::make_shared<CrashHandler>();
        } else if (type == "Console") {
            return std::make_shared<Console>();
        } else if (type == "Audio") {
            AudioSettings settings;
            if (compArgs.contains("channelSetting")) {
                settings.ChannelSetting = static_cast<AudioChannelsSetting>(compArgs["channelSetting"].get<int>());
            }
            return std::make_shared<Audio>(settings);
        } else if (type == "GfxDebugger") {
            return std::make_shared<Fast::GfxDebugger>();
        } else if (type == "Events") {
            return std::make_shared<Events>();
        } else if (type == "FileDropMgr") {
            return std::make_shared<FileDropMgr>();
        } else if (type == "LoggerComponent") {
            // LoggerComponent requires a logger - skip if not provided in overrides.
            return nullptr;
#ifdef ENABLE_SCRIPTING
        } else if (type == "Keystore") {
            return std::make_shared<Keystore>();
        } else if (type == "ScriptLoader") {
            return std::make_shared<ScriptLoader>(std::unordered_map<std::string, std::string>{}, 1, "-g -Wl",
                                                  std::vector<std::string>{}, std::vector<std::string>{},
                                                  std::vector<std::string>{});
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

    // Phase 2: Initialize components in declared order.
    // Search the full hierarchy for each named component.
    if (json.contains("initOrder") && json["initOrder"].is_array()) {
        for (const auto& initEntry : json["initOrder"]) {
            std::string name;
            nlohmann::json compArgs;

            if (initEntry.is_string()) {
                // Simple string: just the component name.
                name = initEntry.get<std::string>();
                compArgs = initArgs.contains(name) ? initArgs[name] : nlohmann::json::object();
            } else if (initEntry.is_object() && initEntry.contains("name")) {
                // Object with inline initArgs: {"name": "Foo", "initArgs": {...}}
                name = initEntry["name"].get<std::string>();
                compArgs = initArgs.contains(name) ? initArgs[name] : nlohmann::json::object();
                if (initEntry.contains("initArgs") && initEntry["initArgs"].is_object()) {
                    for (auto& [key, value] : initEntry["initArgs"].items()) {
                        compArgs[key] = value;
                    }
                }
            } else {
                continue;
            }

            // BFS through the full hierarchy to find the component.
            std::queue<Component*> searchQueue;
            searchQueue.push(context.get());
            std::unordered_set<uint64_t> visited;
            visited.insert(context->GetId());
            bool found = false;

            while (!searchQueue.empty() && !found) {
                Component* current = searchQueue.front();
                searchQueue.pop();
                auto children = current->GetChildren().Get();
                for (const auto& child : *children) {
                    if (visited.insert(child->GetId()).second) {
                        if (child->GetName() == name && !child->IsInitialized()) {
                            child->Init(compArgs);
                            found = true;
                            break;
                        }
                        searchQueue.push(child.get());
                    }
                }
            }
        }
    }

    return true;
}

std::shared_ptr<Context> Context::CreateInstance(const std::string& name, const std::string& shortName,
                                                 const std::string& configFilePath) {
    if (!mContext.expired()) {
        SPDLOG_DEBUG("Trying to create a context when it already exists. Returning existing.");
        return GetInstance();
    }

    auto shared = std::make_shared<Context>(name, shortName, configFilePath);
    mContext = shared;
    return shared;
}

Context::Context(std::string name, std::string shortName, std::string configFilePath)
    : Component(name), mConfigFilePath(std::move(configFilePath)), mName(std::move(name)),
      mShortName(std::move(shortName)) {
}

std::string Context::GetName() const {
    return mName;
}

std::string Context::GetShortName() const {
    return mShortName;
}

std::string Context::GetConfigFilePath() const {
    return mConfigFilePath;
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
    const std::string& effectiveAppName = appName.empty() ? GetInstance()->mShortName : appName;
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

} // namespace Ship
