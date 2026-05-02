#include "libultraship/bridge/audiobridge.h"
#include "ship/Context.h"
#include "ship/audio/Audio.h"

// Audio bridge functions require a Ship::Audio component as a direct child of the Context.

extern "C" {

int32_t AudioPlayerBuffered() {
    auto audio = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Audio>()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    if (!audio->IsInitialized()) {
        return 0;
    }

    return audio->Buffered();
}

int32_t AudioPlayerGetDesiredBuffered() {
    auto audio = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Audio>()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    if (!audio->IsInitialized()) {
        return 0;
    }

    return audio->GetDesiredBuffered();
}

AudioChannelsSetting GetAudioChannels() {
    auto audio = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Audio>()->GetAudioPlayer();

    if (audio == nullptr) {
        return audioStereo;
    }

    return audio->GetAudioChannels();
}

int32_t GetNumAudioChannels() {
    auto audio = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Audio>()->GetAudioPlayer();

    if (audio == nullptr) {
        return 2;
    }

    return audio->GetNumOutputChannels();
}

void AudioPlayerPlayFrame(const uint8_t* buf, size_t len) {
    auto audio = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Audio>()->GetAudioPlayer();
    if (audio == nullptr) {
        return;
    }

    if (!audio->IsInitialized()) {
        return;
    }

    audio->Play(buf, len);
}

void SetAudioChannels(AudioChannelsSetting channels) {
    auto audio = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Audio>();
    if (audio == nullptr) {
        return;
    }

    audio->SetAudioChannels(channels);
}
}
