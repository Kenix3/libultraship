#pragma once
#include "AudioPlayer.h"
#include <SDL3/SDL.h>

#include <atomic>
#include <mutex>
#include <vector>

namespace Ship {
/**
 * @brief AudioPlayer implementation backed by SDL3's audio subsystem.
 *
 * SDLAudioPlayer opens the platform output with `SDL_OpenAudioDeviceStream` and lets SDL pull from it:
 * SDL owns the audio thread and calls AudioStreamCallback() whenever the device needs more samples.
 * DoPlay() only writes into a ring buffer, which decouples the game thread's timing from the device's.
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
     * @brief Returns the number of audio frames currently waiting in the ring buffer.
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
     * @brief Writes interleaved PCM samples into the ring buffer for the audio thread to consume.
     * @param buf Interleaved sample data (stereo: L,R,… or surround: FL,FR,C,LFE,SL,SR,…).
     * @param len Length of @p buf in bytes.
     */
    void DoPlay(const uint8_t* buf, size_t len) override;

  private:
    /**
     * @brief SDL audio-thread callback: fills the device's request from the ring buffer.
     * @param userdata The SDLAudioPlayer that opened @p stream.
     * @param additionalAmount Bytes SDL needs right now to keep the device fed.
     */
    static void SDLCALL AudioStreamCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount,
                                            int totalAmount);

    void RingInit(uint32_t bytes);
    int32_t RingAvailable() const;
    int32_t RingSpace() const;
    int32_t RingRead(uint8_t* dst, int32_t bytes);
    int32_t RingWrite(const uint8_t* src, int32_t bytes);

    SDL_AudioStream* mStream = nullptr; ///< Stream bound to the opened SDL audio device.
    int32_t mNumChannels = 2;           ///< Number of output channels (2 for stereo, 6 for 5.1).

    std::vector<uint8_t> mRing; ///< Power-of-two sized storage bridging the game and audio threads.
    uint32_t mRingMask = 0;     ///< mRing.size() - 1, for wrapping the pointers below.
    int64_t mRingReadPos = 0;   ///< Unbounded read pointer; the difference with the write one is the fill.
    int64_t mRingWritePos = 0;  ///< Unbounded write pointer.

    std::vector<uint8_t> mScratch;       ///< Pre-sized so the audio thread never allocates in steady state.
    std::mutex mMutex;                   ///< Serialises ring access between DoPlay() and the audio thread.
    std::atomic<bool> mRunning{ false }; ///< False once the device is closing; stops the callback working.
};
} // namespace Ship
