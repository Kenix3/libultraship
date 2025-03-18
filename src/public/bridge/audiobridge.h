#pragma once

#ifndef AUDIOBRIDGE_H
#define AUDIOBRIDGE_H

#include "stdint.h"
#include "stddef.h"

typedef enum AudioChannelsSetting { audioStereo, audioSurround51, audioMax } AudioChannelsSetting;

#ifdef __cplusplus
extern "C" {
#endif

int32_t AudioPlayerBuffered();
int32_t AudioPlayerGetDesiredBuffered();
AudioChannelsSetting GetAudioChannels();
int32_t GetNumAudioChannels();
void AudioPlayerPlayFrame(const uint8_t* buf, size_t len);

#ifdef __cplusplus
};
#endif

#endif
