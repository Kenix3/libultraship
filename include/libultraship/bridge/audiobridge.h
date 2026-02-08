#pragma once

#include "stdint.h"
#include "stddef.h"
#include "ship/audio/AudioChannelsSetting.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t AudioPlayerBuffered();
int32_t AudioPlayerGetDesiredBuffered();
AudioChannelsSetting GetAudioChannels();
int32_t GetNumAudioChannels();
void AudioPlayerPlayFrame(const uint8_t* buf, size_t len);

// Set audio channels configuration at runtime (stereo or 5.1 surround)
// This will reinitialize the audio backend without requiring a game restart
void SetAudioChannels(AudioChannelsSetting channels);

#ifdef __cplusplus
};
#endif
