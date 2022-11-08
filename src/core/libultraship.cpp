#include "core/libultraship.h"
#include "core/Window.h"
#include "UltraController.h"
#include <SDL2/SDL.h>
#include <spdlog/spdlog.h>

#include "menu/ImGuiImpl.h"
#include "misc/Hooks.h"

extern "C" {
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

uint64_t osGetTime(void) {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

uint32_t osGetCount(void) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
}