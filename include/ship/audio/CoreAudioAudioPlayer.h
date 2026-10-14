#pragma once

#ifdef __APPLE__

#include "AudioPlayer.h"
#include <AudioToolbox/AudioToolbox.h>
#include <atomic>

namespace Ship {
/**
 * @brief AudioPlayer implementation backed by Apple's Core Audio framework.
 *
 * CoreAudioAudioPlayer uses an AudioUnit (kAudioUnitSubType_DefaultOutput) with a
 * render callback to pull interleaved PCM samples from an internal ring buffer.
 * DoPlay() pushes data into the ring buffer, and the Core Audio render callback
 * reads from it on the audio thread.
 *
 * This backend is only available on Apple platforms (macOS / iOS).
 */
class CoreAudioAudioPlayer : public AudioPlayer {
  public:
    /**
     * @brief Constructs a CoreAudioAudioPlayer with the given audio settings.
     * @param settings Sample rate, buffer size, desired buffered frames, and channel mode.
     */
    CoreAudioAudioPlayer(AudioSettings settings);
    ~CoreAudioAudioPlayer();

    /**
     * @brief Returns the number of audio frames currently queued in the ring buffer.
     */
    int Buffered() override;

  protected:
    /**
     * @brief Opens and configures the Core Audio output unit.
     * @return true if the audio unit was created and started successfully.
     */
    bool DoInit() override;

    /**
     * @brief Stops and disposes of the Core Audio output unit and frees the ring buffer.
     */
    void DoClose() override;

    /**
     * @brief Copies interleaved PCM samples into the ring buffer for playback.
     * @param buf Interleaved sample data.
     * @param len Length of @p buf in bytes.
     */
    void DoPlay(const uint8_t* buf, size_t len) override;

  private:
    /**
     * @brief Core Audio render callback that pulls samples from the ring buffer.
     *
     * Called on the audio thread by the output AudioUnit whenever it needs more
     * sample data. Reads lock-free from the ring buffer using atomic read/write
     * indices (SPSC).
     *
     * @param inRefCon Pointer to the owning CoreAudioAudioPlayer instance.
     * @param ioActionFlags Render action flags (unused).
     * @param inTimeStamp Timestamp for the requested render (unused).
     * @param inBusNumber Output bus number (unused).
     * @param inNumberFrames Number of frames requested.
     * @param ioData Buffer list to fill with audio data.
     * @return noErr on success.
     */
    static OSStatus CoreAudioRenderCallback(void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags,
                                            const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber,
                                            UInt32 inNumberFrames, AudioBufferList* ioData);

    AudioUnit mAudioUnit;
    int32_t mNumChannels;
    uint8_t* mRingBuffer;   ///< SPSC circular buffer for audio samples (no locking).
    size_t mRingBufferSize; ///< Total size of the ring buffer in bytes.
    /// Read position. Written only by the consumer (render callback);
    /// read by both with acquire/release ordering.
    std::atomic<size_t> mRingBufferReadPos;
    /// Write position. Written only by the producer (DoPlay);
    /// read by both with acquire/release ordering.
    std::atomic<size_t> mRingBufferWritePos;
    bool mInitialized;
};
} // namespace Ship
#endif
