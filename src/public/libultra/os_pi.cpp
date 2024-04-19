#include "libultraship/libultraship.h"

extern "C" {

void osCreatePiManager(OSPri pri, OSMesgQueue* cmdQ, OSMesg* cmdBuf, int32_t cmdMsgCnt) {
}

int32_t osPiReadIo(uint32_t a, uint32_t* b) {
    return 0;
}

int32_t osPiWriteIo(uint32_t devAddr, uint32_t data) {
    return 0;
}

int32_t osPiStartDma(OSIoMesg* mb, int32_t priority, int32_t direction, uintptr_t devAddr, void* vAddr, size_t nbytes,
                     OSMesgQueue* mq) {
    memcpy(vAddr, (const void*)devAddr, nbytes);
    return 0;
}
}