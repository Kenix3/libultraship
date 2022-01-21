#include "Window.h"
#include "spdlog/spdlog.h"
#include "KeyboardController.h"
#include "SDLController.h"
#include "GlobalCtx2.h"
#include "DisplayList.h"
#include "ResourceMgr.h"
#include "Texture.h"
#include "Lib/Fast3D/gfx_pc.h"
#include "Lib/Fast3D/gfx_sdl.h"
#include "Lib/Fast3D/gfx_opengl.h"
#include "stox.h"
#include <SDL2/SDL.h>
#include <map>
#include <string>

extern "C" {
    struct OSMesgQueue;

    uint8_t __osMaxControllers = MAXCONTROLLERS;

    int32_t osContInit(OSMesgQueue* mq, uint8_t* controllerBits, OSContStatus* status) {
        std::shared_ptr<Ship::ConfigFile> pConf = Ship::GlobalCtx2::GetInstance()->GetConfig();
        Ship::ConfigFile& Conf = *pConf.get();

        if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
            SPDLOG_ERROR("Failed to initialize SDL game controllers ({})", SDL_GetError());
            exit(EXIT_FAILURE);
        }

        // TODO: THis for loop is debug. Burn it with fire.
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                // Get the GUID from SDL
                char buf[33];
                SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), buf, sizeof(buf));
                auto guid = std::string(buf);
                auto name = std::string(SDL_GameControllerNameForIndex(i));

                SPDLOG_INFO("Found Controller \"{}\" with ID \"{}\"", name, guid);
            }
        }

        for (int32_t i = 0; i < __osMaxControllers; i++) {
            std::string ControllerType = Conf["CONTROLLERS"]["CONTROLLER " + std::to_string(i+1)];
            mINI::INIStringUtil::toLower(ControllerType);

            if (ControllerType == "keyboard") {
                Ship::Controller* pad = new Ship::KeyboardController(i);
                Ship::Window::Controllers[i] = std::shared_ptr<Ship::Controller>(pad);
            } else if (ControllerType == "usb") {
                // TODO: switch controller types based on the window used by Fast3D
                Ship::Controller* pad = new Ship::SDLController(i);
                Ship::Window::Controllers[i] = std::shared_ptr<Ship::Controller>(pad);
            } else if (ControllerType == "unplugged") {
                // Do nothing for unplugged controllers
            } else {
                SPDLOG_ERROR("Invalid Controller Type: {}", ControllerType);
            }
        }

        for (size_t i = 0; i < __osMaxControllers; i++) {
            if (Ship::Window::Controllers[i] != nullptr) {
                *controllerBits |= 1 << i;
            }
        }

        return 0;
    }

    int32_t osContStartReadData(OSMesgQueue* mesg) {
        return 0;
    }

    void osContGetReadData(OSContPad* pad) {
        pad->button = 0;
        pad->stick_x = 0;
        pad->stick_y = 0;
        pad->err_no = 0;

        for (size_t i = 0; i < __osMaxControllers; i++) {
            if (Ship::Window::Controllers[i] != nullptr) {
                Ship::Window::Controllers[i]->Read(&pad[i]);
            }
        }
    }

    char* ResourceMgr_GetNameByCRC(uint64_t crc, char* alloc) {
        std::string hashStr = Ship::GlobalCtx2::GetInstance()->GetResourceManager()->HashToString(crc);
        strcpy(alloc, hashStr.c_str());
        return (char*)hashStr.c_str();
    }

    Vtx* ResourceMgr_LoadVtxByCRC(uint64_t crc)
    {
        std::string hashStr = Ship::GlobalCtx2::GetInstance()->GetResourceManager()->HashToString(crc);

        if (hashStr != "") {
            return (Vtx*)Ship::GlobalCtx2::GetInstance()->GetResourceManager()->LoadFile(hashStr)->buffer.get();
        }
        else {
            return nullptr;
        }
    }

    Gfx* ResourceMgr_LoadGfxByCRC(uint64_t crc) 
    {
        std::string hashStr = Ship::GlobalCtx2::GetInstance()->GetResourceManager()->HashToString(crc);

        if (hashStr != "") {
            auto res = std::static_pointer_cast<Ship::DisplayList>(Ship::GlobalCtx2::GetInstance()->GetResourceManager()->LoadResource(hashStr));
            return (Gfx*)&res->instructions[0];
        }
        else {
            return nullptr;
        }
    }

    char* ResourceMgr_LoadTexByCRC(uint64_t crc) 
    {
        std::string hashStr = Ship::GlobalCtx2::GetInstance()->GetResourceManager()->HashToString(crc);

        if (hashStr != "") 
        {
            auto res = (Ship::Texture*)Ship::GlobalCtx2::GetInstance()->GetResourceManager()->LoadResource(hashStr).get();
            return (char*)res->imageData;
        }
        else {
            return nullptr;
        }
    }

    char* ResourceMgr_LoadTexByName(char* texPath) 
    {
        auto res = (Ship::Texture*)Ship::GlobalCtx2::GetInstance()->GetResourceManager()->LoadResource(texPath).get();
        return (char*)res->imageData;
    }
}

extern "C" struct GfxRenderingAPI gfx_opengl_api;
extern "C" struct GfxWindowManagerAPI gfx_sdl;
extern "C" void SetWindowManager(GfxWindowManagerAPI** WmApi, GfxRenderingAPI** RenderingApi);
extern "C" void ToggleConsole();

namespace Ship {
    std::shared_ptr<Ship::Controller> Window::Controllers[MAXCONTROLLERS] = { nullptr };
    int32_t Window::lastScancode;

    Window::Window(std::shared_ptr<GlobalCtx2> Context) : Context(Context) {
        WmApi = nullptr;
        RenderingApi = nullptr;
        bIsFullscreen = false;
        dwWidth = 320;
        dwHeight = 240;
    }

    Window::~Window() {
        SPDLOG_INFO("destruct window");
    }

    void Window::Init() {
        std::shared_ptr<ConfigFile> pConf = GlobalCtx2::GetInstance()->GetConfig();
        ConfigFile& Conf = *pConf.get();

        SetWindowManager(&WmApi, &RenderingApi);
        bIsFullscreen = Ship::stob(Conf["WINDOW"]["FULLSCREEN"]);
        dwWidth = Ship::stoi(Conf["WINDOW"]["WINDOW WIDTH"], 320);
        dwHeight = Ship::stoi(Conf["WINDOW"]["WINDOW HEIGHT"], 240);
        dwWidth = Ship::stoi(Conf["WINDOW"]["FULLSCREEN WIDTH"], 1920);
        dwHeight = Ship::stoi(Conf["WINDOW"]["FULLSCREEN HEIGHT"], 1080);

        gfx_init(WmApi, RenderingApi, GetContext()->GetName().c_str(), bIsFullscreen);
        WmApi->set_fullscreen_changed_callback(Window::OnFullscreenChanged);
        WmApi->set_keyboard_callbacks(Window::KeyDown, Window::KeyUp, Window::AllKeysUp);
    }

    void Window::RunCommands(Gfx* Commands) {
        gfx_start_frame();
        gfx_run(Commands);
        gfx_end_frame();
    }

    void Window::SetFrameDivisor(int divisor) {
        gfx_set_framedivisor(divisor);
    }

    void Window::ToggleFullscreen() {
        SetFullscreen(!bIsFullscreen);
    }

    void Window::SetFullscreen(bool bIsFullscreen) {
        this->bIsFullscreen = bIsFullscreen;
        WmApi->set_fullscreen(bIsFullscreen);
    }

    void Window::MainLoop(void (*MainFunction)(void)) {
        WmApi->main_loop(MainFunction);
    }

    bool Window::KeyDown(int32_t dwScancode) {
        bool bIsProcessed = false;
        for (size_t i = 0; i < __osMaxControllers; i++) {
            KeyboardController* pad = dynamic_cast<KeyboardController*>(Ship::Window::Controllers[i].get());
            if (pad != nullptr) {
                if (pad->PressButton(dwScancode)) {
                    bIsProcessed = true;
                }
            }
        }

        lastScancode = dwScancode;

        return bIsProcessed;
    }

    bool Window::KeyUp(int32_t dwScancode) {
        std::shared_ptr<ConfigFile> pConf = GlobalCtx2::GetInstance()->GetConfig();
        ConfigFile& Conf = *pConf.get();

        if (dwScancode == Ship::stoi(Conf["KEYBOARD SHORTCUTS"]["KEY_FULLSCREEN"])) {
            GlobalCtx2::GetInstance()->GetWindow()->ToggleFullscreen();
        }

        if (dwScancode == Ship::stoi(Conf["KEYBOARD SHORTCUTS"]["KEY_CONSOLE"])) {
            ToggleConsole();
        }

        bool bIsProcessed = false;
        for (size_t i = 0; i < __osMaxControllers; i++) {
            KeyboardController* pad = dynamic_cast<KeyboardController*>(Ship::Window::Controllers[i].get());
            if (pad != nullptr) {
                if (pad->ReleaseButton(dwScancode)) {
                    bIsProcessed = true;
                }
            }
        }

        return bIsProcessed;
    }

    void Window::AllKeysUp(void) {
        for (size_t i = 0; i < __osMaxControllers; i++) {
            KeyboardController* pad = dynamic_cast<KeyboardController*>(Ship::Window::Controllers[i].get());
            if (pad != nullptr) {
                pad->ReleaseAllButtons();
            }
        }
    }

    void Window::OnFullscreenChanged(bool bIsFullscreen) {
        std::shared_ptr<ConfigFile> pConf = GlobalCtx2::GetInstance()->GetConfig();
        ConfigFile& Conf = *pConf.get();

        GlobalCtx2::GetInstance()->GetWindow()->bIsFullscreen = bIsFullscreen;
        Conf["WINDOW"]["FULLSCREEN"] = std::to_string(GlobalCtx2::GetInstance()->GetWindow()->IsFullscreen());
    }

    uint32_t Window::GetCurrentWidth() {
        WmApi->get_dimensions(&dwWidth, &dwHeight);
        return dwWidth;
    }

    uint32_t Window::GetCurrentHeight() {
        WmApi->get_dimensions(&dwWidth, &dwHeight);
        return dwHeight;
    }
}