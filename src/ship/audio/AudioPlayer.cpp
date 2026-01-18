#include "ship/audio/AudioPlayer.h"
#include "spdlog/spdlog.h"
#include <algorithm>

namespace Ship {
AudioPlayer::~AudioPlayer() {
    SPDLOG_TRACE("destruct audio player");
}

bool AudioPlayer::Init() {
    // Initialize PLII decoder if surround mode is enabled
    if (mAudioSettings.AudioSurround == AudioChannelsSetting::audioSurround51) {
        SPDLOG_INFO("Initializing Dolby Pro Logic II decoder");
        mPLIIDecoder = std::make_unique<DolbyProLogicIIDecoder>(mAudioSettings.SampleRate);
        // Mark that we need to prime the delay buffers with real audio
        mNeedsPriming = true;
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

int32_t AudioPlayer::GetSampleLength() const {
    return mAudioSettings.SampleLength;
}

int32_t AudioPlayer::GetDesiredBuffered() const {
    return mAudioSettings.DesiredBuffered;
}

AudioChannelsSetting AudioPlayer::GetAudioChannels() const {
    return mAudioSettings.AudioSurround;
}

void AudioPlayer::SetSampleRate(int32_t rate) {
    mAudioSettings.SampleRate = rate;
}

void AudioPlayer::SetSampleLength(int32_t length) {
    mAudioSettings.SampleLength = length;
}

void AudioPlayer::SetDesiredBuffered(int32_t size) {
    mAudioSettings.DesiredBuffered = size;
}

bool AudioPlayer::SetAudioChannels(AudioChannelsSetting channels) {
    if (mAudioSettings.AudioSurround == channels) {
        return true;  // No change needed
    }

    SPDLOG_INFO("Changing audio channels from {} to {}",
                mAudioSettings.AudioSurround == AudioChannelsSetting::audioSurround51 ? "5.1 Surround" : "Stereo",
                channels == AudioChannelsSetting::audioSurround51 ? "5.1 Surround" : "Stereo");

    // Skip frames after switching to surround to let filters settle
    if (channels == AudioChannelsSetting::audioSurround51) {
        mNeedsPriming = true;
    }

    // Close current audio device
    DoClose();
    mInitialized = false;

    // Update channel setting
    mAudioSettings.AudioSurround = channels;

    // Setup or teardown PLII decoder
    if (channels == AudioChannelsSetting::audioSurround51) {
        if (!mPLIIDecoder) {
            mPLIIDecoder = std::make_unique<DolbyProLogicIIDecoder>(mAudioSettings.SampleRate);
        } else {
            // Reset filter state to prevent glitches from old filter history
            mPLIIDecoder->Reset();
        }
        // Clear and reserve surround buffer
        mSurroundBuffer.clear();
        mSurroundBuffer.reserve(4096 * 6);
    } else {
        mPLIIDecoder.reset();
        mSurroundBuffer.clear();
        mSurroundBuffer.shrink_to_fit();
    }

    // Reinitialize with new settings
    mInitialized = DoInit();

    return mInitialized;
}

int32_t AudioPlayer::GetNumOutputChannels() const {
    return mAudioSettings.AudioSurround == AudioChannelsSetting::audioSurround51 ? 6 : 2;
}

void AudioPlayer::Play(const uint8_t* buf, size_t len) {
    if (mAudioSettings.AudioSurround == AudioChannelsSetting::audioSurround51 && mPLIIDecoder) {
        // Input is stereo, decode to surround
        const int16_t* stereoIn = reinterpret_cast<const int16_t*>(buf);
        int numStereoSamples = len / (2 * sizeof(int16_t));  // Number of stereo sample pairs

        // Resize surround buffer if needed
        size_t surroundSamplesNeeded = numStereoSamples * 6;
        if (mSurroundBuffer.size() < surroundSamplesNeeded) {
            mSurroundBuffer.resize(surroundSamplesNeeded);
        }

        // Prime the PLII decoder by running the first audio through multiple times
        // This fills the delay buffers with real audio data instead of zeros
        if (mNeedsPriming) {
            mNeedsPriming = false;
            // Run the first buffer through ~10 times to fill delay buffers (~10ms at 32kHz)
            // The delay is 320 samples, and each buffer is typically 528-560 samples
            for (int i = 0; i < 1000; i++) {
                mPLIIDecoder->Process(stereoIn, mSurroundBuffer.data(), numStereoSamples);
            }
        }

        // Decode stereo to surround using PLII
        mPLIIDecoder->Process(stereoIn, mSurroundBuffer.data(), numStereoSamples);


        // Play the surround audio
        DoPlay(reinterpret_cast<const uint8_t*>(mSurroundBuffer.data()),
               numStereoSamples * 6 * sizeof(int16_t));
    } else {
        // Stereo passthrough
        DoPlay(buf, len);
    }
}
} // namespace Ship
