#include "libultraship/libultraship.h"
#include "AudioDmaRegistry.h"

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
    // On N64, DMA reads from ROM which has no bounds — the last chunk of
    // an audio sample can extend past the data. On PC, clamp to the blob size
    // and zero-fill the remainder so ADPCM decoding sees silence.
    size_t safeBytes = AudioDma_Clamp(devAddr, nbytes);
    if (safeBytes < nbytes) {
        memset(vAddr, 0, nbytes);
    }
    memcpy(vAddr, (const void*)devAddr, safeBytes);
    return 0;
}
}