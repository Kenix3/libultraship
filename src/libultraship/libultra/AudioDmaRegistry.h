#ifndef PORT_AUDIO_DMA_REGISTRY_H
#define PORT_AUDIO_DMA_REGISTRY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Registry of audio blob address ranges so osPiStartDma can clamp
// memcpy to the actual source size (N64 ROM had no bounds).
void AudioDma_Register(const void* base, size_t size);
size_t AudioDma_Clamp(uintptr_t addr, size_t nbytes);
void AudioDma_Clear(void);

#ifdef __cplusplus
}
#endif

#endif
