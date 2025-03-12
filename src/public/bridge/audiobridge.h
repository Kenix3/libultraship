#pragma once

#ifndef AUDIOBRIDGE_H
#define AUDIOBRIDGE_H

#include "stdint.h"
#include "stddef.h"

typedef enum AudioSurroundSetting {
    stereo,
    surround51
} AudioSurroundSetting;

#ifdef __cplusplus
extern "C" {
#endif

int32_t AudioPlayerBuffered();
int32_t AudioPlayerGetDesiredBuffered();
AudioSurroundSetting GetAudioSurround();
void AudioPlayerPlayFrame(const uint8_t* buf, size_t len);

#ifdef __cplusplus
};
#endif

#endif
