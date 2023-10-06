#include "libultraship/libultraship.h"

std::map<OSMesgQueue*, std::pair<std::shared_ptr<std::mutex>, std::shared_ptr<std::condition_variable>>> __lusMesgLockMap;

extern "C" {

__OSEventState __osEventStateTab[OS_NUM_EVENTS] = { 0 };

#define MQ_GET_COUNT(mq) ((mq)->validCount)
#define MQ_IS_EMPTY(mq) (MQ_GET_COUNT(mq) == 0)
#define MQ_IS_FULL(mq) (MQ_GET_COUNT(mq) >= (mq)->msgCount)

#define MQ_MUTEX(mq) (*__lusMesgLockMap[mq].first)
#define MQ_CVAR(mq) (*__lusMesgLockMap[mq].second)

void osCreateMesgQueue(OSMesgQueue * mq, OSMesg* msgBuf, int32_t count) {

    mq->validCount = 0;
    mq->first = 0;
    mq->msgCount = count;
    mq->msg = msgBuf;

    mq->mtqueue = nullptr;
    mq->fullqueue = nullptr;

    __lusMesgLockMap[mq] = std::make_pair(std::make_shared<std::mutex>(), std::make_shared<std::condition_variable>());
}

int32_t osSendMesg(OSMesgQueue* mq, OSMesg msg, int32_t flag) {

    //mq is not initialised by osCreateMesgQueue
    if (!__lusMesgLockMap.contains(mq))
        return -1;

    std::unique_lock<std::mutex> ul(MQ_MUTEX(mq));

    while (MQ_IS_FULL(mq)) {
        if (flag == OS_MESG_NOBLOCK)
            return -1;

        // Wait for space in the queue.
        MQ_CVAR(mq).wait(ul);
    }

    if (mq->validCount >= mq->msgCount) {
        return -1;
    }

    int32_t last = (mq->first + mq->validCount) % mq->msgCount;
    mq->msg[last] = msg;
    mq->validCount++;

    // Wake threads waiting on this queue.
    MQ_CVAR(mq).notify_all();

    return 0;
}

int32_t osJamMesg(OSMesgQueue* mq, OSMesg msg, int32_t flag) {

    // mq is not initialised by osCreateMesgQueue
    if (!__lusMesgLockMap.contains(mq))
        return -1;

    std::unique_lock ul(MQ_MUTEX(mq));

    while (MQ_IS_FULL(mq)) {
        if (flag == OS_MESG_NOBLOCK) {
            return -1;
        }

        // Wait for space in the queue.
        MQ_CVAR(mq).wait(ul);
    }

    mq->first = (mq->first + mq->msgCount - 1) % mq->msgCount;
    mq->msg[mq->first] = msg;
    mq->validCount++;

    // Wake threads waiting on this queue.
    MQ_CVAR(mq).notify_all();

    return 0;
}

int32_t osRecvMesg(OSMesgQueue* mq, OSMesg* msg, int32_t flag) {

    // mq is not initialised by osCreateMesgQueue
    if (!__lusMesgLockMap.contains(mq))
        return -1;

    std::unique_lock ul(MQ_MUTEX(mq));

    while (MQ_IS_EMPTY(mq)) {
        if (flag == OS_MESG_NOBLOCK)
            return -1;

        // Wait for mesg
        MQ_CVAR(mq).wait(ul);
    }

    if (msg != nullptr) {
        *msg = mq->msg[mq->first];
    }

    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;

    // Wake threads waiting on this queue.
    MQ_CVAR(mq).notify_all();

    return 0;
}

void osSetEventMesg(OSEvent event, OSMesgQueue* mq, OSMesg msg) {

    __OSEventState* es = &__osEventStateTab[event];

    es->queue = mq;
    es->msg = msg;
}
}