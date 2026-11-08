#include "ship/audio/SDLAudioPlayer.h"
#include <spdlog/spdlog.h>

#include <algorithm>
#include <bit>
#include <cstring>

namespace Ship {

SDLAudioPlayer::~SDLAudioPlayer() {
    SPDLOG_TRACE("destruct SDL audio player");
    DoClose();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void SDLAudioPlayer::RingInit(uint32_t bytes) {
    const uint32_t size = std::bit_ceil(bytes);
    mRing.assign(size, 0);
    mRingMask = size - 1;
    mRingReadPos = mRingWritePos = 0;
}

int32_t SDLAudioPlayer::RingAvailable() const {
    return static_cast<int32_t>(mRingWritePos - mRingReadPos);
}

int32_t SDLAudioPlayer::RingSpace() const {
    return static_cast<int32_t>(mRing.size()) - RingAvailable();
}

int32_t SDLAudioPlayer::RingRead(uint8_t* dst, int32_t bytes) {
    bytes = std::min(bytes, RingAvailable());
    if (bytes <= 0) {
        return 0;
    }

    const uint32_t offset = static_cast<uint32_t>(mRingReadPos) & mRingMask;
    const uint32_t chunk = std::min(static_cast<uint32_t>(mRing.size()) - offset, static_cast<uint32_t>(bytes));

    std::memcpy(dst, mRing.data() + offset, chunk);
    if (chunk < static_cast<uint32_t>(bytes)) {
        std::memcpy(dst + chunk, mRing.data(), bytes - chunk);
    }

    mRingReadPos += bytes;
    return bytes;
}

int32_t SDLAudioPlayer::RingWrite(const uint8_t* src, int32_t bytes) {
    bytes = std::min(bytes, RingSpace());
    if (bytes <= 0) {
        return 0;
    }

    const uint32_t offset = static_cast<uint32_t>(mRingWritePos) & mRingMask;
    const uint32_t chunk = std::min(static_cast<uint32_t>(mRing.size()) - offset, static_cast<uint32_t>(bytes));

    std::memcpy(mRing.data() + offset, src, chunk);
    if (chunk < static_cast<uint32_t>(bytes)) {
        std::memcpy(mRing.data(), src + chunk, bytes - chunk);
    }

    mRingWritePos += bytes;
    return bytes;
}

void SDLAudioPlayer::DoClose() {
    mRunning.store(false, std::memory_order_release);

    if (mDevice != 0) {
        // Closing pauses the device and joins SDL's audio thread, so no callback can run past this
        // point and the buffers below are safe to free.
        SDL_CloseAudioDevice(mDevice);
        mDevice = 0;
    }

    mRing.clear();
    mRing.shrink_to_fit();
    mRingMask = 0;
    mRingReadPos = mRingWritePos = 0;

    mLastChunk.clear();
    mLastChunk.shrink_to_fit();
    mLastChunkValid = false;
    mUnderrunFaded = false;
}

bool SDLAudioPlayer::DoInit() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        SPDLOG_ERROR("SDL init error: {}", SDL_GetError());
        return false;
    }

    // Always open with the correct number of output channels
    mNumChannels = this->GetNumOutputChannels();

    const uint32_t bytesPerFrame = sizeof(int16_t) * static_cast<uint32_t>(mNumChannels);
    const uint32_t desiredBuffered = static_cast<uint32_t>(this->GetDesiredBuffered());

    // Twice what the game aims to keep buffered, so a burst never meets a full ring during normal
    // playback. RingInit rounds up to a power of two, which adds a little more headroom.
    RingInit(desiredBuffered * 2 * bytesPerFrame);

    // Sized for a whole producer chunk; a single device burst is much smaller than that.
    mLastChunk.assign(desiredBuffered * bytesPerFrame, 0);
    mLastChunkValid = false;
    mUnderrunFaded = false;

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = this->GetSampleRate();
    want.format = AUDIO_S16SYS;
    want.channels = mNumChannels;
    want.samples = this->GetSampleLength();
    want.callback = AudioCallback;
    want.userdata = this;

    // No allowed changes, so SDL converts internally and `have` matches what was asked for.
    mDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (mDevice == 0) {
        SPDLOG_ERROR("SDL_OpenAudioDevice error: {}", SDL_GetError());
        DoClose();
        return false;
    }

    SPDLOG_INFO("SDL Audio initialized: {} channels, {} Hz", mNumChannels, this->GetSampleRate());

    mRunning.store(true, std::memory_order_release);
    SDL_PauseAudioDevice(mDevice, 0);
    return true;
}

int SDLAudioPlayer::Buffered() {
    std::lock_guard<std::mutex> lock(mMutex);
    return RingAvailable() / (static_cast<int32_t>(sizeof(int16_t)) * mNumChannels);
}

void SDLAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
    if (!mRunning.load(std::memory_order_acquire)) {
        return;
    }

    std::lock_guard<std::mutex> lock(mMutex);

    // Whole chunk or nothing: a partial write would leave a gap mid-chunk, which is audible as a click.
    if (RingSpace() >= static_cast<int32_t>(len)) {
        RingWrite(buf, static_cast<int32_t>(len));
    }
}

void SDLCALL SDLAudioPlayer::AudioCallback(void* userdata, Uint8* stream, int len) {
    auto* self = static_cast<SDLAudioPlayer*>(userdata);

    // SDL2 requires the whole buffer to be written, so every path below fills all of `len`.
    if (len <= 0) {
        return;
    }
    if (!self->mRunning.load(std::memory_order_acquire)) {
        std::memset(stream, 0, static_cast<size_t>(len));
        return;
    }

    // This runs on the audio thread and must never block: if the game thread holds the lock, play
    // silence for this burst rather than waiting for it.
    if (!self->mMutex.try_lock()) {
        std::memset(stream, 0, static_cast<size_t>(len));
        return;
    }

    const int32_t stride = static_cast<int32_t>(sizeof(int16_t)) * self->mNumChannels;
    const int32_t lastChunkSize = static_cast<int32_t>(self->mLastChunk.size());

    if (self->RingAvailable() >= len) {
        self->RingRead(stream, len);
        self->mUnderrunFaded = false;

        // Keep it for the underrun path below.
        if (len <= lastChunkSize) {
            std::memcpy(self->mLastChunk.data(), stream, static_cast<size_t>(len));
            self->mLastChunkValid = true;
        }
    } else if (self->mLastChunkValid && !self->mUnderrunFaded) {
        // Underrun: the game thread has not kept up. Replay the last burst faded to zero, which the ear
        // reads as the sound tailing off rather than as the click a hole in the stream produces.
        const int32_t copy = std::min(len, lastChunkSize);
        std::memcpy(stream, self->mLastChunk.data(), static_cast<size_t>(copy));
        if (copy < len) {
            std::memset(stream + copy, 0, static_cast<size_t>(len - copy));
        }

        // SoH always produces S16, which AUDIO_S16SYS in DoInit() matches.
        const int32_t frames = len / stride;
        auto* samples = reinterpret_cast<int16_t*>(stream);
        for (int32_t frame = 0; frame < frames; ++frame) {
            const float gain = 1.0f - static_cast<float>(frame) / static_cast<float>(frames);
            for (int32_t channel = 0; channel < self->mNumChannels; ++channel) {
                auto& sample = samples[frame * self->mNumChannels + channel];
                sample = static_cast<int16_t>(static_cast<float>(sample) * gain);
            }
        }

        // Only the first burst fades; repeating it would turn a stall into a stutter.
        self->mUnderrunFaded = true;
    } else {
        // Nothing played yet, or the fade already ran: stay quiet until the game thread catches up.
        std::memset(stream, 0, static_cast<size_t>(len));
    }

    self->mMutex.unlock();
}
} // namespace Ship
