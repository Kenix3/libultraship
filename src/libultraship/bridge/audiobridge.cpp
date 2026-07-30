#include "libultraship/bridge/audiobridge.h"
#include "ship/audio/Audio.h"
#include <atomic>

static std::atomic<std::shared_ptr<Ship::Audio>> sAudio;

void AudioSetAudioComponent(std::shared_ptr<Ship::Audio> audio) {
    sAudio.store(std::move(audio), std::memory_order_release);
}

std::shared_ptr<Ship::Audio> AudioGetAudioComponent() {
    return sAudio.load(std::memory_order_acquire);
}

// Audio bridge functions require a Ship::Audio component as a direct child of the Context.

extern "C" {

int32_t AudioPlayerBuffered() {
    auto audioComponent = AudioGetAudioComponent();
    if (audioComponent == nullptr) {
        return 0;
    }

    auto audio = audioComponent->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    if (!audio->IsInitialized()) {
        return 0;
    }

    return audio->Buffered();
}

int32_t AudioPlayerGetDesiredBuffered() {
    auto audioComponent = AudioGetAudioComponent();
    if (audioComponent == nullptr) {
        return 0;
    }

    auto audio = audioComponent->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    if (!audio->IsInitialized()) {
        return 0;
    }

    return audio->GetDesiredBuffered();
}

AudioChannelsSetting GetAudioChannels() {
    auto audioComponent = AudioGetAudioComponent();
    if (audioComponent == nullptr) {
        return audioStereo;
    }

    auto audio = audioComponent->GetAudioPlayer();
    if (audio == nullptr) {
        return audioStereo;
    }

    return audio->GetAudioChannels();
}

int32_t GetNumAudioChannels() {
    auto audioComponent = AudioGetAudioComponent();
    if (audioComponent == nullptr) {
        return 2;
    }

    auto audio = audioComponent->GetAudioPlayer();
    if (audio == nullptr) {
        return 2;
    }

    return audio->GetNumOutputChannels();
}

void AudioPlayerPlayFrame(const uint8_t* buf, size_t len) {
    auto audioComponent = AudioGetAudioComponent();
    if (audioComponent == nullptr) {
        return;
    }

    auto audio = audioComponent->GetAudioPlayer();
    if (audio == nullptr) {
        return;
    }

    if (!audio->IsInitialized()) {
        return;
    }

    audio->Play(buf, len);
}

void SetAudioChannels(AudioChannelsSetting channels) {
    auto audio = AudioGetAudioComponent();
    if (audio == nullptr) {
        return;
    }

    audio->SetAudioChannels(channels);
}
}
