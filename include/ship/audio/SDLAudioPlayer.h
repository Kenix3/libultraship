#pragma once
#include "AudioPlayer.h"
#include <SDL2/SDL.h>

#include <atomic>
#include <mutex>
#include <vector>

namespace Ship {
/**
 * @brief AudioPlayer implementation backed by SDL2's audio subsystem.
 *
 * SDLAudioPlayer uses `SDL_OpenAudioDevice` to open a platform audio output and lets SDL pull
 * from it: SDL owns the audio thread and calls AudioCallback() whenever the device needs more
 * samples. DoPlay() only writes into a ring buffer, which decouples the game thread's timing
 * from the device's and keeps the whole latency budget in one place. If the ring runs dry the
 * callback replays the last burst faded out, which covers a load spike far less audibly than a
 * hole in the stream. It supports both stereo and 6-channel (5.1) output depending on the
 * AudioChannelsSetting configured in AudioPlayer.
 *
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
    /**
     * @brief SDL audio-thread callback: fills the device's request from the ring buffer.
     * @param userdata The SDLAudioPlayer that opened the device.
     * @param stream Destination buffer, which must be filled completely.
     * @param len Length of @p stream in bytes.
     */
    static void SDLCALL AudioCallback(void* userdata, Uint8* stream, int len);

    void RingInit(uint32_t bytes);
    int32_t RingAvailable() const;
    int32_t RingSpace() const;
    int32_t RingRead(uint8_t* dst, int32_t bytes);
    int32_t RingWrite(const uint8_t* src, int32_t bytes);

    SDL_AudioDeviceID mDevice = 0; ///< Handle to the opened SDL audio device.
    int32_t mNumChannels = 2;      ///< Number of output channels (2 for stereo, 6 for 5.1).

    std::vector<uint8_t> mRing; ///< Power-of-two sized storage bridging the game and audio threads.
    uint32_t mRingMask = 0;     ///< mRing.size() - 1, for wrapping the pointers below.
    int64_t mRingReadPos = 0;   ///< Unbounded read pointer; the difference with the write one is the fill.
    int64_t mRingWritePos = 0;  ///< Unbounded write pointer.

    std::mutex mMutex;                   ///< Serialises ring access between DoPlay() and the audio thread.
    std::atomic<bool> mRunning{ false }; ///< False once the device is closing; stops the callback working.

    std::vector<uint8_t> mLastChunk; ///< Last burst played, replayed faded out to cover an underrun.
    bool mLastChunkValid = false;    ///< False until a burst has actually been played.
    bool mUnderrunFaded = false;     ///< True once faded, so a sustained underrun does not loop the chunk.
};
} // namespace Ship
