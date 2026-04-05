#include "AudioDmaRegistry.h"
#include <spdlog/spdlog.h>

// Small fixed-size registry of audio blob address ranges.
// Audio DMA (osPiStartDma) reads fixed-size chunks that can extend past blob
// data. On N64 the source was ROM with no bounds; on PC we clamp to the blob.

static constexpr int kMaxAudioBlobs = 8;

struct AudioBlobRange {
    uintptr_t base;
    uintptr_t end;
};

static AudioBlobRange sAudioBlobs[kMaxAudioBlobs];
static int sAudioBlobCount = 0;

extern "C" void AudioDma_Register(const void* base, size_t size) {
    if (sAudioBlobCount >= kMaxAudioBlobs) {
        return;
    }
    auto b = reinterpret_cast<uintptr_t>(base);
    sAudioBlobs[sAudioBlobCount++] = { b, b + size };
}

extern "C" size_t AudioDma_Clamp(uintptr_t addr, size_t nbytes) {
    for (int i = 0; i < sAudioBlobCount; i++) {
        if (addr >= sAudioBlobs[i].base && addr < sAudioBlobs[i].end) {
            size_t avail = sAudioBlobs[i].end - addr;
            return (nbytes <= avail) ? nbytes : avail;
        }
    }
    // Not a registered audio blob — pass through unclamped
    return nbytes;
}

extern "C" void AudioDma_Clear(void) {
    sAudioBlobCount = 0;
}
