#include "OTRWindow.h"
#include "spdlog/spdlog.h"
#include "KeyboardController.h"
#include <map>

extern "C" {
    struct OSMesgQueue;

    uint8_t __osMaxControllers = MAXCONTROLLERS;

    int32_t osContInit(OSMesgQueue* mq, uint8_t* controllerBits, OSContStatus* status) {

        // TODO: Configuration should determine the type of controller and which are plugged in. Can also read from SDL to figure out if any controllers are plugged in.
        OtrLib::OTRController* pad = new OtrLib::KeyboardController(0);
        OtrLib::OTRWindow::Controllers[0] = std::make_shared<OtrLib::OTRController>(*pad);

        for (size_t i = 0; i < __osMaxControllers; i++) {
            if (OtrLib::OTRWindow::Controllers[i] != nullptr) {
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
            if (OtrLib::OTRWindow::Controllers[i] != nullptr) {
                OtrLib::OTRWindow::Controllers[i]->Read(&pad[i]);
            }
        }
    }
}

namespace OtrLib {
    std::shared_ptr<OtrLib::OTRController> OTRWindow::Controllers[MAXCONTROLLERS] = { nullptr };

    OTRWindow::OTRWindow(std::shared_ptr<OTRContext> Context) : Context(Context) {
        WmApi = &gfx_sdl;
        RenderingApi = &gfx_opengl_api;
    }

    void OTRWindow::Init() {
        gfx_init(WmApi, RenderingApi, Context->GetName().c_str(), false);
        WmApi->set_keyboard_callbacks(OTRWindow::KeyDown, OTRWindow::KeyUp, OTRWindow::AllKeysUp);
    }

    void OTRWindow::RunCommands(Gfx* Commands) {
        gfx_run(Commands);
    }

    void OTRWindow::MainLoop(void (*MainFunction)(void)) {
        WmApi->main_loop(MainFunction);
    }

    bool OTRWindow::KeyDown(int32_t dwScancode) {
        bool bIsProcessed = false;
        for (size_t i = 0; i < __osMaxControllers; i++) {
            KeyboardController* pad = dynamic_cast<KeyboardController*>(OtrLib::OTRWindow::Controllers[i].get());
            if (pad != nullptr) {
                if (pad->PressButton(dwScancode)) {
                    bIsProcessed = true;
                }
            }
        }

        return bIsProcessed;
    }

    bool OTRWindow::KeyUp(int32_t dwScancode) {
        bool bIsProcessed = false;
        for (size_t i = 0; i < __osMaxControllers; i++) {
            KeyboardController* pad = dynamic_cast<KeyboardController*>(OtrLib::OTRWindow::Controllers[i].get());
            if (pad != nullptr) {
                if (pad->ReleaseButton(dwScancode)) {
                    bIsProcessed = true;
                }
            }
        }

        return bIsProcessed;
    }

    void OTRWindow::AllKeysUp(void) {
        for (size_t i = 0; i < __osMaxControllers; i++) {
            KeyboardController* pad = dynamic_cast<KeyboardController*>(OtrLib::OTRWindow::Controllers[i].get());
            if (pad != nullptr) {
                pad->ReleaseAllButtons();
            }
        }
    }
}