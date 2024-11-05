#include "Audio.h"

#include "Context.h"

namespace Ship {

Audio::~Audio() {
    SPDLOG_TRACE("destruct audio");
}

void Audio::InitAudioPlayer() {
    switch (GetCurrentAudioBackend()) {
#ifdef _WIN32
        case AudioBackend::WASAPI:
            mAudioPlayer = std::make_shared<WasapiAudioPlayer>(this->mAudioSettings);
            break;
#endif
        default:
            mAudioPlayer = std::make_shared<SDLAudioPlayer>(this->mAudioSettings);
    }

    if (mAudioPlayer) {
        if (!mAudioPlayer->Init()) {
            // Failed to initialize system audio player.
            // Fallback to SDL if the native system player does not work.
            SetCurrentAudioBackend(AudioBackend::SDL);
            mAudioPlayer = std::make_shared<SDLAudioPlayer>(this->mAudioSettings);
            mAudioPlayer->Init();
        }
    }
}

void Audio::Init() {
    mAvailableAudioBackends = std::make_shared<std::vector<AudioBackend>>();
#ifdef _WIN32
    mAvailableAudioBackends->push_back(AudioBackend::WASAPI);
#endif
    mAvailableAudioBackends->push_back(AudioBackend::SDL);

    SetCurrentAudioBackend(Context::GetInstance()->GetConfig()->GetCurrentAudioBackend());
}

std::shared_ptr<AudioPlayer> Audio::GetAudioPlayer() {
    return mAudioPlayer;
}

AudioBackend Audio::GetCurrentAudioBackend() {
    return mAudioBackend;
}

void Audio::SetCurrentAudioBackend(AudioBackend backend) {
    mAudioBackend = backend;
    Context::GetInstance()->GetConfig()->SetCurrentAudioBackend(GetCurrentAudioBackend());
    Context::GetInstance()->GetConfig()->Save();

    InitAudioPlayer();
}

std::shared_ptr<std::vector<AudioBackend>> Audio::GetAvailableAudioBackends() {
    return mAvailableAudioBackends;
}

} // namespace Ship
