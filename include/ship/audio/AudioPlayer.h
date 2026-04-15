#pragma once
#include "stdint.h"
#include "stddef.h"
#include <string>
#include <memory>
#include "ship/audio/AudioChannelsSetting.h"
#include "ship/audio/SoundMatrixDecoder.h"

namespace Ship {

/**
 * @brief Configuration parameters shared by all AudioPlayer backends.
 */
struct AudioSettings {
    int32_t SampleRate = 44100;     ///< Output sample rate in Hz.
    int32_t SampleLength = 1024;    ///< Number of samples per audio frame.
    int32_t DesiredBuffered = 2480; ///< Target number of frames to keep buffered.
    AudioChannelsSetting ChannelSetting =
        AudioChannelsSetting::audioStereo; ///< Channel mode (stereo / 5.1 matrix / 5.1 raw).
};

/**
 * @brief Abstract base class for platform-specific audio output backends.
 *
 * AudioPlayer owns the audio device lifecycle (DoInit / DoClose) and the optional
 * SoundMatrixDecoder that converts stereo input to 5.1 surround. Concrete
 * subclasses implement DoInit(), DoClose(), and DoPlay() for each supported platform
 * (SDL2, CoreAudio, WASAPI, or a null no-op).
 *
 * Obtain the active instance from Context::GetAudio() → Audio::GetAudioPlayer().
 */
class AudioPlayer {

  public:
    /**
     * @brief Constructs an AudioPlayer with the given configuration.
     * @param settings Sample rate, buffer size, desired buffered frames, and channel mode.
     */
    AudioPlayer(AudioSettings settings) : mAudioSettings(settings) {
    }
    ~AudioPlayer();

    /**
     * @brief Calls DoInit() and sets the initialised flag on success.
     * @return true if the audio device was initialised successfully.
     */
    bool Init();

    /**
     * @brief Returns the number of audio frames currently queued in the device buffer.
     *
     * The audio system uses this to pace audio production: if Buffered() exceeds
     * DesiredBuffered the game can skip producing audio for that tick.
     */
    virtual int32_t Buffered() = 0;

    /**
     * @brief Submits a frame of PCM audio to the output device.
     *
     * If 5.1 surround output is configured and the channel setting requires matrix
     * decoding, the stereo @p buf is first decoded to 6-channel surround before
     * being passed to DoPlay().
     *
     * @param buf Interleaved samples:
     *            - Stereo: (L, R, L, R, …)
     *            - 5.1:    (FL, FR, C, LFE, SL, SR, …)
     * @param len Length of @p buf in bytes.
     */
    void Play(const uint8_t* buf, size_t len);

    /** @brief Returns true if Init() has been called and succeeded. */
    bool IsInitialized();

    /** @brief Returns the configured output sample rate in Hz. */
    int32_t GetSampleRate() const;

    /** @brief Returns the configured number of samples per audio frame. */
    int32_t GetSampleLength() const;

    /** @brief Returns the target number of frames to keep buffered. */
    int32_t GetDesiredBuffered() const;

    /** @brief Returns the current channel-output mode. */
    AudioChannelsSetting GetAudioChannels() const;

    /**
     * @brief Sets the output sample rate.
     * @param rate New sample rate in Hz.
     */
    void SetSampleRate(int32_t rate);

    /**
     * @brief Sets the number of samples per audio frame.
     * @param length New frame size in samples.
     */
    void SetSampleLength(int32_t length);

    /**
     * @brief Sets the target number of buffered frames.
     * @param size New buffered-frame target.
     */
    void SetDesiredBuffered(int32_t size);

    /**
     * @brief Changes the channel mode and reinitialises the audio device.
     *
     * Closes the current device, updates the channel setting, and calls DoInit()
     * again. If reinitialisation fails the previous channel mode is restored.
     *
     * @param channels New channel mode.
     * @return true if the device was successfully reinitialised.
     */
    bool SetAudioChannels(AudioChannelsSetting channels);

    /**
     * @brief Returns the number of output channels implied by the current channel setting.
     * @return 2 for stereo, 6 for any 5.1 mode.
     */
    int32_t GetNumOutputChannels() const;

  protected:
    /**
     * @brief Opens and configures the platform audio device.
     *
     * Called by Init() and by SetAudioChannels() when reinitialising.
     *
     * @return true on success.
     */
    virtual bool DoInit() = 0;

    /**
     * @brief Closes the platform audio device and releases its resources.
     *
     * Called by SetAudioChannels() before reinitialisation and by the destructor.
     */
    virtual void DoClose() = 0;

    /**
     * @brief Writes interleaved PCM samples to the platform audio device.
     *
     * Receives audio already in the correct output format (stereo or 6-channel).
     *
     * @param buf Sample data.
     * @param len Length of @p buf in bytes.
     */
    virtual void DoPlay(const uint8_t* buf, size_t len) = 0;

  private:
    std::unique_ptr<SoundMatrixDecoder>
        mSoundMatrixDecoder; ///< Stereo-to-surround decoder (active in matrix-5.1 mode).

    AudioSettings mAudioSettings;
    bool mInitialized = false;
};
} // namespace Ship

#ifdef _WIN32
#include "WasapiAudioPlayer.h"
#endif

#ifdef __APPLE__
#include "CoreAudioAudioPlayer.h"
#endif

#include "SDLAudioPlayer.h"
#include "NullAudioPlayer.h"
