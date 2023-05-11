#include "core/bridge/audioplayerbridge.h"
#include "core/Context.h"
#include "menu/Gui.h"

extern "C" {

bool AudioPlayerInit(const char* backend) {
    auto audio = LUS::Context::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return false;
    }
    if (audio->Init()) {
        return true;
    }

    LUS::Context::GetInstance()->InitAudioPlayer(backend);
    if (audio->Init()) {
        return true;
    }

    return false;
}

int32_t AudioPlayerBuffered(void) {
    auto audio = LUS::Context::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    return audio->Buffered();
}

int32_t AudioPlayerGetDesiredBuffered(void) {
    auto audio = LUS::Context::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return 0;
    }

    return audio->GetDesiredBuffered();
}

void AudioPlayerPlayFrame(const uint8_t* buf, size_t len) {
    auto audio = LUS::Context::GetInstance()->GetAudioPlayer();
    if (audio == nullptr) {
        return;
    }

    audio->Play(buf, len);
}
}
