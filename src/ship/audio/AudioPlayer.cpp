#include "ship/audio/AudioPlayer.h"
#include "ship/audio/AudioResampler.h"
#include "spdlog/spdlog.h"
#include <cassert>

namespace Ship {

AudioPlayer::~AudioPlayer() {
    SPDLOG_TRACE("destruct audio player");
}

// Resampler channel count differs between modes:
//   - Float pipeline: resampler processes stereo (input is always stereo;
//     mix + surround decode both run after the resample step).
//   - S16 legacy:     resampler processes GetNumOutputChannels() (surround
//     decode runs *before* the resample step, preserving the historical
//     order other libultraship consumers rely on).
void AudioPlayer::RebuildResampler() {
    const int32_t channels = mAudioSettings.UseFloatPipeline ? 2 : GetNumOutputChannels();
    if (mAudioSettings.SourceSampleRate != mAudioSettings.SampleRate && mAudioSettings.SourceSampleRate > 0) {
        SPDLOG_INFO("AudioPlayer: initializing resampler {} Hz → {} Hz, {} ch ({})",
                    mAudioSettings.SourceSampleRate, mAudioSettings.SampleRate, channels,
                    mAudioSettings.UseFloatPipeline ? "float HD" : "s16 legacy");
        mResampler = std::make_unique<AudioResampler>(mAudioSettings.SourceSampleRate, mAudioSettings.SampleRate,
                                                      channels);
    } else {
        SPDLOG_INFO("AudioPlayer: resampler disabled {} Hz → {} Hz, {} ch", mAudioSettings.SourceSampleRate,
                    mAudioSettings.SampleRate, channels);
        mResampler = nullptr;
    }
}

bool AudioPlayer::Init() {
    if (mAudioSettings.ChannelSetting == AudioChannelsSetting::audioMatrix51) {
        SPDLOG_INFO("Initializing sound matrix decoder for surround");
        mSoundMatrixDecoder = std::make_unique<SoundMatrixDecoder>(mAudioSettings.SampleRate);
    }

    RebuildResampler();
    mInitialized = DoInit();
    return IsInitialized();
}

bool AudioPlayer::IsInitialized() {
    return mInitialized;
}

int32_t AudioPlayer::GetSampleRate() const {
    return mAudioSettings.SampleRate;
}

int32_t AudioPlayer::GetSourceSampleRate() const {
    return mAudioSettings.SourceSampleRate;
}

int32_t AudioPlayer::GetSampleLength() const {
    return mAudioSettings.SampleLength;
}

int32_t AudioPlayer::GetDesiredBuffered() const {
    // Scale DesiredBuffered from source rate to output rate so callers
    // (e.g. DoPlay fill threshold) work in output-rate frames consistently.
    if (mAudioSettings.SourceSampleRate > 0 && mAudioSettings.SourceSampleRate != mAudioSettings.SampleRate) {
        return (int32_t)((int64_t)mAudioSettings.DesiredBuffered * mAudioSettings.SampleRate /
                         mAudioSettings.SourceSampleRate);
    }
    return mAudioSettings.DesiredBuffered;
}

AudioChannelsSetting AudioPlayer::GetAudioChannels() const {
    return mAudioSettings.ChannelSetting;
}

void AudioPlayer::SetSampleRate(int32_t rate) {
    mAudioSettings.SampleRate = rate;
}

void AudioPlayer::SetSourceSampleRate(int32_t rate) {
    mAudioSettings.SourceSampleRate = rate;
}

void AudioPlayer::SetSampleLength(int32_t length) {
    mAudioSettings.SampleLength = length;
}

void AudioPlayer::SetDesiredBuffered(int32_t size) {
    mAudioSettings.DesiredBuffered = size;
}

bool AudioPlayer::SetAudioChannels(AudioChannelsSetting channels) {
    if (mAudioSettings.ChannelSetting == channels) {
        return true; // No change needed
    }

    SPDLOG_INFO("Changing audio channels from {} to {}", AudioChannelsSettingName(mAudioSettings.ChannelSetting),
                AudioChannelsSettingName(channels));

    // Close current audio device
    DoClose();

    // Update channel setting
    mAudioSettings.ChannelSetting = channels;

    // Setup or teardown sound matrix decoder
    if (channels == AudioChannelsSetting::audioMatrix51) {
        if (!mSoundMatrixDecoder) {
            mSoundMatrixDecoder = std::make_unique<SoundMatrixDecoder>(mAudioSettings.SampleRate);
        }
    } else {
        // When switching away from matrix mode, release the decoder
        mSoundMatrixDecoder.reset();
    }

    // Channel-count change can affect the s16 legacy resampler (built at
    // GetNumOutputChannels()); rebuild to pick that up.
    RebuildResampler();
    return DoInit();
}

bool AudioPlayer::SetMixSource(MixSource source) {
    if (!mAudioSettings.UseFloatPipeline && source) {
        SPDLOG_WARN("AudioPlayer::SetMixSource ignored — float pipeline is disabled");
        return false;
    }
    mMixSource = std::move(source);
    return true;
}

bool AudioPlayer::SetUseFloatPipeline(bool enabled) {
    if (mAudioSettings.UseFloatPipeline == enabled) {
        return true;
    }
    SPDLOG_INFO("AudioPlayer: switching pipeline mode {} → {}",
                mAudioSettings.UseFloatPipeline ? "float HD" : "s16 legacy",
                enabled ? "float HD" : "s16 legacy");
    DoClose();
    const bool oldMode = mAudioSettings.UseFloatPipeline;
    mAudioSettings.UseFloatPipeline = enabled;
    if (!enabled) {
        // Dropping the float path also drops any installed mix source — the
        // s16 mix happens upstream in OTRGlobals.
        mMixSource = nullptr;
    }
    RebuildResampler();
    mInitialized = DoInit();
    if (!mInitialized) {
        SPDLOG_ERROR("AudioPlayer: reinit failed at new mode, reverting");
        mAudioSettings.UseFloatPipeline = oldMode;
        RebuildResampler();
        mInitialized = DoInit();
    }
    return mInitialized && mAudioSettings.UseFloatPipeline == enabled;
}

int32_t AudioPlayer::GetNumOutputChannels() const {
    switch (mAudioSettings.ChannelSetting) {
        case AudioChannelsSetting::audioMatrix51:
        case AudioChannelsSetting::audioRaw51:
            return 6;
        case AudioChannelsSetting::audioStereo:
        default:
            return 2;
    }
}

void AudioPlayer::Play(const uint8_t* buf, size_t len) {
    if (mAudioSettings.UseFloatPipeline) {
        SPDLOG_WARN("AudioPlayer::Play(uint8_t*) called in float mode — dropping buffer");
        return;
    }

    // Legacy stages (unchanged from the pre-float-pipeline behaviour so
    // existing libultraship consumers keep their byte-exact contract):
    //   1. Surround decode if matrix-5.1 (stereo s16 → 6-channel s16).
    //   2. Resample at the current channel count.
    //   3. DoPlay.

    const uint8_t* pcm = buf;
    size_t pcmLen = len;

    std::vector<uint8_t> surroundBuf;

    if (mAudioSettings.ChannelSetting == AudioChannelsSetting::audioMatrix51) {
        if (!mSoundMatrixDecoder) {
            SPDLOG_ERROR("AudioPlayer: Matrix 5.1 mode enabled but SoundMatrixDecoder is not initialized");
            return;
        }
        const auto [surroundOut, surroundLen] = mSoundMatrixDecoder->Process(buf, len);
        // Copy to local buffer so we own the memory through the resampler step
        surroundBuf.assign(surroundOut, surroundOut + surroundLen);
        pcm = surroundBuf.data();
        pcmLen = surroundLen;
    }

    // Step 2: resample if source rate ≠ output rate
    if (mResampler) {
        const int ch = GetNumOutputChannels();
        const int32_t inFrames = static_cast<int32_t>(pcmLen / (sizeof(int16_t) * ch));
        const int32_t maxOut = mResampler->MaxOutputFrames(inFrames);

        assert(static_cast<size_t>(maxOut * ch) <= kResampleBufSamples &&
               "Resample output exceeds kResampleBufSamples — increase the buffer size");

        const int32_t outFrames = mResampler->Process(reinterpret_cast<const int16_t*>(pcm), inFrames,
                                                      mResampleBufS16.data(), maxOut);
        DoPlay(reinterpret_cast<const uint8_t*>(mResampleBufS16.data()),
               static_cast<size_t>(outFrames * ch * sizeof(int16_t)));
        return;
    }

    // Step 3: passthrough (no resampling needed)
    DoPlay(pcm, pcmLen);
}

void AudioPlayer::Play(const float* buf, size_t frames) {
    if (!mAudioSettings.UseFloatPipeline) {
        SPDLOG_WARN("AudioPlayer::Play(float*) called in s16 mode — dropping buffer");
        return;
    }

    // Stages of the float audio pipeline:
    //   1. Resample the primary input (always stereo from the game engine)
    //      to the device's output rate.
    //   2. Mix in the optional secondary stereo source (FluidSynth) at the
    //      output rate, with a tanh-style soft-clip. The source therefore
    //      bypasses the resampler entirely and runs at native device quality.
    //   3. Surround-decode stereo → 5.1 if in matrix-5.1 mode.
    //   4. DoPlay the resulting interleaved float buffer.

    // ── Stage 1: resample stereo to output rate ───────────────────────────
    const float* stereoOutRate = buf;
    int32_t outFrames = static_cast<int32_t>(frames);
    if (mResampler) {
        const int32_t inFrames = static_cast<int32_t>(frames);
        const int32_t maxOut = mResampler->MaxOutputFrames(inFrames);

        assert(static_cast<size_t>(maxOut * 2) <= kResampleBufSamples &&
               "Resample output exceeds kResampleBufSamples — increase the buffer size");

        outFrames = mResampler->Process(buf, inFrames, mResampleBuf.data(), maxOut);
        stereoOutRate = mResampleBuf.data();
    }

    // ── Stage 2: mix the secondary source (post-resampler, output rate) ──
    // We always write the mixed result back into mResampleBuf so the same
    // pointer feeds the surround decode below.
    if (mMixSource) {
        assert(static_cast<size_t>(outFrames * 2) <= kResampleBufSamples &&
               "Mix output exceeds kResampleBufSamples — increase the buffer size");
        mMixSource(mMixSourceBuf.data(), outFrames);

        // Tanh approximation used to soft-clip the secondary-source mix. Matches the
        // curve used in SoH's OTRAudio_Thread before the float pipeline landed, so
        // dynamics behave identically when the synth contributes a peaky signal.
        auto SoftClipTanhApprox = [](float x) -> float {
            const float x2 = x * x;
            return x * (27.0f + x2) / (27.0f + 9.0f * x2);
        };

        for (int32_t i = 0; i < outFrames * 2; i++) {
            mResampleBuf[i] = SoftClipTanhApprox(stereoOutRate[i] + mMixSourceBuf[i]);
        }
        stereoOutRate = mResampleBuf.data();
    } else if (stereoOutRate != mResampleBuf.data()) {
        // No mix and no resample: stereoOutRate still points at the caller's
        // buffer. The surround decode below copies through its own buffer,
        // and the stereo passthrough at the end works off whatever
        // stereoOutRate points to, so no copy is needed here.
    }

    // ── Stage 3: surround decode (stereo → 5.1) ──────────────────────────
    if (mAudioSettings.ChannelSetting == AudioChannelsSetting::audioMatrix51) {
        if (!mSoundMatrixDecoder) {
            SPDLOG_ERROR("AudioPlayer: Matrix 5.1 mode enabled but SoundMatrixDecoder is not initialized");
            return;
        }
        const auto [surroundOut, surroundFrames] =
            mSoundMatrixDecoder->Process(stereoOutRate, static_cast<size_t>(outFrames));
        DoPlay(reinterpret_cast<const uint8_t*>(surroundOut),
               static_cast<size_t>(surroundFrames) * 6 * sizeof(float));
        return;
    }

    // ── Stage 4: stereo passthrough ──────────────────────────────────────
    DoPlay(reinterpret_cast<const uint8_t*>(stereoOutRate),
           static_cast<size_t>(outFrames) * 2 * sizeof(float));
}
} // namespace Ship
