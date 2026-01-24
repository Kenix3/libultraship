#include "ship/audio/AudioPlayer.h"
#include "spdlog/spdlog.h"

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
    return mAudioSettings.ChannelSetting;
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
        } else {
            mSoundMatrixDecoder.reset();
        }
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
    if (mAudioSettings.ChannelSetting != AudioChannelsSetting::audioMatrix51) {
        // Stereo or Raw 5.1 passthrough
        DoPlay(buf, len);
        return;
    }

    if (!mSoundMatrixDecoder) {
        SPDLOG_ERROR("AudioPlayer: Matrix 5.1 mode enabled but SoundMatrixDecoder is not initialized");
        return;
    }

    // Input is stereo, decode to surround using matrix decoder
    const int16_t* stereoIn = reinterpret_cast<const int16_t*>(buf);
    int numStereoSamples = len / (2 * sizeof(int16_t)); // Number of stereo sample pairs

    // Decode stereo to surround using sound matrix decoder
    const int16_t* surroundOut = mSoundMatrixDecoder->Process(stereoIn, numStereoSamples);
    if (!surroundOut) {
        SPDLOG_ERROR("AudioPlayer: SoundMatrixDecoder::Process failed - decoder not initialized");
        return;
    }

    // Play the audio
    DoPlay(reinterpret_cast<const uint8_t*>(surroundOut), numStereoSamples * 6 * sizeof(int16_t));
}
} // namespace Ship
