#include "libultraship/libultraship.h"

extern "C" {

Uint32 __lusViCallback(Uint32 interval, void* param) {
    __OSEventState* es = &__osEventStateTab[OS_EVENT_VI];

    if (es && es->queue) {
        osSendMesg(es->queue, es->msg, OS_MESG_NOBLOCK);
    }

    return interval;
}

void osCreateViManager(OSPri pri) {
    SDL_AddTimer(16, &__lusViCallback, NULL);
}

void osViSetEvent(OSMesgQueue* queue, OSMesg mesg, uint32_t c) {

    __OSEventState* es = &__osEventStateTab[OS_EVENT_VI];

    es->queue = queue;
    es->msg = mesg;
}

void osViSwapBuffer(void* a) {
}

void osViSetSpecialFeatures(uint32_t a) {
}

void osViSetMode(OSViMode* a) {
}

void osViBlack(uint8_t a) {
}

void* osViGetNextFramebuffer() {
    return nullptr;
}

void* osViGetCurrentFramebuffer() {
    return nullptr;
}

void osViSetXScale(float a) {
}

void osViSetYScale(float a) {
}
}