#include "ship/audio/AudioPlayer.h"
#include "ship/audio/AudioResampler.h"
#include "spdlog/spdlog.h"
#include <cassert>


namespace Ship {

AudioPlayer::~AudioPlayer() {
    SPDLOG_TRACE("destruct audio player");
}

bool AudioPlayer::Init() {
    // Initialize sound matrix decoder if matrix surround mode is enabled
    if (mAudioSettings.ChannelSetting == AudioChannelsSetting::audioMatrix51) {
        SPDLOG_INFO("Initializing sound matrix decoder for surround");
        mSoundMatrixDecoder = std::make_unique<SoundMatrixDecoder>(mAudioSettings.SampleRate);
    }

    // Initialize resampler if source and output rates differ
    if (mAudioSettings.SourceSampleRate != mAudioSettings.SampleRate &&
        mAudioSettings.SourceSampleRate > 0) {
        SPDLOG_INFO("AudioPlayer: initializing resampler {} Hz → {} Hz, {} ch",
                    mAudioSettings.SourceSampleRate, mAudioSettings.SampleRate,
                    GetNumOutputChannels());
        mResampler = std::make_unique<AudioResampler>(
            mAudioSettings.SourceSampleRate,
            mAudioSettings.SampleRate,
            GetNumOutputChannels());
    }
    else
    {
        SPDLOG_INFO("AudioPlayer: resampler disabled {} Hz → {} Hz, {} ch",
                    mAudioSettings.SourceSampleRate, mAudioSettings.SampleRate,
                    GetNumOutputChannels());
        mResampler = nullptr;
    }

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
    if (mAudioSettings.SourceSampleRate > 0 &&
        mAudioSettings.SourceSampleRate != mAudioSettings.SampleRate) {
        return (int32_t)((int64_t)mAudioSettings.DesiredBuffered
                         * mAudioSettings.SampleRate
                         / mAudioSettings.SourceSampleRate);
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
        mSoundMatrixDecoder.reset();
    }

    // Rebuild resampler with new channel count
    if (mAudioSettings.SourceSampleRate != mAudioSettings.SampleRate &&
        mAudioSettings.SourceSampleRate > 0) {
        mResampler = std::make_unique<AudioResampler>(
            mAudioSettings.SourceSampleRate,
            mAudioSettings.SampleRate,
            GetNumOutputChannels());
    }

    return DoInit();
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
    // Step 1: surround decode if needed (stereo → 5.1)
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
        pcm    = surroundBuf.data();
        pcmLen = surroundLen;
    }

    // Step 2: resample if source rate ≠ output rate
    if (mResampler) {
        const int ch = GetNumOutputChannels();
        const int32_t inFrames  = static_cast<int32_t>(pcmLen / (sizeof(int16_t) * ch));
        const int32_t maxOut    = mResampler->MaxOutputFrames(inFrames);

        assert(static_cast<size_t>(maxOut * ch) <= kResampleBufSamples
               && "Resample output exceeds kResampleBufSamples — increase the buffer size");

        const int32_t outFrames = mResampler->Process(
            reinterpret_cast<const int16_t*>(pcm),
            inFrames,
            mResampleBuf.data(),
            maxOut);

        DoPlay(reinterpret_cast<const uint8_t*>(mResampleBuf.data()),
               static_cast<size_t>(outFrames * ch * sizeof(int16_t)));
        return;
    }

    // Step 3: passthrough (no resampling needed)
    DoPlay(pcm, pcmLen);
}

} // namespace Ship
