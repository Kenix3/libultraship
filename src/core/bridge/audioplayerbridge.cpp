#include "core/bridge/audioplayerbridge.h"
#include "core/Window.h"
#include "menu/ImGuiImpl.h"

extern "C" {

bool AudioPlayerInit(void) {
    auto audio = Ship::Window::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return false;
    }
    if (audio->Init()) {
        return true;
    }

    // loop over available audio apis if current fails
    auto audioBackends = SohImGui::GetAvailableAudioBackends();
    for (uint8_t i = 0; i < audioBackends.size(); i++) {
        SohImGui::SetCurrentAudioBackend(i, audioBackends[i]);
        Ship::Window::GetInstance()->InitializeAudioPlayer(audioBackends[i].first);
        if (audio->Init()) {
            return true;
        }
    }

    return false;
}

int32_t AudioPlayerBuffered(void) {
    auto audio = Ship::Window::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    return audio->Buffered();
}

int32_t AudioPlayerGetDesiredBuffered(void) {
    auto audio = Ship::Window::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    return audio->GetDesiredBuffered();
}

void AudioPlayerPlayFrame(const uint8_t* buf, size_t len) {
    auto audio = Ship::Window::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return;
    }

    audio->Play(buf, len);
}
}
