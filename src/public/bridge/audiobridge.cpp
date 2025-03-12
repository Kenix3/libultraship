#include "public/bridge/audiobridge.h"
#include "Context.h"
#include "audio/Audio.h"

extern "C" {

int32_t AudioPlayerBuffered() {
    auto audio = Ship::Context::GetInstance()->GetAudio()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    if (!audio->IsInitialized()) {
        return 0;
    }

    return audio->Buffered();
}

int32_t AudioPlayerGetDesiredBuffered() {
    auto audio = Ship::Context::GetInstance()->GetAudio()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    if (!audio->IsInitialized()) {
        return 0;
    }

    return audio->GetDesiredBuffered();
}

AudioSurroundSetting GetAudioSurround() {
    auto audio = Ship::Context::GetInstance()->GetAudio()->GetAudioPlayer();

    if (audio == nullptr) {
        return stereo;
    }

    return audio->GetAudioSurround();
}

int32_t GetNumAudioChannels() {
    return GetAudioSurround() == surround51 ? 6 : 2;
}

void AudioPlayerPlayFrame(const uint8_t* buf, size_t len) {
    auto audio = Ship::Context::GetInstance()->GetAudio()->GetAudioPlayer();
    if (audio == nullptr) {
        return;
    }

    if (!audio->IsInitialized()) {
        return;
    }

    audio->Play(buf, len);
}
}
