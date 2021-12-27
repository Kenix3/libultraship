#include "OTRInput.h"
#include "spdlog/spdlog.h"
#include "OTRController.h"
#include "KeyboardController.h"
#include "UltraController.h"
#include "Lib/Fast3D/gfx_pc.h"
#include "Lib/Fast3D/gfx_sdl.h"
#include "Lib/Fast3D/gfx_opengl.h"
#include <map>

extern "C" {
    struct OSMesgQueue;

    uint8_t __osMaxControllers = MAXCONTROLLERS;
    std::shared_ptr<OtrLib::OTRController> controllers[MAXCONTROLLERS] = { nullptr };

    int32_t osContInit(OSMesgQueue* mq, uint8_t* controllerBits, OSContStatus* status) {

        // TODO: Configuration should determine the type of controller and which are plugged in. Can also read from SDL to figure out if any controllers are plugged in.
        OtrLib::OTRController* pad = new OtrLib::KeyboardController(0);
        controllers[0] = std::make_shared<OtrLib::OTRController>(*pad);

        for (size_t i = 0; i < __osMaxControllers; i++) {
            if (controllers[i] != nullptr) {
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
            if (controllers[i] != nullptr) {
                controllers[i]->Read(&pad[i]);
            }
        }
    }
}

namespace OtrLib {
    static std::shared_ptr<OTRInput> Input = nullptr;

    OTRInput::OTRInput() : bIsRunning(false) {
        std::thread thread(*this);
        Thread = &thread;
    }

    void OTRInput::operator()() {
        spdlog::info("Starting input thread.");

        // Setup SDL
        if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
            spdlog::error("({}) Failed to initialize SDL", SDL_GetError());
            return;
        }

        bIsRunning = true;

        // Run the input loop.
        while (bIsRunning) {

            SDL_Event e;
            while (SDL_PollEvent(&e) != 0) {
                HandleEvent(e);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Kill SDL
        SDL_Quit();
        spdlog::info("Ending input thread.");
    }

    void OTRInput::HandleEvent(SDL_Event e) {
        // TODO: All of this is debugging code right now.
        switch (e.type) {
            case SDL_KEYDOWN:
                SDL_Keycode key = e.key.keysym.sym;
                SDL_Scancode scancode = SDL_GetScancodeFromKey(key);
                bool bIsOnController = ((KeyboardController*)controllers[0].get())->ButtonMapping.count(scancode);
                spdlog::info("Key Pressed: {} ::: bIsOnController: {}", SDL_GetKeyName(key), bIsOnController);
                break;
        }
    }

    bool OTRInput::StartInput() {
        if (Input == nullptr) {
            Input = std::make_shared<OTRInput>(OTRInput());
            return true;
        } else {
            spdlog::error("Failed to create OTRInput: Input already exists");
            return false;
        }
    }

    bool OTRInput::StopInput() {
        spdlog::info("Shutting down input thread.");

        // Shut down the input thread
        Input->bIsRunning = false;
        if (Input->Thread->joinable())
            Input->Thread->join();
        Input = nullptr;

        // Invalidate the Controller classes.
        for (size_t i = 0; i < __osMaxControllers; i++) {
            controllers[i] = nullptr;
        }

        spdlog::info("Input thread successfully shut down.");
        return true;
    }
}