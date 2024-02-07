#include "Context.h"
#include "controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include <iostream>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "install_config.h"

#ifdef _WIN32
#include <tchar.h>
#endif

#ifdef __APPLE__
#include "utils/OSXFolderManager.h"
#elif defined(__SWITCH__)
#include "port/switch/SwitchImpl.h"
#elif defined(__WIIU__)
#include "port/wiiu/WiiUImpl.h"
#endif

namespace LUS {
std::weak_ptr<Context> Context::mContext;

std::shared_ptr<Context> Context::GetInstance() {
    return mContext.lock();
}

Context::~Context() {
    SPDLOG_TRACE("destruct context");
    // Explicitly destructing everything so that logging is done last.
    mAudio = nullptr;
    GetWindow()->SaveWindowSizeToConfig(GetConfig());
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

std::shared_ptr<Context> Context::CreateInstance(const std::string name, const std::string shortName,
                                                 const std::string configFilePath,
                                                 const std::vector<std::string>& otrFiles,
                                                 const std::unordered_set<uint32_t>& validHashes,
                                                 uint32_t reservedThreadCount) {
    if (mContext.expired()) {
        auto shared = std::make_shared<Context>(name, shortName, configFilePath);
        mContext = shared;
        shared->Init(otrFiles, validHashes, reservedThreadCount);
        return shared;
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
    : mName(std::move(name)), mShortName(std::move(shortName)), mConfigFilePath(std::move(configFilePath)) {
}

void Context::Init(const std::vector<std::string>& otrFiles, const std::unordered_set<uint32_t>& validHashes,
                   uint32_t reservedThreadCount) {
    InitLogging();
    InitConfiguration();
    InitConsoleVariables();
    InitResourceManager(otrFiles, validHashes, reservedThreadCount);
    InitControlDeck();
    InitCrashHandler();
    InitConsole();
    InitWindow();
    InitAudio();
}

void Context::InitLogging() {
    if (GetLogger() != nullptr) {
        return;
    }

    try {
        // Setup Logging
        spdlog::init_thread_pool(8192, 1);
        std::vector<spdlog::sink_ptr> sinks;

#if (!defined(_WIN32) && !defined(__WIIU__)) || defined(_DEBUG)
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

#ifndef __WIIU__
        auto logPath = GetPathRelativeToAppDirectory(("logs/" + GetName() + ".log"));
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logPath, 1024 * 1024 * 10, 10);
#ifdef _DEBUG
        fileSink->set_level(spdlog::level::trace);
#else
        fileSink->set_level(spdlog::level::debug);
#endif
        sinks.push_back(fileSink);
#endif

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

#ifndef __WIIU__
        GetLogger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%@] [%l] %v");
#else
        GetLogger()->set_pattern("[%s:%#] [%l] %v");
#endif

        spdlog::register_logger(GetLogger());
        spdlog::set_default_logger(GetLogger());
    } catch (const spdlog::spdlog_ex& ex) { std::cout << "Log initialization failed: " << ex.what() << std::endl; }
}

void Context::InitConfiguration() {
    if (GetConfig() != nullptr) {
        return;
    }

    mConfig = std::make_shared<Config>(GetPathRelativeToAppDirectory(GetConfigFilePath()));
}

void Context::InitConsoleVariables() {
    if (GetConsoleVariables() != nullptr) {
        return;
    }

    mConsoleVariables = std::make_shared<ConsoleVariable>();
}

void Context::InitResourceManager(const std::vector<std::string>& otrFiles,
                                  const std::unordered_set<uint32_t>& validHashes, uint32_t reservedThreadCount) {
    if (GetResourceManager() != nullptr) {
        return;
    }

    mMainPath = GetConfig()->GetString("Game.Main Archive", GetAppDirectoryPath());
    mPatchesPath = GetConfig()->GetString("Game.Patches Archive", GetAppDirectoryPath() + "/mods");
    if (otrFiles.empty()) {
        std::vector<std::string> paths = std::vector<std::string>();
        paths.push_back(mMainPath);
        paths.push_back(mPatchesPath);

        mResourceManager = std::make_shared<ResourceManager>();
        GetResourceManager()->Init(paths, validHashes, reservedThreadCount);
    } else {
        mResourceManager = std::make_shared<ResourceManager>();
        GetResourceManager()->Init(otrFiles, validHashes, reservedThreadCount);
    }

    if (!GetResourceManager()->DidLoadSuccessfully()) {
#if defined(__SWITCH__)
        printf("Main OTR file not found!\n");
#elif defined(__WIIU__)
        LUS::WiiU::ThrowMissingOTR(mMainPath.c_str());
#else
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OTR file not found",
                                 "Main OTR file not found. Please generate one", nullptr);
        SPDLOG_ERROR("Main OTR file not found!");
#endif
        return;
    }
#ifdef __SWITCH__
    LUS::Switch::Init(PostInitPhase);
#endif
}

void Context::InitControlDeck(std::vector<uint16_t> additionalBitmasks) {
    if (GetControlDeck() != nullptr) {
        return;
    }

    mControlDeck = std::make_shared<ControlDeck>(additionalBitmasks);
}

void Context::InitCrashHandler() {
    if (GetCrashHandler() != nullptr) {
        return;
    }

    mCrashHandler = std::make_shared<CrashHandler>();
}

void Context::InitAudio() {
    if (GetAudio() != nullptr) {
        return;
    }

    mAudio = std::make_shared<Audio>();
    GetAudio()->Init();
}

void Context::InitConsole() {
    if (GetConsole() != nullptr) {
        return;
    }

    mConsole = std::make_shared<Console>();
    GetConsole()->Init();
}

void Context::InitWindow(std::shared_ptr<GuiWindow> customInputEditorWindow) {
    if (GetWindow() != nullptr) {
        return;
    }

    mWindow = std::make_shared<Window>(customInputEditorWindow);
    GetWindow()->Init();
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

std::string Context::GetConfigFilePath() {
    return mConfigFilePath;
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
        int lastSlash = progpath.find_last_of("/");
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

#if defined(__linux__) || defined(__APPLE__)
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

} // namespace LUS
