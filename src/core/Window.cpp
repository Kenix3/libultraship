#include <string>
#include <chrono>
#include <fstream>
#include <iostream>
#include "core/Window.h"
#include "resource/ResourceMgr.h"
#include "controller/KeyboardController.h"
#include "UltraController.h"
#include "resource/types/DisplayList.h"
#include "menu/Console.h"
#include "resource/types/Array.h"
#include "resource/types/Texture.h"
#include "resource/types/Blob.h"
#include "resource/types/Matrix.h"
#include "misc/Hooks.h"
#include <string>
#include "graphic/Fast3D/gfx_pc.h"
#include "graphic/Fast3D/gfx_sdl.h"
#include "graphic/Fast3D/gfx_dxgi.h"
#include "graphic/Fast3D/gfx_glx.h"
#include "graphic/Fast3D/gfx_opengl.h"
#include "graphic/Fast3D/gfx_direct3d11.h"
#include "graphic/Fast3D/gfx_direct3d12.h"
#include "graphic/Fast3D/gfx_wiiu.h"
#include "graphic/Fast3D/gfx_gx2.h"
#include "graphic/Fast3D/gfx_rendering_api.h"
#include "graphic/Fast3D/gfx_window_manager_api.h"
#include <SDL2/SDL.h>
#include "menu/ImGuiImpl.h"
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "log/spd/sohconsole_sink.h"
#include "graphic/Fast3D/U64/PR/ultra64/gbi.h"
#ifdef __APPLE__
#include "misc/OSXFolderManager.h"
#elif defined(__SWITCH__)
#include "port/switch/SwitchImpl.h"
#elif defined(__WIIU__)
#include "port/wiiu/WiiUImpl.h"
#endif

#define LOAD_TEX(texPath) \
    static_cast<Ship::Texture*>(Ship::Window::GetInstance()->GetResourceManager()->LoadResource(texPath).get());

extern "C" {
struct OSMesgQueue;

uint8_t __osMaxControllers = MAXCONTROLLERS;

int32_t osContInit(OSMesgQueue* mq, uint8_t* controllerBits, OSContStatus* status) {
    *controllerBits = 0;

#ifndef __WIIU__
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        SPDLOG_ERROR("Failed to initialize SDL game controllers ({})", SDL_GetError());
        exit(EXIT_FAILURE);
    }

#ifndef __SWITCH__
    const char* controllerDb = "gamecontrollerdb.txt";
    int mappingsAdded = SDL_GameControllerAddMappingsFromFile(controllerDb);
    if (mappingsAdded >= 0) {
        SPDLOG_INFO("Added SDL game controllers from \"{}\" ({})", controllerDb, mappingsAdded);
    } else {
        SPDLOG_ERROR("Failed add SDL game controller mappings from \"{}\" ({})", controllerDb, SDL_GetError());
    }
#endif
#endif

    Ship::Window::GetInstance()->GetControlDeck()->Init(controllerBits);

    return 0;
}

int32_t osContStartReadData(OSMesgQueue* mesg) {
    return 0;
}

void osContGetReadData(OSContPad* pad) {
    pad->button = 0;
    pad->stick_x = 0;
    pad->stick_y = 0;
    pad->right_stick_x = 0;
    pad->right_stick_y = 0;
    pad->err_no = 0;
    pad->gyro_x = 0;
    pad->gyro_y = 0;

    if (SohImGui::GetInputEditor()->IsOpened()) {
        return;
    }

    Ship::Window::GetInstance()->GetControlDeck()->WriteToPad(pad);
    Ship::ExecuteHooks<Ship::ControllerRead>(pad);
}

const char* ResourceMgr_GetNameByCRC(uint64_t crc) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(crc);
    return hashStr != nullptr ? hashStr->c_str() : nullptr;
}

Vtx* ResourceMgr_LoadVtxByCRC(uint64_t crc) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(crc);

    if (hashStr != nullptr) {
        auto res = std::static_pointer_cast<Ship::Array>(
            Ship::Window::GetInstance()->GetResourceManager()->LoadResource(hashStr->c_str()));
        return (Vtx*)res->vertices.data();
    }

    return nullptr;
}

int32_t* ResourceMgr_LoadMtxByCRC(uint64_t crc) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(crc);

    if (hashStr != nullptr) {
        auto res = std::static_pointer_cast<Ship::Matrix>(
            Ship::Window::GetInstance()->GetResourceManager()->LoadResource(hashStr->c_str()));
        return (int32_t*)res->mtx.data();
    }

    return nullptr;
}

Gfx* ResourceMgr_LoadGfxByCRC(uint64_t crc) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(crc);

    if (hashStr != nullptr) {
        auto res = std::static_pointer_cast<Ship::DisplayList>(
            Ship::Window::GetInstance()->GetResourceManager()->LoadResource(hashStr->c_str()));
        return (Gfx*)&res->instructions[0];
    } else {
        return nullptr;
    }
}

char* ResourceMgr_LoadTexByCRC(uint64_t crc) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(crc);

    if (hashStr != nullptr) {
        const auto res = LOAD_TEX(hashStr->c_str());
        Ship::ExecuteHooks<Ship::LoadTexture>(hashStr->c_str(), &res->imageData);

        return reinterpret_cast<char*>(res->imageData);
    } else {
        return nullptr;
    }
}

void ResourceMgr_RegisterResourcePatch(uint64_t hash, uint32_t instrIndex, uintptr_t origData) {
    const std::string* hashStr = Ship::Window::GetInstance()->GetResourceManager()->HashToString(hash);

    if (hashStr != nullptr) {
        auto res =
            (Ship::Texture*)Ship::Window::GetInstance()->GetResourceManager()->LoadResource(hashStr->c_str()).get();

        Ship::Patch patch;
        patch.Crc = hash;
        patch.Index = instrIndex;
        patch.OrigData = origData;

        res->Patches.push_back(patch);
    }
}

char* ResourceMgr_LoadTexByName(char* texPath) {
    const auto res = LOAD_TEX(texPath);
    Ship::ExecuteHooks<Ship::LoadTexture>(texPath, &res->imageData);
    return (char*)res->imageData;
}

uint16_t ResourceMgr_LoadTexWidthByName(char* texPath) {
    const auto res = LOAD_TEX(texPath);
    if (res != nullptr) {
        return res->width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t ResourceMgr_LoadTexHeightByName(char* texPath) {
    const auto res = LOAD_TEX(texPath);
    if (res != nullptr) {
        return res->height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint32_t ResourceMgr_LoadTexSizeByName(char* texPath) {
    const auto res = LOAD_TEX(texPath);
    if (res != nullptr) {
        return res->imageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

void ResourceMgr_WriteTexS16ByName(char* texPath, size_t index, s16 value) {
    const auto res = LOAD_TEX(texPath);

    if (res != nullptr) {
        if ((index * 2) < res->imageDataSize) {
            ((s16*)res->imageData)[index] = value;
        } else {
            // Dangit Morita
            int bp = 0;
        }
    }
}

char* ResourceMgr_LoadBlobByName(char* blobPath) {
    auto res = (Ship::Blob*)Ship::Window::GetInstance()->GetResourceManager()->LoadResource(blobPath).get();
    return (char*)res->data.data();
}

/* Should these go in their own file?*/
uint64_t osGetTime(void) {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

uint32_t osGetCount(void) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
}

namespace Ship {

std::weak_ptr<Window> Window::mContext;

std::shared_ptr<Window> Window::GetInstance() {
    return mContext.lock();
}

std::shared_ptr<Window> Window::CreateInstance(const std::string name, const std::vector<std::string>& otrFiles,
                                               const std::unordered_set<uint32_t>& validHashes) {
    if (mContext.expired()) {
        auto shared = std::make_shared<Window>(name);
        mContext = shared;
        shared->Initialize(otrFiles, validHashes);
        return shared;
    }

    SPDLOG_DEBUG("Trying to create a context when it already exists. Returning existing.");

    return GetInstance();
}

Window::Window(std::string Name)
    : mName(std::move(Name)), mAudioPlayer(nullptr), mControlDeck(nullptr), mResourceManager(nullptr), mLogger(nullptr),
      mConfig(nullptr) {
    mWindowManagerApi = nullptr;
    mRenderingApi = nullptr;
    mIsFullscreen = false;
    mWidth = 320;
    mHeight = 240;
}

Window::~Window() {
    SPDLOG_DEBUG("destruct window");
}

void Window::CreateDefaults() {
    if (GetConfig()->isNewInstance) {
        GetConfig()->setInt("Window.Width", 640);
        GetConfig()->setInt("Window.Height", 480);

        GetConfig()->setString("Window.GfxBackend", "");
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

void Window::Initialize(const std::vector<std::string>& otrFiles, const std::unordered_set<uint32_t>& validHashes) {
    InitializeLogging();
    InitializeConfiguration();
    InitializeResourceManager(otrFiles, validHashes);
    CreateDefaults();
    InitializeControlDeck();

    mIsFullscreen = GetConfig()->getBool("Window.Fullscreen.Enabled", false);

    if (mIsFullscreen) {
        mWidth = GetConfig()->getInt("Window.Fullscreen.Width", 1920);
        mHeight = GetConfig()->getInt("Window.Fullscreen.Height", 1080);
    } else {
        mWidth = GetConfig()->getInt("Window.Width", 640);
        mHeight = GetConfig()->getInt("Window.Height", 480);
    }

    mGfxBackend = GetConfig()->getString("Window.GfxBackend");
    InitializeWindowManager();

    mAudioBackend = GetConfig()->getString("Window.AudioBackend");
    InitializeAudioPlayer();

    gfx_init(mWindowManagerApi, mRenderingApi, GetName().c_str(), mIsFullscreen, mWidth, mHeight);
    mWindowManagerApi->set_fullscreen_changed_callback(OnFullscreenChanged);
    mWindowManagerApi->set_keyboard_callbacks(KeyDown, KeyUp, AllKeysUp);

    Ship::RegisterHook<ExitGame>([this]() { mControlDeck->SaveControllerSettings(); });
}

std::string Window::GetAppDirectoryPath() {
#ifdef __APPLE__
    FolderManager folderManager;
    std::string fpath = std::string(folderManager.pathForDirectory(NSApplicationSupportDirectory, NSUserDomainMask));
    fpath.append("/com.shipofharkinian.soh");
    return fpath;
#endif

#ifdef __linux__
    char* fpath = std::getenv("SHIP_HOME");
    if (fpath != NULL)
        return std::string(fpath);
#endif

    return ".";
}

std::string Window::GetPathRelativeToAppDirectory(const char* path) {
    return GetAppDirectoryPath() + "/" + path;
}

void Window::StartFrame() {
    gfx_start_frame();
}

void Window::SetTargetFps(int32_t fps) {
    gfx_set_target_fps(fps);
}

void Window::SetMaximumFrameLatency(int32_t latency) {
    gfx_set_maximum_frame_latency(latency);
}

void Window::GetPixelDepthPrepare(float x, float y) {
    gfx_get_pixel_depth_prepare(x, y);
}

uint16_t Window::GetPixelDepth(float x, float y) {
    return gfx_get_pixel_depth(x, y);
}

void Window::ToggleFullscreen() {
    SetFullscreen(!mIsFullscreen);
}

void Window::SetFullscreen(bool isFullscreen) {
    this->mIsFullscreen = isFullscreen;
    mWindowManagerApi->set_fullscreen(isFullscreen);
}

void Window::SetCursorVisibility(bool visible) {
    mWindowManagerApi->set_cursor_visibility(visible);
}

void Window::MainLoop(void (*MainFunction)(void)) {
    mWindowManagerApi->main_loop(MainFunction);
}

bool Window::KeyUp(int32_t scancode) {
    if (scancode == GetInstance()->GetConfig()->getInt("Shortcuts.Fullscreen", 0x044)) {
        GetInstance()->ToggleFullscreen();
    }

    GetInstance()->SetLastScancode(-1);

    bool isProcessed = false;
    auto controlDeck = GetInstance()->GetControlDeck();
    const auto pad = dynamic_cast<KeyboardController*>(
        controlDeck->GetPhysicalDevice(controlDeck->GetNumPhysicalDevices() - 2).get());
    if (pad != nullptr) {
        if (pad->ReleaseButton(scancode)) {
            isProcessed = true;
        }
    }

    return isProcessed;
}

bool Window::KeyDown(int32_t scancode) {
    bool isProcessed = false;
    auto controlDeck = GetInstance()->GetControlDeck();
    const auto pad = dynamic_cast<KeyboardController*>(
        controlDeck->GetPhysicalDevice(controlDeck->GetNumPhysicalDevices() - 2).get());
    if (pad != nullptr) {
        if (pad->PressButton(scancode)) {
            isProcessed = true;
        }
    }

    GetInstance()->SetLastScancode(scancode);

    return isProcessed;
}

void Window::AllKeysUp(void) {
    auto controlDeck = Window::GetInstance()->GetControlDeck();
    const auto pad = dynamic_cast<KeyboardController*>(
        controlDeck->GetPhysicalDevice(controlDeck->GetNumPhysicalDevices() - 2).get());
    if (pad != nullptr) {
        pad->ReleaseAllButtons();
    }
}

void Window::OnFullscreenChanged(bool isNowFullscreen) {
    std::shared_ptr<Mercury> pConf = Window::GetInstance()->GetConfig();

    Window::GetInstance()->mIsFullscreen = isNowFullscreen;
    pConf->setBool("Window.Fullscreen.Enabled", isNowFullscreen);
    if (isNowFullscreen) {
        bool menuBarOpen = Window::GetInstance()->GetMenuBar();
        Window::GetInstance()->SetCursorVisibility(menuBarOpen);
    } else if (!isNowFullscreen) {
        Window::GetInstance()->SetCursorVisibility(true);
    }
}

uint32_t Window::GetCurrentWidth() {
    mWindowManagerApi->get_dimensions(&mWidth, &mHeight);
    return mWidth;
}

uint32_t Window::GetCurrentHeight() {
    mWindowManagerApi->get_dimensions(&mWidth, &mHeight);
    return mHeight;
}

void Window::InitializeAudioPlayer() {
#ifdef _WIN32
    mAudioPlayer = std::make_shared<WasapiAudioPlayer>();
#elif defined(__linux)
    mAudioPlayer = std::make_shared<PulseAudioPlayer>();
#else
    mAudioPlayer = std::make_shared<SDLAudioPlayer>();
#endif

    // Config can override
#ifdef _WIN32
    if (mAudioBackend == "wasapi") {
        mAudioPlayer = std::make_shared<WasapiAudioPlayer>();
    }
#endif
#if defined(__linux)
    if (mAudioBackend == "pulse") {
        mAudioPlayer = std::make_shared<PulseAudioPlayer>();
    }
#endif
    if (mAudioBackend == "sdl") {
        mAudioPlayer = std::make_shared<SDLAudioPlayer>();
    }
}

void Window::InitializeWindowManager() {
    // First set default
#ifdef ENABLE_OPENGL
    mRenderingApi = &gfx_opengl_api;
#if defined(__linux__) && defined(X11_SUPPORTED)
    // LINUX_TODO:
    // *mWindowManagerApi = &gfx_glx;
    mWindowManagerApi = &gfx_sdl;
#else
    mWindowManagerApi = &gfx_sdl;
#endif
#endif
#ifdef ENABLE_DX12
    mRenderingApi = &gfx_direct3d12_api;
    mWindowManagerApi = &gfx_dxgi_api;
#endif
#ifdef ENABLE_DX11
    mRenderingApi = &gfx_direct3d11_api;
    mWindowManagerApi = &gfx_dxgi_api;
#endif
#ifdef __WIIU__
    mRenderingApi = &gfx_gx2_api;
    mWindowManagerApi = &gfx_wiiu;
#endif

    // Config can override
#ifdef ENABLE_DX11
    if (mGfxBackend == "dx11") {
        mRenderingApi = &gfx_direct3d11_api;
        mWindowManagerApi = &gfx_dxgi_api;
    }
#endif
#ifdef ENABLE_OPENGL
    if (mGfxBackend == "sdl") {
        mRenderingApi = &gfx_opengl_api;
        mWindowManagerApi = &gfx_sdl;
    }
#if defined(__linux__) && defined(X11_SUPPORTED)
    if (mGfxBackend == "glx") {
        mRenderingApi = &gfx_opengl_api;
        mWindowManagerApi = &gfx_glx;
    }
#endif
#endif
}

void Window::InitializeControlDeck() {
    mControlDeck = std::make_shared<ControlDeck>();
}

void Window::InitializeLogging() {
    try {
        // Setup Logging
        spdlog::init_thread_pool(8192, 1);
        std::vector<spdlog::sink_ptr> sinks;

        auto SohConsoleSink = std::make_shared<spdlog::sinks::soh_sink_mt>();
        // SohConsoleSink->set_level(spdlog::level::trace);
        sinks.push_back(SohConsoleSink);

#if (!defined(_WIN32) && !defined(__WIIU__)) || defined(_DEBUG)
#if defined(_DEBUG) && defined(_WIN32)
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
        auto ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        // ConsoleSink->set_level(spdlog::level::trace);
        sinks.push_back(ConsoleSink);
#endif

#ifndef __WIIU__
        auto logPath = GetPathRelativeToAppDirectory(("logs/" + GetName() + ".log").c_str());
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logPath, 1024 * 1024 * 10, 10);
        fileSink->set_level(spdlog::level::trace);
        sinks.push_back(fileSink);
#endif

        mLogger = std::make_shared<spdlog::async_logger>(GetName(), sinks.begin(), sinks.end(), spdlog::thread_pool(),
                                                         spdlog::async_overflow_policy::block);
        GetLogger()->set_level(spdlog::level::trace);

#ifndef __WIIU__
        GetLogger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%@] [%l] %v");
#else
        GetLogger()->set_pattern("[%s:%#] [%l] %v");
#endif

        spdlog::register_logger(GetLogger());
        spdlog::set_default_logger(GetLogger());
    } catch (const spdlog::spdlog_ex& ex) { std::cout << "Log initialization failed: " << ex.what() << std::endl; }
}

void Window::InitializeResourceManager(const std::vector<std::string>& otrFiles,
                                       const std::unordered_set<uint32_t>& validHashes) {
    mMainPath = mConfig->getString("Game.Main Archive", GetAppDirectoryPath());
    mPatchesPath = mConfig->getString("Game.Patches Archive", GetAppDirectoryPath() + "/mods");
    if (otrFiles.empty()) {
        mResourceManager = std::make_shared<ResourceMgr>(GetInstance(), mMainPath, mPatchesPath, validHashes);
    } else {
        mResourceManager = std::make_shared<ResourceMgr>(GetInstance(), otrFiles, validHashes);
    }

    if (!mResourceManager->DidLoadSuccessfully()) {
#if defined(__SWITCH__)
        printf("Main OTR file not found!\n");
#elif defined(__WIIU__)
        Ship::WiiU::ThrowMissingOTR(mMainPath.c_str());
#else
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OTR file not found",
                                 "Main OTR file not found. Please generate one", nullptr);
        SPDLOG_ERROR("Main OTR file not found!");
#endif
        exit(1);
    }
#ifdef __SWITCH__
    Ship::Switch::Init(PostInitPhase);
#endif
}

void Window::InitializeConfiguration() {
    mConfig = std::make_shared<Mercury>(GetPathRelativeToAppDirectory("shipofharkinian.json"));
}

void Window::WriteSaveFile(const std::filesystem::path& savePath, const uintptr_t addr, void* dramAddr,
                           const size_t size) {
    std::ofstream saveFile = std::ofstream(savePath, std::fstream::in | std::fstream::out | std::fstream::binary);
    saveFile.seekp(addr);
    saveFile.write((char*)dramAddr, size);
    saveFile.close();
}

void Window::ReadSaveFile(std::filesystem::path savePath, uintptr_t addr, void* dramAddr, size_t size) {
    std::ifstream saveFile = std::ifstream(savePath, std::fstream::in | std::fstream::out | std::fstream::binary);

    // If the file doesn't exist, initialize DRAM
    if (saveFile.good()) {
        saveFile.seekg(addr);
        saveFile.read((char*)dramAddr, size);
    } else {
        memset(dramAddr, 0, size);
    }

    saveFile.close();
}

bool Window::IsFullscreen() {
    return mIsFullscreen;
}

uint32_t Window::GetMenuBar() {
    return mMenuBar;
}

void Window::SetMenuBar(uint32_t menuBar) {
    this->mMenuBar = menuBar;
}

std::string Window::GetName() {
    return mName;
}

std::shared_ptr<ControlDeck> Window::GetControlDeck() {
    return mControlDeck;
}

std::shared_ptr<AudioPlayer> Window::GetAudioPlayer() {
    return mAudioPlayer;
}

std::shared_ptr<ResourceMgr> Window::GetResourceManager() {
    return mResourceManager;
}

std::shared_ptr<Mercury> Window::GetConfig() {
    return mConfig;
}

std::shared_ptr<spdlog::logger> Window::GetLogger() {
    return mLogger;
}

const char* Window::GetKeyName(int32_t scancode) {
    return mWindowManagerApi->get_key_name(scancode);
}

int32_t Window::GetLastScancode() {
    return mLastScancode;
}

void Window::SetLastScancode(int32_t scanCode) {
    mLastScancode = scanCode;
}
} // namespace Ship
