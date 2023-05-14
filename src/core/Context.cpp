#include "Context.h"
#include <iostream>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "log/spd/ConsoleSink.h"
#include "misc/Hooks.h"

#ifdef __APPLE__
#include "misc/OSXFolderManager.h"
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

Context::Context(std::string name, std::string shortName, std::string configFilePath)
    : mName(std::move(name)), mShortName(std::move(shortName)), mConfigFilePath(std::move(configFilePath)) {
}

void Context::CreateDefaultSettings() {
    if (GetConfig()->isNewInstance) {
        GetConfig()->setInt("Window.Width", 640);
        GetConfig()->setInt("Window.Height", 480);

        GetConfig()->setString("Window.GfxBackend", "");
        GetConfig()->setString("Window.GfxApi", "");
        GetConfig()->setString("Window.AudioBackend", "");

        GetConfig()->setBool("Window.Fullscreen.Enabled", false);
        GetConfig()->setInt("Window.Fullscreen.Width", 1920);
        GetConfig()->setInt("Window.Fullscreen.Height", 1080);

        GetConfig()->setString("Game.SaveName", "");
        GetConfig()->setString("Game.Main Archive", "");
        GetConfig()->setString("Game.Patches Archive", "");

        GetConfig()->setInt("Shortcuts.Fullscreen", 0x044);
        GetConfig()->setInt("Shortcuts.Console", 0x029);
        GetConfig()->save();
    }
}

void Context::Init(const std::vector<std::string>& otrFiles, const std::unordered_set<uint32_t>& validHashes,
                   uint32_t reservedThreadCount) {
    InitLogging();
    InitConfiguration();
    InitConsoleVariables();
    InitResourceManager(otrFiles, validHashes, reservedThreadCount);
    CreateDefaultSettings();
    InitControlDeck();
    InitCrashHandler();
    InitAudioPlayer(DetermineAudioBackendFromConfig());
    InitConsole();
    InitWindow();

    LUS::RegisterHook<ExitGame>([this]() { mControlDeck->SaveSettings(); });
}

void Context::InitLogging() {
    try {
        // Setup Logging
        spdlog::init_thread_pool(8192, 1);
        std::vector<spdlog::sink_ptr> sinks;

        auto consoleSink = std::make_shared<spdlog::sinks::lus_sink_mt>();
        // consoleSink->set_level(spdlog::level::trace);
        sinks.push_back(consoleSink);

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
    mConfig = std::make_shared<Mercury>(GetPathRelativeToAppDirectory(GetConfigFilePath()));
}

void Context::InitConsoleVariables() {
    mConsoleVariables = std::make_shared<ConsoleVariable>();
}

void Context::InitResourceManager(const std::vector<std::string>& otrFiles,
                                  const std::unordered_set<uint32_t>& validHashes, uint32_t reservedThreadCount) {
    mMainPath = GetConfig()->getString("Game.Main Archive", GetAppDirectoryPath());
    mPatchesPath = mConfig->getString("Game.Patches Archive", GetAppDirectoryPath() + "/mods");
    if (otrFiles.empty()) {
        mResourceManager =
            std::make_shared<ResourceManager>(GetInstance(), mMainPath, mPatchesPath, validHashes, reservedThreadCount);
    } else {
        mResourceManager = std::make_shared<ResourceManager>(GetInstance(), otrFiles, validHashes, reservedThreadCount);
    }

    if (!mResourceManager->DidLoadSuccessfully()) {
#if defined(__SWITCH__)
        printf("Main OTR file not found!\n");
#elif defined(__WIIU__)
        LUS::WiiU::ThrowMissingOTR(mMainPath.c_str());
#else
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OTR file not found",
                                 "Main OTR file not found. Please generate one", nullptr);
        SPDLOG_ERROR("Main OTR file not found!");
#endif
        mOtrFileExists = false;
        return;
    }
    mOtrFileExists = true;
#ifdef __SWITCH__
    LUS::Switch::Init(PostInitPhase);
#endif
}

void Context::InitControlDeck() {
    mControlDeck = std::make_shared<ControlDeck>();
}

void Context::InitCrashHandler() {
    mCrashHandler = std::make_shared<CrashHandler>();
}

void Context::InitAudioPlayer(AudioBackend backend) {
    SetAudioBackend(backend);
    switch (GetAudioBackend()) {
#ifdef _WIN32
        case AudioBackend::WASAPI:
            mAudioPlayer = std::make_shared<WasapiAudioPlayer>();
            break;
#endif
#if defined(__linux)
        case AudioBackend::PULSE:
            mAudioPlayer = std::make_shared<PulseAudioPlayer>();
            break;
#endif
        default:
            mAudioPlayer = std::make_shared<SDLAudioPlayer>();
    }

    if (mAudioPlayer) {
        if (!mAudioPlayer->Init()) {
            // Failed to initialize system audio player.
            // Fallback to SDL if the native system player does not work.
            SetAudioBackend(AudioBackend::SDL);
            mAudioPlayer = std::make_shared<SDLAudioPlayer>();
            mAudioPlayer->Init();
        }
    }
}

void Context::InitConsole() {
    mConsole = std::make_shared<Console>();
    GetConsole()->Init();
}

void Context::InitWindow() {
    mWindow = std::make_shared<Window>(GetInstance());
    GetWindow()->Init();
}

std::shared_ptr<ConsoleVariable> Context::GetConsoleVariables() {
    return mConsoleVariables;
}

std::shared_ptr<spdlog::logger> Context::GetLogger() {
    return mLogger;
}

std::shared_ptr<Mercury> Context::GetConfig() {
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

std::shared_ptr<AudioPlayer> Context::GetAudioPlayer() {
    return mAudioPlayer;
}

std::shared_ptr<Window> Context::GetWindow() {
    return mWindow;
}

std::shared_ptr<Console> Context::GetConsole() {
    return mConsole;
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

AudioBackend Context::DetermineAudioBackendFromConfig() {
    std::string backendName = Context::GetInstance()->GetConfig()->getString("Window.AudioBackend");
    if (backendName == "wasapi") {
        return AudioBackend::WASAPI;
    } else if (backendName == "pulse") {
        return AudioBackend::PULSE;
    } else if (backendName == "sdl") {
        return AudioBackend::SDL;
    } else {
#ifdef _WIN32
        return AudioBackend::WASAPI;
#elif defined(__linux)
        return AudioBackend::PULSE;
#else
        return AudioBackend::SDL;
#endif
    }
}

std::string Context::DetermineAudioBackendNameFromBackend(AudioBackend backend) {
    switch (backend) {
        case AudioBackend::WASAPI:
            return "wasapi";
        case AudioBackend::PULSE:
            return "pulse";
        case AudioBackend::SDL:
            return "sdl";
        default:
            return "";
    }
}

AudioBackend Context::GetAudioBackend() {
    return mAudioBackend;
}

void Context::SetAudioBackend(AudioBackend backend) {
    std::string audioBackendName = DetermineAudioBackendNameFromBackend(backend);
    Context::GetInstance()->GetConfig()->setString("Window.AudioBackend", audioBackendName);
    mAudioBackend = backend;
}

std::string Context::GetAppBundlePath() {
#ifdef __APPLE__
    FolderManager folderManager;
    return folderManager.getMainBundlePath();
#endif

#ifdef __linux__
    char* fpath = std::getenv("SHIP_BIN_DIR");
    if (fpath != NULL) {
        return std::string(fpath);
    }
#endif

    return ".";
}

std::string Context::GetAppDirectoryPath() {
#if defined(__linux__) || defined(__APPLE__)
    char* fpath = std::getenv("SHIP_HOME");
    if (fpath != NULL) {
        return std::string(fpath);
    }
#endif

    return ".";
}

std::string Context::GetPathRelativeToAppBundle(const std::string path) {
    return GetAppBundlePath() + "/" + path;
}

std::string Context::GetPathRelativeToAppDirectory(const std::string path) {
    return GetAppDirectoryPath() + "/" + path;
}

bool Context::DoesOtrFileExist() {
    return mOtrFileExists;
}
} // namespace LUS