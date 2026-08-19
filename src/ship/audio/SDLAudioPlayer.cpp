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

    if (mStream != nullptr) {
        // Destroying the stream stops the callback and closes the bound device. SDL joins its audio
        // thread first, so no callback can run past this point and the buffers below are safe to free.
        SDL_DestroyAudioStream(mStream);
        mStream = nullptr;
    }

    mRing.clear();
    mRing.shrink_to_fit();
    mRingMask = 0;
    mRingReadPos = mRingWritePos = 0;

    mScratch.clear();
    mScratch.shrink_to_fit();
}

bool SDLAudioPlayer::DoInit() {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
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
    mScratch.assign(desiredBuffered * bytesPerFrame, 0);

    const SDL_AudioSpec spec = { SDL_AUDIO_S16, mNumChannels, this->GetSampleRate() };
    mStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, AudioStreamCallback, this);
    if (mStream == nullptr) {
        SPDLOG_ERROR("SDL_OpenAudioDeviceStream error: {}", SDL_GetError());
        DoClose();
        return false;
    }

    SPDLOG_INFO("SDL Audio initialized: {} channels, {} Hz", mNumChannels, this->GetSampleRate());

    mRunning.store(true, std::memory_order_release);
    SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(mStream));
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

void SDLCALL SDLAudioPlayer::AudioStreamCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount,
                                                 int /*totalAmount*/) {
    auto* self = static_cast<SDLAudioPlayer*>(userdata);

    // SDL also calls with 0 purely as a notification.
    if (additionalAmount <= 0 || !self->mRunning.load(std::memory_order_acquire)) {
        return;
    }

    const int32_t stride = static_cast<int32_t>(sizeof(int16_t)) * self->mNumChannels;
    // SDL asks for frame-aligned byte counts, but round down defensively.
    const int32_t bytes = (additionalAmount / stride) * stride;
    if (bytes <= 0) {
        return;
    }

    // Only grows if SDL ever asks for more than a whole producer chunk; it settles after that.
    if (self->mScratch.size() < static_cast<size_t>(bytes)) {
        self->mScratch.resize(static_cast<size_t>(bytes));
    }
    uint8_t* dst = self->mScratch.data();

    // This runs on the audio thread and must never block: if the game thread holds the lock, play
    // silence for this burst rather than waiting for it.
    if (!self->mMutex.try_lock()) {
        std::memset(dst, 0, static_cast<size_t>(bytes));
        SDL_PutAudioStreamData(stream, dst, bytes);
        return;
    }

    if (self->RingAvailable() >= bytes) {
        self->RingRead(dst, bytes);
    } else {
        // Underrun: the game thread has not kept up, so fill the gap with silence.
        std::memset(dst, 0, static_cast<size_t>(bytes));
    }

    self->mMutex.unlock();

    SDL_PutAudioStreamData(stream, dst, bytes);
}
} // namespace Ship
