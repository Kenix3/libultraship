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
 * @brief Submits a frame of s16 PCM audio to the audio output device (legacy).
 *
 * Default entry point preserved for libultraship consumers on the s16
 * audio path. Forwards to AudioPlayer::Play(uint8_t*, size_t); valid only
 * when the player is in s16 mode (the default).
 *
 * @param buf Interleaved sample data (stereo: L,R,… or surround: FL,FR,C,LFE,SL,SR,…).
 * @param len Length of @p buf in bytes.
 */
API_EXPORT void AudioPlayerPlayFrame(const uint8_t* buf, size_t len);

/**
 * @brief Submits a frame of float PCM audio to the audio output device.
 *
 * Float-pipeline entry point. Valid only when the player has been switched
 * to float mode (see AudioPlayer::SetUseFloatPipeline). The full audio
 * path — resample / optional mix-source sum / surround decode — runs in
 * float at the device's output rate.
 *
 * @param buf Interleaved stereo float samples (L, R, L, R, …) in nominal [-1, 1] range.
 * @param frames Number of stereo frames in @p buf.
 */
API_EXPORT void AudioPlayerPlayFrameF32(const float* buf, size_t frames);

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
