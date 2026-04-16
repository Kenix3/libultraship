#pragma once

#include "stdint.h"
#include "stddef.h"
#include "ship/Api.h"
#include "ship/audio/AudioChannelsSetting.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Returns the number of audio frames currently queued in the audio device buffer. */
API_EXPORT int32_t AudioPlayerBuffered();

/** @brief Returns the configured target number of buffered audio frames. */
API_EXPORT int32_t AudioPlayerGetDesiredBuffered();

/** @brief Returns the current audio channel mode (stereo / 5.1 matrix / 5.1 raw). */
API_EXPORT AudioChannelsSetting GetAudioChannels();

/** @brief Returns the number of output channels active on the current audio device (2 or 6). */
API_EXPORT int32_t GetNumAudioChannels();

/**
 * @brief Submits a frame of PCM audio to the audio output device.
 *
 * @param buf Interleaved sample data (stereo: L,R,… or surround: FL,FR,C,LFE,SL,SR,…).
 * @param len Length of @p buf in bytes.
 */
API_EXPORT void AudioPlayerPlayFrame(const uint8_t* buf, size_t len);

/**
 * @brief Changes the audio channel configuration at runtime.
 *
 * Reinitialises the audio backend with the new channel mode without requiring a
 * game restart. Stereo and 5.1 surround (matrix or raw) are supported.
 *
 * @param channels New channel mode to apply.
 */
API_EXPORT void SetAudioChannels(AudioChannelsSetting channels);

#ifdef __cplusplus
};
#endif
