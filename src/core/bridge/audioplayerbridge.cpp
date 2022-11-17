#include "audioplayerbridge.h"
#include "core/Window.h"

extern "C" {

bool AudioPlayerInit(void) {
    auto audio = Ship::Window::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return false;
    }

    return audio->Init();
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
