#pragma once
#include "stdint.h"
#include "stddef.h"
#include <string>
#include <memory>
#include <functional>
#include "ship/audio/AudioChannelsSetting.h"
#include "ship/audio/AudioResampler.h"
#include "ship/audio/SoundMatrixDecoder.h"

namespace Ship {

/**
 * @brief Configuration parameters shared by all AudioPlayer backends.
 */
struct AudioSettings {
    int32_t SampleRate = 48000;     ///< Output sample rate in Hz.
    int32_t SourceSampleRate = 0;   ///< Source sample rate in Hz. (0 = same as SampleRate, no resampling)
    int32_t SampleLength = 1024;    ///< Number of samples per audio frame.
    int32_t DesiredBuffered = 2480; ///< Target number of frames to keep buffered.
    AudioChannelsSetting ChannelSetting =
        AudioChannelsSetting::audioStereo; ///< Channel mode (stereo / 5.1 matrix / 5.1 raw).

    /// When true, the AudioPlayer pipeline (Play / resampler / matrix decoder
    /// / backend device format) runs in 32-bit float. Enables the optional
    /// MixSource hook for sources rendered at the device output rate. When
    /// false (default), the pipeline is interleaved int16 — same byte layout
    /// and entry points the AudioPlayer always had, so existing libultraship
    /// consumers keep working with no code change.
    bool UseFloatPipeline = false;
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
    virtual ~AudioPlayer();

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
     * @brief Submits a frame of PCM audio to the output device — legacy s16 path.
     *
     * Default entry point preserved for libultraship consumers. Samples are
     * interleaved signed 16-bit; the legacy resampler / matrix-decoder
     * boundaries do the (lossy) s16↔float conversions internally. Calls
     * here when @c UseFloatPipeline is true are a configuration mistake
     * and emit a warning + drop the buffer.
     *
     * @param buf Interleaved s16 samples (Stereo: (L,R,…); 5.1: (FL,FR,C,LFE,SL,SR,…)).
     * @param len Length of @p buf in bytes.
     */
    void Play(const uint8_t* buf, size_t len);

    /**
     * @brief Submits a frame of PCM audio to the output device — float pipeline.
     *
     * Only valid when @c UseFloatPipeline is true. The audio path stays in
     * 32-bit float through resample, optional MixSource summing + soft-clip,
     * surround decode, and into the backend. The MixSource (if set) runs at
     * the device output rate, so any secondary source can render at native
     * device quality without traversing the resampler.
     *
     * @param buf Interleaved stereo float samples in nominal [-1, 1].
     * @param frames Number of stereo frames in @p buf.
     */
    void Play(const float* buf, size_t frames);

    /** @brief Returns true if Init() has been called and succeeded. */
    bool IsInitialized();

    /** @brief Returns the configured output sample rate in Hz. */
    int32_t GetSampleRate() const;

    /** @brief Returns the configured source sample rate in Hz. */
    int32_t GetSourceSampleRate() const;

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
     * @brief Sets the source sample rate.
     * @param rate New sample rate in Hz.
     */
    void SetSourceSampleRate(int32_t rate);

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

    /**
     * @brief Callback signature for a secondary stereo audio source mixed in
     *        after the resampler.
     *
     * The callback fills @p stereoOut with @p frames frames of interleaved
     * stereo float at the device's output rate (GetSampleRate()), which
     * lets the source bypass the resampler entirely — the resampler runs
     * only over the primary input stream. The mix sums the two sources
     * with a tanh-style soft-clip before surround decoding (if any).
     */
    using MixSource = std::function<void(float* stereoOut, int frames)>;

    /**
     * @brief Installs a secondary audio source whose contribution is mixed
     *        in at the output rate, post-resampler.
     *
     * Only meaningful when @c UseFloatPipeline is true (the s16 legacy path
     * has no mix step). Pass @c nullptr to remove the source. Thread-safe
     * with respect to the audio thread only in the sense that
     * std::function assignment is sequentially consistent on x86; callers
     * in practice swap this once at synth install/teardown.
     *
     * @return true if accepted; false (and ignored) when the player is in
     *         the s16 legacy mode.
     */
    bool SetMixSource(MixSource source);

    /**
     * @brief Switches the pipeline between legacy s16 and float HD modes
     *        at runtime.
     *
     * Closes the audio device, updates @c UseFloatPipeline, and reopens
     * the device with the matching format. The Play overload that matches
     * the new mode is the only one valid until the next switch.
     *
     * @return true if the device successfully reinitialised in the new
     *         mode. On failure the previous mode is restored.
     */
    bool SetUseFloatPipeline(bool enabled);

    /** @brief Returns whether the float pipeline mode is active. */
    bool IsUsingFloatPipeline() const { return mAudioSettings.UseFloatPipeline; }

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
    /// Picks the right channel count (stereo for float mode, output channel
    /// count for legacy s16 mode) and (re)constructs mResampler. No-op when
    /// the rates already match.
    void RebuildResampler();

    /// Whether a stereo->5.1 matrix decoder is required for the current
    /// (channel, pipeline) combination. Matrix 5.1 always needs it; Raw 5.1
    /// needs it only in float mode (there the source is stereo, so the engine's
    /// native 6-channel output isn't available to pass through). Raw 5.1 on the
    /// s16 path keeps passing the engine's native 6 channels straight through.
    bool NeedsMatrixDecoder() const;

    /// (Re)creates or releases mSoundMatrixDecoder to match NeedsMatrixDecoder().
    /// Call after any channel-setting or pipeline-mode change.
    void EnsureMatrixDecoder();

    std::unique_ptr<SoundMatrixDecoder>
        mSoundMatrixDecoder; ///< Stereo-to-surround decoder (Matrix 5.1, or Raw 5.1 in float mode).
    std::unique_ptr<AudioResampler> mResampler;

    // Fixed-size scratch buffers — no heap allocation on the audio hot path.
    // Sized for the worst-case ratio and channel count of the data the buffer
    // holds at its stage. mResampleBuf holds *stereo* output-rate frames
    // (resample step), so 2 channels suffice; mMixSourceBuf likewise holds
    // stereo frames the secondary source writes into. mSurroundBuf is sized
    // for 6 channels of output-rate frames (matrix-5.1 final output) so the
    // decoder has somewhere to write before DoPlay. 16384 gives comfortable
    // headroom for 32k→48k @ SampleLength=1024 (ceil(1024 * 3/2) * 6 = 9216)
    // and for higher device rates (e.g. 32k→96k).
    static constexpr size_t kResampleBufSamples = 16384;
    std::array<float, kResampleBufSamples> mResampleBuf{};
    std::array<float, kResampleBufSamples> mMixSourceBuf{};
    // Legacy s16 path uses its own scratch so both code paths can coexist
    // without retypeing the float buffer. 16384 × 2 B = 32 KB.
    std::array<int16_t, kResampleBufSamples> mResampleBufS16{};

    MixSource mMixSource;

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
