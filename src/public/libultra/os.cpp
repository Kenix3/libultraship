#include "libultraship/libultraship.h"
#include <SDL2/SDL.h>

extern "C" {
uint8_t __osMaxControllers = MAXCONTROLLERS;

int32_t osContInit(OSMesgQueue *mq, uint8_t *controllerBits, OSContStatus *status) {
    *controllerBits = 0;

#ifndef __WIIU__
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        SPDLOG_ERROR("Failed to initialize SDL game controllers ({})", SDL_GetError());
        exit(EXIT_FAILURE);
    }

#ifndef __SWITCH__
    std::string controllerDb = LUS::Context::LocateFileAcrossAppDirs("gamecontrollerdb.txt");
    int mappingsAdded = SDL_GameControllerAddMappingsFromFile(controllerDb.c_str());
    if (mappingsAdded >= 0) {
        SPDLOG_INFO("Added SDL game controllers from \"{}\" ({})", controllerDb, mappingsAdded);
    } else {
        SPDLOG_ERROR("Failed add SDL game controller mappings from \"{}\" ({})", controllerDb, SDL_GetError());
    }
#endif
#endif

    LUS::Context::GetInstance()->GetControlDeck()->Init(controllerBits);

    return 0;
}

int32_t osContStartReadData(OSMesgQueue *mesg) {
    return 0;
}

void osContGetReadData(OSContPad *pad) {
    memset(pad, 0, sizeof(OSContPad) * __osMaxControllers);

    LUS::Context::GetInstance()->GetControlDeck()->WriteToPad(pad);
}

uint64_t osGetTime(void) {
    return std::chrono::steady_clock::now().time_since_epoch().count();
}

uint32_t osGetCount(void) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
}
}
