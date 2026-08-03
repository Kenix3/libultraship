#if ENABLE_SDL3

#include "ship/audio/SDL3AudioPlayer.h"
#include <spdlog/spdlog.h>

#include <atomic>
#include <cstring>
#include <mutex>
#include <vector>

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_error.h>

// SDL3 is loaded at runtime (see Impl::LoadSdl3); we include its headers for
// types/constants but never link it, so we need the platform's dynamic loader.
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Ship {

// ===========================================================================
// SDL3AudioPlayer::Impl — all SDL3 state and the audio-thread machinery.
// Because it lives entirely in this .cpp, the pull callback can take the real
// SDL_AudioStream* and SDL types appear nowhere in the header.
// ===========================================================================

struct SDL3AudioPlayer::Impl {
    bool Open(int sampleRate, int numChannels, int desiredBuffered);
    void Close();
    int Buffered();
    void Play(const uint8_t* buf, size_t len);

    // SDL3 pull callback (SDL_AudioStreamCallback). userdata is this Impl.
    static void SDLCALL AudioStreamCallback(void* userdata, SDL_AudioStream* stream, int additional_amount,
                                            int total_amount);

    // --- Dynamically-loaded SDL3 entry points ------------------------------
    // We dlopen() SDL3 with local visibility and call it only through these
    // pointers, rather than linking it. The engine also links SDL2 (window /
    // input / ImGui); on ELF that puts both libraries' SDL_* symbols in one
    // global table, and the names common to both (SDL_InitSubSystem,
    // SDL_GetError, ...) bind to SDL2 -- so SDL_InitSubSystem would init SDL2's
    // audio while the SDL3-only SDL_OpenAudioDeviceStream runs against SDL3's
    // uninitialised subsystem, and the stream open fails. Loading SDL3 privately
    // (RTLD_LOCAL) keeps its symbols out of the global table entirely. It also
    // makes SDL3 an optional *runtime* dependency: if it is missing, Open() fails
    // cleanly and Audio falls back to another backend instead of the executable
    // refusing to start.
    bool LoadSdl3();
    void UnloadSdl3();

    void* mSdl3Lib = nullptr;
    decltype(&SDL_InitSubSystem) pInitSubSystem = nullptr;
    decltype(&SDL_QuitSubSystem) pQuitSubSystem = nullptr;
    decltype(&SDL_GetError) pGetError = nullptr;
    decltype(&SDL_OpenAudioDeviceStream) pOpenAudioDeviceStream = nullptr;
    decltype(&SDL_ResumeAudioStreamDevice) pResumeAudioStreamDevice = nullptr;
    decltype(&SDL_PutAudioStreamData) pPutAudioStreamData = nullptr;
    decltype(&SDL_DestroyAudioStream) pDestroyAudioStream = nullptr;

    // --- Ring buffer: power-of-two size, unbounded read/write pointers ---
    struct RingBuffer {
        std::vector<uint8_t> data;
        uint32_t mask = 0; // data.size() - 1
        int64_t rdptr = 0;
        int64_t wrptr = 0;
    } mRing;

    void RingInit(uint32_t bytes);
    int32_t RingAvailable() const;
    int32_t RingSpace() const;
    int32_t RingRead(uint8_t* dst, int32_t bytes);
    int32_t RingWrite(const uint8_t* src, int32_t bytes);

    SDL_AudioStream* mStream = nullptr;
    bool mAudioSubsystemInitialized = false;
    int mNumChannels = 2;

    std::mutex mMutex;
    std::atomic<int> mRunning{ 0 };

    // Scratch the callback assembles a burst into before handing it to SDL3.
    // Pre-sized in Open so the audio thread does not allocate in steady state.
    std::vector<uint8_t> mScratch;

    // Underrun recovery: last successfully played chunk, replayed with fade-out.
    std::vector<uint8_t> mLastChunk;
    bool mLastChunkValid = false;
    bool mUnderrunFaded = false;
};

// ---------------------------------------------------------------------------
// Ring buffer helpers
// ---------------------------------------------------------------------------

static constexpr uint32_t NextPow2(uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}

void SDL3AudioPlayer::Impl::RingInit(uint32_t bytes) {
    uint32_t sz = NextPow2(bytes);
    mRing.data.assign(sz, 0);
    mRing.mask = sz - 1;
    mRing.rdptr = mRing.wrptr = 0;
}

int32_t SDL3AudioPlayer::Impl::RingAvailable() const {
    return static_cast<int32_t>(mRing.wrptr - mRing.rdptr);
}

int32_t SDL3AudioPlayer::Impl::RingSpace() const {
    return static_cast<int32_t>(mRing.data.size()) - RingAvailable();
}

int32_t SDL3AudioPlayer::Impl::RingRead(uint8_t* dst, int32_t bytes) {
    int32_t avail = RingAvailable();
    if (bytes > avail)
        bytes = avail;
    if (bytes <= 0)
        return 0;

    const uint32_t size = static_cast<uint32_t>(mRing.data.size());
    uint32_t off = static_cast<uint32_t>(mRing.rdptr & mRing.mask);
    uint32_t chunk = size - off;
    if (chunk > static_cast<uint32_t>(bytes))
        chunk = static_cast<uint32_t>(bytes);

    std::memcpy(dst, mRing.data.data() + off, chunk);
    if (chunk < static_cast<uint32_t>(bytes))
        std::memcpy(dst + chunk, mRing.data.data(), bytes - chunk);

    mRing.rdptr += bytes;
    return bytes;
}

int32_t SDL3AudioPlayer::Impl::RingWrite(const uint8_t* src, int32_t bytes) {
    int32_t space = RingSpace();
    if (bytes > space)
        bytes = space;
    if (bytes <= 0)
        return 0;

    const uint32_t size = static_cast<uint32_t>(mRing.data.size());
    uint32_t off = static_cast<uint32_t>(mRing.wrptr & mRing.mask);
    uint32_t chunk = size - off;
    if (chunk > static_cast<uint32_t>(bytes))
        chunk = static_cast<uint32_t>(bytes);

    std::memcpy(mRing.data.data() + off, src, chunk);
    if (chunk < static_cast<uint32_t>(bytes))
        std::memcpy(mRing.data.data(), src + chunk, bytes - chunk);

    mRing.wrptr += bytes;
    return bytes;
}

// ---------------------------------------------------------------------------
// Runtime SDL3 loading (see the note on the function-pointer members above)
// ---------------------------------------------------------------------------

bool SDL3AudioPlayer::Impl::LoadSdl3() {
    if (mSdl3Lib != nullptr) {
        return true; // already loaded
    }

#if defined(_WIN32)
    mSdl3Lib = static_cast<void*>(LoadLibraryA("SDL3.dll"));
#elif defined(__APPLE__)
    mSdl3Lib = dlopen("libSDL3.0.dylib", RTLD_NOW | RTLD_LOCAL);
#else
    mSdl3Lib = dlopen("libSDL3.so.0", RTLD_NOW | RTLD_LOCAL);
#endif
    if (mSdl3Lib == nullptr) {
        SPDLOG_ERROR("SDL3: unable to load the SDL3 library at runtime; SDL3 audio backend unavailable");
        return false;
    }

    auto resolve = [this](const char* name) -> void* {
#if defined(_WIN32)
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(mSdl3Lib), name));
#else
        return dlsym(mSdl3Lib, name);
#endif
    };

    pInitSubSystem = reinterpret_cast<decltype(pInitSubSystem)>(resolve("SDL_InitSubSystem"));
    pQuitSubSystem = reinterpret_cast<decltype(pQuitSubSystem)>(resolve("SDL_QuitSubSystem"));
    pGetError = reinterpret_cast<decltype(pGetError)>(resolve("SDL_GetError"));
    pOpenAudioDeviceStream = reinterpret_cast<decltype(pOpenAudioDeviceStream)>(resolve("SDL_OpenAudioDeviceStream"));
    pResumeAudioStreamDevice =
        reinterpret_cast<decltype(pResumeAudioStreamDevice)>(resolve("SDL_ResumeAudioStreamDevice"));
    pPutAudioStreamData = reinterpret_cast<decltype(pPutAudioStreamData)>(resolve("SDL_PutAudioStreamData"));
    pDestroyAudioStream = reinterpret_cast<decltype(pDestroyAudioStream)>(resolve("SDL_DestroyAudioStream"));

    if (pInitSubSystem == nullptr || pQuitSubSystem == nullptr || pGetError == nullptr ||
        pOpenAudioDeviceStream == nullptr || pResumeAudioStreamDevice == nullptr || pPutAudioStreamData == nullptr ||
        pDestroyAudioStream == nullptr) {
        SPDLOG_ERROR("SDL3: failed to resolve required SDL3 audio symbols");
        UnloadSdl3();
        return false;
    }

    return true;
}

void SDL3AudioPlayer::Impl::UnloadSdl3() {
    pInitSubSystem = nullptr;
    pQuitSubSystem = nullptr;
    pGetError = nullptr;
    pOpenAudioDeviceStream = nullptr;
    pResumeAudioStreamDevice = nullptr;
    pPutAudioStreamData = nullptr;
    pDestroyAudioStream = nullptr;

    if (mSdl3Lib != nullptr) {
#if defined(_WIN32)
        FreeLibrary(static_cast<HMODULE>(mSdl3Lib));
#else
        dlclose(mSdl3Lib);
#endif
        mSdl3Lib = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Open
// ---------------------------------------------------------------------------

bool SDL3AudioPlayer::Impl::Open(int sampleRate, int numChannels, int desiredBuffered) {
    if (!LoadSdl3()) {
        return false;
    }

    if (!pInitSubSystem(SDL_INIT_AUDIO)) {
        SPDLOG_ERROR("SDL3: SDL_InitSubSystem(SDL_INIT_AUDIO) failed: {}", pGetError());
        Close();
        return false;
    }
    mAudioSubsystemInitialized = true;

    mNumChannels = numChannels;

    const uint32_t bytesPerFrame = sizeof(int16_t) * static_cast<uint32_t>(mNumChannels);

    // Ring buffer: 2 x desiredBuffered frames (already scaled to output rate by
    // AudioPlayer::GetDesiredBuffered). Comfortable headroom above the producer
    // fill threshold so DoPlay() never hits the full-ring guard during normal
    // playback. The next-pow2 rounding in RingInit adds a small extra margin.
    const uint32_t ringFrames = static_cast<uint32_t>(desiredBuffered) * 2;
    RingInit(ringFrames * bytesPerFrame);

    // Last-chunk buffer for underrun fade-out: one desiredBuffered worth of
    // frames so it can always hold a complete producer chunk.
    const uint32_t lastChunkSize = static_cast<uint32_t>(desiredBuffered) * bytesPerFrame;
    mLastChunk.assign(lastChunkSize, 0);
    mLastChunkValid = false;
    mUnderrunFaded = false;

    // Pre-size the callback scratch so the audio thread does not allocate during
    // steady-state playback. A single device burst is far smaller than this.
    mScratch.assign(lastChunkSize, 0);

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.format = SDL_AUDIO_S16; // native-endian signed 16-bit; SoH produces S16
    spec.channels = mNumChannels;
    spec.freq = sampleRate;

    // Open the default playback device with our pull callback. SDL3 routes this
    // to the platform's native audio API (PipeWire / PulseAudio / ALSA on Linux,
    // CoreAudio on macOS, WASAPI on Windows, etc.).
    mStream = pOpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, AudioStreamCallback, this);
    if (mStream == nullptr) {
        SPDLOG_ERROR("SDL3: SDL_OpenAudioDeviceStream failed: {}", pGetError());
        Close();
        return false;
    }

    mRunning.store(1, std::memory_order_release);

    // Streams returned by SDL_OpenAudioDeviceStream start paused.
    if (!pResumeAudioStreamDevice(mStream)) {
        SPDLOG_ERROR("SDL3: SDL_ResumeAudioStreamDevice failed: {}", pGetError());
        Close();
        return false;
    }

    SPDLOG_INFO("SDL3 audio initialized: {} ch, {} Hz", mNumChannels, sampleRate);
    return true;
}

// ---------------------------------------------------------------------------
// Close
// ---------------------------------------------------------------------------

void SDL3AudioPlayer::Impl::Close() {
    mRunning.store(0, std::memory_order_release);

    if (mStream != nullptr) {
        // Destroying the stream stops the callback and closes the bound device.
        // SDL joins the audio thread, so no callback runs after this returns —
        // it is therefore safe to release the buffers below.
        if (pDestroyAudioStream != nullptr) {
            pDestroyAudioStream(mStream);
        }
        mStream = nullptr;
    }

    mRing.data.clear();
    mRing.data.shrink_to_fit();
    mRing.mask = 0;
    mRing.rdptr = mRing.wrptr = 0;

    mLastChunk.clear();
    mLastChunk.shrink_to_fit();
    mLastChunkValid = false;
    mUnderrunFaded = false;

    mScratch.clear();
    mScratch.shrink_to_fit();

    if (mAudioSubsystemInitialized) {
        if (pQuitSubSystem != nullptr) {
            pQuitSubSystem(SDL_INIT_AUDIO);
        }
        mAudioSubsystemInitialized = false;
    }

    // Release the library last; this nulls the function pointers, so it must come
    // after the pQuitSubSystem / pDestroyAudioStream calls above.
    UnloadSdl3();
}

// ---------------------------------------------------------------------------
// Buffered — returns queued frames (called from the producer / main thread)
// ---------------------------------------------------------------------------

int SDL3AudioPlayer::Impl::Buffered() {
    std::lock_guard<std::mutex> lock(mMutex);
    const int bytesPerFrame = static_cast<int>(sizeof(int16_t)) * mNumChannels;
    return RingAvailable() / bytesPerFrame;
}

// ---------------------------------------------------------------------------
// Play — push PCM from the producer / game thread into the ring buffer
// ---------------------------------------------------------------------------

void SDL3AudioPlayer::Impl::Play(const uint8_t* buf, size_t len) {
    if (!mRunning.load(std::memory_order_acquire))
        return;

    std::lock_guard<std::mutex> lock(mMutex);

    // Whole-chunk-or-nothing: refuse the incoming buffer if it does not fit
    // entirely. A partial write would create a PCM discontinuity audible as a
    // click. The producer in OTRAudio_Thread guards against this condition by
    // skipping a wake when there is no room for the smallest next burst.
    if (RingSpace() >= static_cast<int32_t>(len)) {
        RingWrite(buf, static_cast<int32_t>(len));
    }
}

// ---------------------------------------------------------------------------
// AudioStreamCallback — SDL3 RT callback, pulls audio from the ring buffer
// ---------------------------------------------------------------------------

void SDLCALL SDL3AudioPlayer::Impl::AudioStreamCallback(void* userdata, SDL_AudioStream* stream, int additional_amount,
                                                        int /*total_amount*/) {
    auto* self = static_cast<Impl*>(userdata);

    // additional_amount is the number of bytes SDL needs right now to keep the
    // device fed. SDL may also call with 0 purely as a notification.
    if (additional_amount <= 0)
        return;
    if (!self->mRunning.load(std::memory_order_acquire))
        return;

    const int stride = static_cast<int>(sizeof(int16_t)) * self->mNumChannels;
    // SDL requests frame-aligned byte counts, but round down defensively.
    const int32_t n_bytes = (additional_amount / stride) * stride;
    if (n_bytes <= 0)
        return;
    const int32_t n_frames = n_bytes / stride;

    // Grow the scratch only if SDL ever asks for more than the pre-sized burst
    // (rare; it stabilises after the first such callback).
    if (self->mScratch.size() < static_cast<size_t>(n_bytes))
        self->mScratch.resize(static_cast<size_t>(n_bytes));
    uint8_t* dst = self->mScratch.data();

    // trylock: this is a RT callback and must never block. If the producer holds
    // mMutex, emit silence rather than blocking — the underrun recovery on a
    // later callback masks the gap.
    if (!self->mMutex.try_lock()) {
        std::memset(dst, 0, static_cast<size_t>(n_bytes));
        self->pPutAudioStreamData(stream, dst, n_bytes);
        return;
    }

    const int32_t avail = self->RingAvailable();
    const int32_t lastChunkSize = static_cast<int32_t>(self->mLastChunk.size());

    if (avail >= n_bytes) {
        // Normal path: ring buffer has enough data.
        self->RingRead(dst, n_bytes);
        self->mUnderrunFaded = false;

        // Save for underrun recovery.
        if (n_bytes <= lastChunkSize) {
            std::memcpy(self->mLastChunk.data(), dst, static_cast<size_t>(n_bytes));
            self->mLastChunkValid = true;
        }
    } else if (self->mLastChunkValid && !self->mUnderrunFaded) {
        // Underrun recovery: replay the last chunk with a linear fade-out. A
        // faded repeat is perceptually far less harsh than a silence click.
        const uint32_t copy =
            (n_bytes <= lastChunkSize) ? static_cast<uint32_t>(n_bytes) : static_cast<uint32_t>(lastChunkSize);
        std::memcpy(dst, self->mLastChunk.data(), copy);
        if (copy < static_cast<uint32_t>(n_bytes))
            std::memset(dst + copy, 0, static_cast<size_t>(n_bytes) - copy);

        // Apply linear fade-out (S16 samples only — SoH always produces S16).
        for (int32_t i = 0; i < n_frames; ++i) {
            const double gain = 1.0 - static_cast<double>(i) / static_cast<double>(n_frames);
            for (int ch = 0; ch < self->mNumChannels; ++ch) {
                const int off = (i * self->mNumChannels + ch) * static_cast<int>(sizeof(int16_t));
                auto* s = reinterpret_cast<int16_t*>(dst + off);
                *s = static_cast<int16_t>(static_cast<double>(*s) * gain);
            }
        }
        self->mUnderrunFaded = true;
    } else {
        // Complete underrun: output silence.
        std::memset(dst, 0, static_cast<size_t>(n_bytes));
    }

    self->mMutex.unlock();

    self->pPutAudioStreamData(stream, dst, n_bytes);
}

// ===========================================================================
// SDL3AudioPlayer — thin AudioPlayer facade delegating to Impl.
// ===========================================================================

SDL3AudioPlayer::SDL3AudioPlayer(AudioSettings settings) : AudioPlayer(settings), mImpl(std::make_unique<Impl>()) {
}

SDL3AudioPlayer::~SDL3AudioPlayer() {
    SPDLOG_TRACE("destruct SDL3 audio player");
    mImpl->Close();
}

bool SDL3AudioPlayer::DoInit() {
    return mImpl->Open(GetSampleRate(), GetNumOutputChannels(), GetDesiredBuffered());
}

void SDL3AudioPlayer::DoClose() {
    mImpl->Close();
}

int SDL3AudioPlayer::Buffered() {
    return mImpl->Buffered();
}

void SDL3AudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
    mImpl->Play(buf, len);
}

} // namespace Ship

#endif // #if ENABLE_SDL3
