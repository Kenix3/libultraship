#include "libultraship/libultraship.h"

extern "C" {

__OSEventState __osEventStateTab[OS_NUM_EVENTS] = { 0 };

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

int32_t osJamMesg(OSMesgQueue* mq, OSMesg msg, int32_t flag) {
    if (mq->validCount == 0) {
        return -1;
    }

    mq->first = (mq->first + mq->msgCount - 1) % mq->msgCount;
    mq->msg[mq->first] = msg;
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

void osSetEventMesg(OSEvent event, OSMesgQueue* mq, OSMesg msg) {

    __OSEventState* es = &__osEventStateTab[event];

    es->queue = mq;
    es->msg = msg;
}
}