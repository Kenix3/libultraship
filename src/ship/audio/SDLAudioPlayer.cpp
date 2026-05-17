#include "ship/audio/SDLAudioPlayer.h"
#include <spdlog/spdlog.h>

namespace Ship {

SDLAudioPlayer::~SDLAudioPlayer() {
    SPDLOG_TRACE("destruct SDL audio player");
    DoClose();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void SDLAudioPlayer::DoClose() {
    if (mDevice != 0) {
        // Pause playback first
        SDL_PauseAudioDevice(mDevice, 1);

        // Prevent stale audio when reopening
        SDL_ClearQueuedAudio(mDevice);
        SDL_CloseAudioDevice(mDevice);
        mDevice = 0;
    }
}

bool SDLAudioPlayer::DoInit() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        SPDLOG_ERROR("SDL init error: {}", SDL_GetError());
        return false;
    }

    SDL_AudioSpec want;
    SDL_AudioSpec have;
    SDL_zero(want);

    want.freq = GetSampleRate();
    want.format = AUDIO_S16SYS;
    want.channels = GetNumOutputChannels();

    // Increase backend/device buffering slightly
    // to reduce rare underruns/glitches.
    //
    // IMPORTANT:
    // This is NOT part of the emulator buffering logic.
    // It only affects SDL/backend latency.
    want.samples = GetSampleLength() * 2;

    want.callback = nullptr;

    mDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);

    if (mDevice == 0) {
        SPDLOG_ERROR("SDL_OpenAudioDevice error: {}", SDL_GetError());
        return false;
    }

    mNumChannels = have.channels;

    SPDLOG_INFO(
        "SDL audio initialized: {} Hz, {} channels, {} samples",
        have.freq,
        have.channels,
        have.samples
    );

    SDL_PauseAudioDevice(mDevice, 0);
    return true;
}

int SDLAudioPlayer::Buffered() {
    return SDL_GetQueuedAudioSize(mDevice) / (sizeof(int16_t) * mNumChannels);
}

void SDLAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
    SDL_QueueAudio(mDevice, buf, len);
}
} // namespace Ship
