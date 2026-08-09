#pragma once
#include "AudioPlayer.h"
#include <SDL3/SDL.h>

namespace Ship {
/**
 * @brief AudioPlayer implementation backed by SDL3's audio subsystem.
 *
 * SDLAudioPlayer opens the platform output with `SDL_OpenAudioDeviceStream` and queues PCM in DoPlay().
 * It supports stereo and 6-channel output according to the configured AudioChannelsSetting.
 * This backend is available on all platforms that Ship supports.
 */
class SDLAudioPlayer final : public AudioPlayer {
  public:
    /**
     * @brief Constructs an SDLAudioPlayer with the given audio settings.
     * @param settings Sample rate, buffer size, desired buffered frames, and channel mode.
     */
    SDLAudioPlayer(AudioSettings settings) : AudioPlayer(settings) {
    }
    ~SDLAudioPlayer();

    /**
     * @brief Returns the number of audio frames currently queued in the SDL audio device.
     *
     * Used by the audio subsystem to decide how many frames to produce per game tick.
     */
    int Buffered() override;

  protected:
    /**
     * @brief Opens the SDL audio device with the configured settings.
     * @return true if the device was opened successfully.
     */
    bool DoInit() override;

    /**
     * @brief Closes the SDL audio device and releases its resources.
     */
    void DoClose() override;

    /**
     * @brief Queues interleaved PCM samples to the SDL audio device.
     * @param buf Interleaved sample data (stereo: L,R,… or surround: FL,FR,C,LFE,SL,SR,…).
     * @param len Length of @p buf in bytes.
     */
    void DoPlay(const uint8_t* buf, size_t len) override;

  private:
    SDL_AudioStream* mStream = nullptr; ///< Stream bound to the opened SDL audio device.
    int32_t mNumChannels = 2;           ///< Number of output channels (2 for stereo, 6 for 5.1).
};
} // namespace Ship
