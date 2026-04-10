#pragma once

#include "stdint.h"
#include "stddef.h"
#include "ship/Api.h"
#include "ship/audio/AudioChannelsSetting.h"

#ifdef __cplusplus
extern "C" {
#endif

API_EXPORT int32_t AudioPlayerBuffered();
API_EXPORT int32_t AudioPlayerGetDesiredBuffered();
API_EXPORT AudioChannelsSetting GetAudioChannels();
API_EXPORT int32_t GetNumAudioChannels();
API_EXPORT void AudioPlayerPlayFrame(const uint8_t* buf, size_t len);

// Set audio channels configuration at runtime (stereo or 5.1 surround)
// This will reinitialize the audio backend without requiring a game restart
API_EXPORT void SetAudioChannels(AudioChannelsSetting channels);

#ifdef __cplusplus
};
#endif
