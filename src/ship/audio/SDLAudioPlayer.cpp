#include "ship/audio/SDLAudioPlayer.h"
#include <spdlog/spdlog.h>

namespace Ship {

SDLAudioPlayer::~SDLAudioPlayer() {
    SPDLOG_TRACE("destruct SDL audio player");
    DoClose();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void SDLAudioPlayer::DoClose() {
    if (mStream != nullptr) {
        SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(mStream));
        SDL_ClearAudioStream(mStream);
        SDL_DestroyAudioStream(mStream);
        mStream = nullptr;
    }
}

bool SDLAudioPlayer::DoInit() {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SPDLOG_ERROR("SDL init error: {}", SDL_GetError());
        return false;
    }

    // Always open with the correct number of output channels
    mNumChannels = this->GetNumOutputChannels();

    const SDL_AudioSpec spec = { SDL_AUDIO_S16, mNumChannels, this->GetSampleRate() };
    mStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (mStream == nullptr) {
        SPDLOG_ERROR("SDL_OpenAudioDeviceStream error: {}", SDL_GetError());
        return false;
    }

    SPDLOG_INFO("SDL Audio initialized: {} channels, {} Hz", mNumChannels, this->GetSampleRate());

    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(mStream));
    return true;
}

int SDLAudioPlayer::Buffered() {
    // SDL_GetAudioStreamQueued is the SDL3 replacement for the producer-side queue size used by SDL2.
    // SDL_GetAudioStreamAvailable reports converted data for callers that read from the stream instead.
    return mStream == nullptr ? 0 : SDL_GetAudioStreamQueued(mStream) / (sizeof(int16_t) * mNumChannels);
}

void SDLAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
    if (Buffered() < 6000) {
        // Don't fill the audio buffer too much in case this happens
        SDL_PutAudioStreamData(mStream, buf, static_cast<int>(len));
    }
}
} // namespace Ship
