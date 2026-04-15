#include "ship/Context.h"
#include "ship/TickableComponent.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include <iostream>
#include <algorithm>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "ship/install_config.h"
#include "fast/debug/GfxDebugger.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/debug/CrashHandler.h"
#include "ship/window/FileDropMgr.h"
#include "ship/log/LoggerComponent.h"
#include "ship/thread/ThreadPoolComponent.h"

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
    SPDLOG_TRACE("destruct context");
    auto window = GetChildren().GetFirst<Window>();
    if (window) {
        window->SaveWindowToConfig();
    }

    auto config = GetChildren().GetFirst<Config>();

    // Remove children in order to allow explicit teardown before logging shuts down.
    RemoveChildren(true);

    if (config) {
        config->Save();
    }
    spdlog::shutdown();
}

std::shared_ptr<Context>
Context::CreateInstance(const std::string name, const std::string shortName, const std::string configFilePath,
                        const std::vector<std::string>& archivePaths, const std::unordered_set<uint32_t>& validHashes,
                        uint32_t reservedThreadCount, AudioSettings audioSettings, std::shared_ptr<Window> window,
                        std::shared_ptr<ControlDeck> controlDeck) {
    if (mContext.expired()) {
        auto shared = std::make_shared<Context>(name, shortName, configFilePath);
        mContext = shared;
        if (shared->Init(archivePaths, validHashes, reservedThreadCount, audioSettings, window, controlDeck)) {
            return shared;
        } else {
            SPDLOG_ERROR("Failed to initialize");
            return nullptr;
        };
    }

    SPDLOG_DEBUG("Trying to create a context when it already exists. Returning existing.");

    return GetInstance();
}

std::shared_ptr<Context> Context::CreateUninitializedInstance(const std::string name, const std::string shortName,
                                                              const std::string configFilePath) {
    if (mContext.expired()) {
        auto shared = std::make_shared<Context>(name, shortName, configFilePath);
        mContext = shared;
        return shared;
    }

    SPDLOG_DEBUG("Trying to create an uninitialized context when it already exists. Returning existing.");

    return GetInstance();
}

Context::Context(std::string name, std::string shortName, std::string configFilePath)
    : Component(name), Tickable(), mConfigFilePath(std::move(configFilePath)), mName(std::move(name)),
      mShortName(std::move(shortName)), mIsTickableComponentsOrderStale(false) {
}

bool Context::Init(const std::vector<std::string>& archivePaths, const std::unordered_set<uint32_t>& validHashes,
                   uint32_t reservedThreadCount, AudioSettings audioSettings, std::shared_ptr<Window> window,
                   std::shared_ptr<ControlDeck> controlDeck) {
    return InitLogging() && InitConfiguration() && InitConsoleVariables() && InitThreadPool(reservedThreadCount) &&
           InitResourceManager(archivePaths, validHashes, reservedThreadCount) && InitControlDeck(controlDeck) &&
           InitCrashHandler() && InitConsole() && InitWindow(window) && InitAudio(audioSettings) && InitGfxDebugger() &&
           InitFileDropMgr();
}

bool Context::InitLogging() {
    if (GetChildren().GetFirst<LoggerComponent>() != nullptr) {
        return true;
    }

    try {
        // Setup Logging
        spdlog::init_thread_pool(8192, 1);
        std::vector<spdlog::sink_ptr> sinks;

#if (!defined(_WIN32)) || defined(_DEBUG)
#if defined(_DEBUG) && defined(_WIN32)
        // LLVM on Windows allocs a hidden console in its entrypoint function.
        // We free that console here to create our own.
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
        // systemConsoleSink->set_level(spdlog::level::trace);
        sinks.push_back(systemConsoleSink);
#endif

        auto logPath = GetPathRelativeToAppDirectory(("logs/" + GetName() + ".log"));
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logPath, 1024 * 1024 * 10, 10);
        sinks.push_back(fileSink);

        std::shared_ptr<spdlog::logger> logger;
#ifdef _DEBUG
        logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
        logger->flush_on(spdlog::level::trace);
#else
        logger = std::make_shared<spdlog::async_logger>(GetName(), sinks.begin(), sinks.end(), spdlog::thread_pool(),
                                                        spdlog::async_overflow_policy::block);
#endif

        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%@] [%l] %v");

        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);

        AddChild(std::make_shared<LoggerComponent>(logger));
        return true;
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
        return false;
    }
}

bool Context::InitConfiguration() {
    if (GetChildren().GetFirst<Config>() != nullptr) {
        return true;
    }

    auto config = std::make_shared<Config>(GetPathRelativeToAppDirectory(mConfigFilePath));
    AddChild(config);

    if (GetChildren().GetFirst<Config>() == nullptr) {
        SPDLOG_ERROR("Failed to initialize config");
        return false;
    }

    return true;
}

bool Context::InitConsoleVariables() {
    if (GetChildren().GetFirst<ConsoleVariable>() != nullptr) {
        return true;
    }

    auto consoleVariables = std::make_shared<ConsoleVariable>();
    AddChild(consoleVariables);

    if (GetChildren().GetFirst<ConsoleVariable>() == nullptr) {
        SPDLOG_ERROR("Failed to initialize console variables");
        return false;
    }

    return true;
}

bool Context::InitResourceManager(const std::vector<std::string>& archivePaths,
                                  const std::unordered_set<uint32_t>& validHashes, uint32_t reservedThreadCount) {
    if (GetChildren().GetFirst<ResourceManager>() != nullptr) {
        return true;
    }

    auto config = GetChildren().GetFirst<Config>();
    mMainPath = config->GetString("Game.Main Archive", GetAppDirectoryPath());
    mPatchesPath = config->GetString("Game.Patches Archive", GetAppDirectoryPath() + "/mods");

    auto resourceManager = std::make_shared<ResourceManager>();
    AddChild(resourceManager);

    if (archivePaths.empty()) {
        std::vector<std::string> paths = std::vector<std::string>();
        paths.push_back(mMainPath);
        paths.push_back(mPatchesPath);
        resourceManager->Init(paths, validHashes, reservedThreadCount);
    } else {
        resourceManager->Init(archivePaths, validHashes, reservedThreadCount);
    }

    if (!resourceManager->IsLoaded()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OTR file not found",
                                 "Main OTR file not found. Please generate one", nullptr);
        SPDLOG_ERROR("Main OTR file not found!");
#ifdef __IOS__
        // We need this exit to close the app when we dismiss the dialog
        exit(0);
#endif
        return false;
    }

    return true;
}

bool Context::InitControlDeck(std::shared_ptr<ControlDeck> controlDeck) {
    if (GetChildren().GetFirst<ControlDeck>() != nullptr) {
        return true;
    }

    if (controlDeck == nullptr) {
        SPDLOG_ERROR("Failed to initialize control deck");
        return false;
    }

    AddChild(controlDeck);
    return true;
}

bool Context::InitCrashHandler() {
    if (GetChildren().GetFirst<CrashHandler>() != nullptr) {
        return true;
    }

    auto crashHandler = std::make_shared<CrashHandler>();
    AddChild(crashHandler);

    if (GetChildren().GetFirst<CrashHandler>() == nullptr) {
        SPDLOG_ERROR("Failed to initialize crash handler");
        return false;
    }

    return true;
}

bool Context::InitAudio(AudioSettings settings) {
    if (GetChildren().GetFirst<Audio>() != nullptr) {
        return true;
    }

    auto audio = std::make_shared<Audio>(settings);
    AddChild(audio);

    if (GetChildren().GetFirst<Audio>() == nullptr) {
        SPDLOG_ERROR("Failed to initialize audio");
        return false;
    }

    audio->Init();
    return true;
}

bool Context::InitGfxDebugger() {
    if (GetChildren().GetFirst<Fast::GfxDebugger>() != nullptr) {
        return true;
    }

    auto gfxDebugger = std::make_shared<Fast::GfxDebugger>();
    AddChild(gfxDebugger);

    if (GetChildren().GetFirst<Fast::GfxDebugger>() == nullptr) {
        SPDLOG_ERROR("Failed to initialize gfx debugger");
        return false;
    }

    return true;
}

bool Context::InitConsole() {
    if (GetChildren().GetFirst<Console>() != nullptr) {
        return true;
    }

    auto console = std::make_shared<Console>();
    AddChild(console);

    if (GetChildren().GetFirst<Console>() == nullptr) {
        SPDLOG_ERROR("Failed to initialize console");
        return false;
    }

    console->Init();

    return true;
}

bool Context::InitWindow(std::shared_ptr<Window> window) {
    if (GetChildren().GetFirst<Window>() != nullptr) {
        return true;
    }

    if (window == nullptr) {
        SPDLOG_ERROR("Failed to initialize window");
        return false;
    }

    AddChild(window);
    window->Init();

    return true;
}

bool Context::InitFileDropMgr() {
    if (GetChildren().GetFirst<FileDropMgr>() != nullptr) {
        return true;
    }

    auto fileDropMgr = std::make_shared<FileDropMgr>();
    AddChild(fileDropMgr);

    if (GetChildren().GetFirst<FileDropMgr>() == nullptr) {
        SPDLOG_ERROR("Failed to initialize file drop manager");
        return false;
    }
    return true;
}

bool Context::InitThreadPool(uint32_t reservedThreadCount) {
    if (GetChildren().GetFirst<ThreadPoolComponent>() != nullptr) {
        return true;
    }

    // the extra `- 1` is because we reserve an extra thread for spdlog
    size_t threadCount = std::max(1, (int32_t)(std::thread::hardware_concurrency() - reservedThreadCount - 1));
    auto threadPool = std::make_shared<ThreadPoolComponent>(threadCount);
    AddChild(threadPool);

    if (GetChildren().GetFirst<ThreadPoolComponent>() == nullptr) {
        SPDLOG_ERROR("Failed to initialize thread pool");
        return false;
    }

    return true;
}

std::string Context::GetName() {
    return mName;
}

std::string Context::GetShortName() {
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

std::string Context::GetAppDirectoryPath(std::string appName) {
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
    if (char* fpath = std::getenv("SHIP_HOME")) {
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
    if (appName.empty()) {
        appName = GetInstance()->mShortName;
    }
    char* prefpath = SDL_GetPrefPath(NULL, appName.c_str());
    if (prefpath != NULL) {
        std::string ret(prefpath);
        SDL_free(prefpath);
        return ret;
    }
#endif

    return ".";
}

std::string Context::GetPathRelativeToAppBundle(const std::string path) {
    return GetAppBundlePath() + "/" + path;
}

std::string Context::GetPathRelativeToAppDirectory(const std::string path, std::string appName) {
    return GetAppDirectoryPath(appName) + "/" + path;
}

std::string Context::LocateFileAcrossAppDirs(const std::string path, std::string appName) {
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

PartList<TickableComponent>& Context::GetTickableComponents() {
    return mTickableComponents;
}

const PartList<TickableComponent>& Context::GetTickableComponents() const {
    return mTickableComponents;
}

Context& Context::SortTickableComponents() {
    const std::lock_guard<std::mutex> lock(mTickableMutex);
    auto& list = mTickableComponents.GetList();
    std::stable_sort(list.begin(), list.end(),
                     [](const std::shared_ptr<TickableComponent>& a, const std::shared_ptr<TickableComponent>& b) {
                         return a->GetOrder() < b->GetOrder();
                     });
    mIsTickableComponentsOrderStale = false;
    return *this;
}

} // namespace Ship
