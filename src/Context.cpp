#include "Context.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include <iostream>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "install_config.h"
#include "graphic/Fast3D/debug/GfxDebugger.h"
#include "config/ConsoleVariable.h"
#include "controller/controldeck/ControlDeck.h"
#include "debug/CrashHandler.h"
#include "window/FileDropMgr.h"

#ifdef _WIN32
#include <libloaderapi.h>
#include <tchar.h>
#include <windows.h>
#endif

#ifdef __APPLE__
#include "utils/AppleFolderManager.h"
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
    GetWindow()->SaveWindowToConfig();

    // Explicitly destructing everything so that logging is done last.
    mAudio = nullptr;
    mWindow = nullptr;
    mConsole = nullptr;
    mCrashHandler = nullptr;
    mControlDeck = nullptr;
    mResourceManager = nullptr;
    mConsoleVariables = nullptr;
    GetConfig()->Save();
    mConfig = nullptr;
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
    : mConfigFilePath(std::move(configFilePath)), mName(std::move(name)), mShortName(std::move(shortName)) {
}

bool Context::Init(const std::vector<std::string>& archivePaths, const std::unordered_set<uint32_t>& validHashes,
                   uint32_t reservedThreadCount, AudioSettings audioSettings, std::shared_ptr<Window> window,
                   std::shared_ptr<ControlDeck> controlDeck) {
    return InitLogging() && InitConfiguration() && InitConsoleVariables() &&
           InitResourceManager(archivePaths, validHashes, reservedThreadCount) && InitControlDeck(controlDeck) &&
           InitCrashHandler() && InitConsole() && InitWindow(window) && InitAudio(audioSettings) && InitGfxDebugger() &&
           InitFileDropMgr();
}

bool Context::InitLogging() {
    if (GetLogger() != nullptr) {
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
#ifdef _DEBUG
        fileSink->set_level(spdlog::level::trace);
#else
        fileSink->set_level(spdlog::level::debug);
#endif
        sinks.push_back(fileSink);

        mLogger = std::make_shared<spdlog::async_logger>(GetName(), sinks.begin(), sinks.end(), spdlog::thread_pool(),
                                                         spdlog::async_overflow_policy::block);
#ifdef _DEBUG
        GetLogger()->set_level(spdlog::level::trace);
#else
        GetLogger()->set_level(spdlog::level::debug);
#endif

#if defined(_DEBUG)
        GetLogger()->flush_on(spdlog::level::trace);
#endif

        GetLogger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%@] [%l] %v");

        spdlog::register_logger(GetLogger());
        spdlog::set_default_logger(GetLogger());
        return true;
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
        return false;
    }
}

bool Context::InitConfiguration() {
    if (GetConfig() != nullptr) {
        return true;
    }

    mConfig = std::make_shared<Config>(GetPathRelativeToAppDirectory(mConfigFilePath));

    if (GetConfig() == nullptr) {
        SPDLOG_ERROR("Failed to initialize config");
        return false;
    }

    return true;
}

bool Context::InitConsoleVariables() {
    if (GetConsoleVariables() != nullptr) {
        return true;
    }

    mConsoleVariables = std::make_shared<ConsoleVariable>();

    if (GetConsoleVariables() == nullptr) {
        SPDLOG_ERROR("Failed to initialize console variables");
        return false;
    }

    return true;
}

bool Context::InitResourceManager(const std::vector<std::string>& archivePaths,
                                  const std::unordered_set<uint32_t>& validHashes, uint32_t reservedThreadCount) {
    if (GetResourceManager() != nullptr) {
        return true;
    }

    mMainPath = GetConfig()->GetString("Game.Main Archive", GetAppDirectoryPath());
    mPatchesPath = GetConfig()->GetString("Game.Patches Archive", GetAppDirectoryPath() + "/mods");
    if (archivePaths.empty()) {
        std::vector<std::string> paths = std::vector<std::string>();
        paths.push_back(mMainPath);
        paths.push_back(mPatchesPath);

        mResourceManager = std::make_shared<ResourceManager>();
        GetResourceManager()->Init(paths, validHashes, reservedThreadCount);
    } else {
        mResourceManager = std::make_shared<ResourceManager>();
        GetResourceManager()->Init(archivePaths, validHashes, reservedThreadCount);
    }

    if (!GetResourceManager()->IsLoaded()) {
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
    if (GetControlDeck() != nullptr) {
        return true;
    }

    mControlDeck = controlDeck;

    if (GetControlDeck() == nullptr) {
        SPDLOG_ERROR("Failed to initialize control deck");
        return false;
    }

    return true;
}

bool Context::InitCrashHandler() {
    if (GetCrashHandler() != nullptr) {
        return true;
    }

    mCrashHandler = std::make_shared<CrashHandler>();

    if (GetCrashHandler() == nullptr) {
        SPDLOG_ERROR("Failed to initialize crash handler");
        return false;
    }

    return true;
}

bool Context::InitAudio(AudioSettings settings) {
    if (GetAudio() != nullptr) {
        return true;
    }

    mAudio = std::make_shared<Audio>(settings);

    if (GetAudio() == nullptr) {
        SPDLOG_ERROR("Failed to initialize audio");
        return false;
    }

    GetAudio()->Init();
    return true;
}

bool Context::InitGfxDebugger() {
    if (GetGfxDebugger() != nullptr) {
        return true;
    }

    mGfxDebugger = std::make_shared<Fast::GfxDebugger>();

    if (GetGfxDebugger() == nullptr) {
        SPDLOG_ERROR("Failed to initialize gfx debugger");
        return false;
    }

    return true;
}

bool Context::InitConsole() {
    if (GetConsole() != nullptr) {
        return true;
    }

    mConsole = std::make_shared<Console>();

    if (GetConsole() == nullptr) {
        SPDLOG_ERROR("Failed to initialize console");
        return false;
    }

    GetConsole()->Init();

    return true;
}

bool Context::InitWindow(std::shared_ptr<Window> window) {
    if (GetWindow() != nullptr) {
        return true;
    }

    mWindow = window;

    if (GetWindow() == nullptr) {
        SPDLOG_ERROR("Failed to initialize window");
        return false;
    }

    GetWindow()->Init();

    return true;
}

bool Context::InitFileDropMgr() {
    if (GetFileDropMgr() != nullptr) {
        return true;
    }

    mFileDropMgr = std::make_shared<FileDropMgr>();
    if (GetFileDropMgr() == nullptr) {
        SPDLOG_ERROR("Failed to initialize file drop manager");
        return false;
    }
    return true;
}

std::shared_ptr<ConsoleVariable> Context::GetConsoleVariables() {
    return mConsoleVariables;
}

std::shared_ptr<spdlog::logger> Context::GetLogger() {
    return mLogger;
}

std::shared_ptr<Config> Context::GetConfig() {
    return mConfig;
}

std::shared_ptr<ResourceManager> Context::GetResourceManager() {
    return mResourceManager;
}

std::shared_ptr<ControlDeck> Context::GetControlDeck() {
    return mControlDeck;
}

std::shared_ptr<CrashHandler> Context::GetCrashHandler() {
    return mCrashHandler;
}

std::shared_ptr<Window> Context::GetWindow() {
    return mWindow;
}

std::shared_ptr<Console> Context::GetConsole() {
    return mConsole;
}

std::shared_ptr<Audio> Context::GetAudio() {
    return mAudio;
}

std::shared_ptr<Fast::GfxDebugger> Context::GetGfxDebugger() {
    return mGfxDebugger;
}

std::shared_ptr<FileDropMgr> Context::GetFileDropMgr() {
    return mFileDropMgr;
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
    std::string progpath(MAX_PATH, '\0');

    int len = GetModuleFileNameA(NULL, &progpath[0], progpath.size());
    if (len != 0 && len < progpath.size()) {
        progpath.resize(len);

        // Find the last '\' and remove everything after it
        long unsigned int lastSlash = progpath.find_last_of("\\");
        if (lastSlash != std::string::npos) {
            progpath.erase(lastSlash);
        }

        return progpath;
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

} // namespace Ship
