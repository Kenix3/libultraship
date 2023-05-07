#include "core/bridge/audioplayerbridge.h"
#include "core/Context.h"
#include "menu/ImGuiImpl.h"

extern "C" {

bool AudioPlayerInit(void) {
    auto audio = Ship::Context::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return false;
    }
    if (audio->Init()) {
        return true;
    }

    // loop over available audio apis if current fails
    auto audioBackends = Ship::GetAvailableAudioBackends();
    for (uint8_t i = 0; i < audioBackends.size(); i++) {
        Ship::SetCurrentAudioBackend(i, audioBackends[i]);
        Ship::Context::GetInstance()->InitAudioPlayer(audioBackends[i].first);
        if (audio->Init()) {
            return true;
        }
    }

    return false;
}

int32_t AudioPlayerBuffered(void) {
    auto audio = Ship::Context::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    return audio->Buffered();
}

int32_t AudioPlayerGetDesiredBuffered(void) {
    auto audio = Ship::Context::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    return audio->GetDesiredBuffered();
}

void AudioPlayerPlayFrame(const uint8_t* buf, size_t len) {
    auto audio = Ship::Context::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return;
    }

    audio->Play(buf, len);
}
}
