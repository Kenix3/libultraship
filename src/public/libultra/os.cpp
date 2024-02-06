#include "libultraship/libultraship.h"
#include <SDL2/SDL.h>
#include <ratio>

// Establish a chrono duration for the N64 46.875MHz clock rate
typedef std::ratio<3000, 64> n64ClockRatio;
typedef std::ratio_divide<std::micro, n64ClockRatio> n64CycleRate;
typedef std::chrono::duration<long long, n64CycleRate> n64CycleRateDuration;

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

int32_t osContStartReadData(OSMesgQueue* mesg) {
    return 0;
}

void osContGetReadData(OSContPad* pad) {
    memset(pad, 0, sizeof(OSContPad) * __osMaxControllers);

    LUS::Context::GetInstance()->GetControlDeck()->WriteToPad(pad);
}

// Returns the OS time matching the N64 46.875MHz cycle rate
// LUSTODO: This should be adjusted to return the time since "boot"
uint64_t osGetTime(void) {
    return std::chrono::duration_cast<n64CycleRateDuration>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// Returns the CPU clock count matching the N64 46.875Mhz cycle rate
uint32_t osGetCount(void) {
    return std::chrono::duration_cast<n64CycleRateDuration>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

void osCreateMesgQueue(OSMesgQueue* mq, OSMesg* msgBuf, int32_t count) {
    mq->validCount = 0;
    mq->first = 0;
    mq->msgCount = count;
    mq->msg = msgBuf;
    return;
}

int32_t osSendMesg(OSMesgQueue* mq, OSMesg msg, int32_t flag) {
    int32_t index;
    if (mq->validCount >= mq->msgCount) {
        return -1;
    }
    index = (mq->first + mq->validCount) % mq->msgCount;
    mq->msg[index] = msg;
    mq->validCount++;
    return 0;
}

int32_t osRecvMesg(OSMesgQueue* mq, OSMesg* msg, int32_t flag) {
    if (mq->validCount == 0) {
        return -1;
    }
    if (msg != NULL) {
        *msg = *(mq->first + mq->msg);
    }
    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;
    return 0;
}
}
