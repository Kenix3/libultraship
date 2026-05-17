#if ENABLE_PIPEWIRE

#include "ship/audio/PipeWireAudioPlayer.h"
#include <spdlog/spdlog.h>

#include <cassert>
#include <cstring>

namespace Ship {

// ---------------------------------------------------------------------------
// PipeWire stream event table
// ---------------------------------------------------------------------------

// Zero-initialize, then assign only the fields we need.
// Avoids positional init fragility if the struct gains fields in future versions.
struct pw_stream_events PipeWireAudioPlayer::sStreamEvents = {};

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

PipeWireAudioPlayer::PipeWireAudioPlayer(AudioSettings settings) : AudioPlayer(settings) {}

PipeWireAudioPlayer::~PipeWireAudioPlayer() {
    SPDLOG_TRACE("destruct PipeWire audio player");
    PipeWireAudioPlayer::DoClose();
}

// ---------------------------------------------------------------------------
// Ring buffer helpers
// ---------------------------------------------------------------------------

uint32_t PipeWireAudioPlayer::NextPow2(uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}

void PipeWireAudioPlayer::RingInit(uint32_t bytes) {
    uint32_t sz = NextPow2(bytes);
    mRing.data  = new uint8_t[sz]();
    mRing.size  = sz;
    mRing.mask  = sz - 1;
    mRing.rdptr = mRing.wrptr = 0;
}

void PipeWireAudioPlayer::RingDestroy() {
    delete[] mRing.data;
    mRing.data  = nullptr;
    mRing.size  = mRing.mask = 0;
    mRing.rdptr = mRing.wrptr = 0;
}

int32_t PipeWireAudioPlayer::RingAvailable() const {
    return static_cast<int32_t>(mRing.wrptr - mRing.rdptr);
}

int32_t PipeWireAudioPlayer::RingSpace() const {
    return static_cast<int32_t>(mRing.size) - RingAvailable();
}

int32_t PipeWireAudioPlayer::RingRead(uint8_t* dst, int32_t bytes) {
    int32_t avail = RingAvailable();
    if (bytes > avail)
        bytes = avail;
    if (bytes <= 0)
        return 0;

    uint32_t off   = mRing.rdptr & mRing.mask;
    uint32_t chunk = mRing.size - off;
    if (chunk > static_cast<uint32_t>(bytes))
        chunk = static_cast<uint32_t>(bytes);

    std::memcpy(dst, mRing.data + off, chunk);
    if (chunk < static_cast<uint32_t>(bytes))
        std::memcpy(dst + chunk, mRing.data, bytes - chunk);

    mRing.rdptr += bytes;
    return bytes;
}

int32_t PipeWireAudioPlayer::RingWrite(const uint8_t* src, int32_t bytes) {
    int32_t space = RingSpace();
    if (bytes > space)
        bytes = space;
    if (bytes <= 0)
        return 0;

    uint32_t off   = mRing.wrptr & mRing.mask;
    uint32_t chunk = mRing.size - off;
    if (chunk > static_cast<uint32_t>(bytes))
        chunk = static_cast<uint32_t>(bytes);

    std::memcpy(mRing.data + off, src, chunk);
    if (chunk < static_cast<uint32_t>(bytes))
        std::memcpy(mRing.data, src + chunk, bytes - chunk);

    mRing.wrptr += bytes;
    return bytes;
}

// ---------------------------------------------------------------------------
// DoInit
// ---------------------------------------------------------------------------

bool PipeWireAudioPlayer::DoInit() {
    pw_init(nullptr, nullptr);

    // Initialize stream event table here so pw_stream_events fields are
    // set before the stream is created.
    sStreamEvents.version       = PW_VERSION_STREAM_EVENTS;
    sStreamEvents.state_changed = PipeWireAudioPlayer::OnStateChanged;
    sStreamEvents.process       = PipeWireAudioPlayer::OnProcess;

    mNumChannels = GetNumOutputChannels();

    // Ring buffer: 2 × GetDesiredBuffered() frames (already scaled to output
    // rate by AudioPlayer::GetDesiredBuffered). This gives comfortable headroom
    // above the producer fill threshold so DoPlay() never hits the full-ring
    // guard during normal playback. The next-pow2 rounding adds a small extra
    // margin on top.
    const uint32_t bytesPerFrame = sizeof(int16_t) * static_cast<uint32_t>(mNumChannels);
    const uint32_t ringFrames = static_cast<uint32_t>(GetDesiredBuffered()) * 2;
    RingInit(ringFrames * bytesPerFrame);

    // Last-chunk buffer for underrun fade-out: one DesiredBuffered worth of
    // frames so it can always hold a complete producer chunk after resampling.
    mLastChunkSize  = static_cast<uint32_t>(GetDesiredBuffered()) * bytesPerFrame;
    mLastChunk      = new uint8_t[mLastChunkSize]();
    mLastChunkValid = false;
    mUnderrunFaded  = false;

    // Build the SPA format parameter describing our PCM stream.
    uint8_t             podBuf[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(podBuf, sizeof(podBuf));

    // Latency hint: SampleLength frames / sample rate (e.g. "1024/32000").
    char latencyStr[64];
    std::snprintf(latencyStr, sizeof(latencyStr), "%d/%d",
                  GetSampleLength(), GetSampleRate());

    struct spa_audio_info_raw audioInfo = {};
    audioInfo.format   = SPA_AUDIO_FORMAT_S16_LE;
    audioInfo.channels = static_cast<uint32_t>(mNumChannels);
    audioInfo.rate     = static_cast<uint32_t>(GetSampleRate());

    const struct spa_pod* params[1];
    params[0] = spa_format_audio_raw_build(
        &b, SPA_PARAM_EnumFormat, &audioInfo);

    // Create thread loop — PipeWire manages the RT thread for us.
    mLoop = pw_thread_loop_new("libultraship-pw", nullptr);
    if (!mLoop) {
        SPDLOG_ERROR("PipeWire: failed to create thread loop");
        DoClose();
        return false;
    }

    // Create the playback stream.
    mStream = pw_stream_new_simple(
        pw_thread_loop_get_loop(mLoop),
        "libultraship",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE,     "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE,     "Game",
            PW_KEY_NODE_LATENCY,   latencyStr,
            nullptr),
        &sStreamEvents,
        this);

    if (!mStream) {
        SPDLOG_ERROR("PipeWire: failed to create stream");
        DoClose();
        return false;
    }

    if (pw_stream_connect(mStream,
                          PW_DIRECTION_OUTPUT,
                          PW_ID_ANY,
                          static_cast<pw_stream_flags>(
                              PW_STREAM_FLAG_AUTOCONNECT |
                              PW_STREAM_FLAG_MAP_BUFFERS),
                          params, 1) < 0) {
        SPDLOG_ERROR("PipeWire: failed to connect stream");
        DoClose();
        return false;
    }

    if (pw_thread_loop_start(mLoop) < 0) {
        SPDLOG_ERROR("PipeWire: failed to start thread loop");
        DoClose();
        return false;
    }

    mRunning.store(1, std::memory_order_release);
    SPDLOG_INFO("PipeWire audio initialized: {} ch, {} Hz, latency hint {}",
                mNumChannels, GetSampleRate(), latencyStr);
    return true;
}

// ---------------------------------------------------------------------------
// DoClose
// ---------------------------------------------------------------------------

void PipeWireAudioPlayer::DoClose() {
    mRunning.store(0, std::memory_order_release);

    if (mLoop && mStream) {
        pw_thread_loop_lock(mLoop);
        pw_stream_disconnect(mStream);
        pw_thread_loop_unlock(mLoop);
    }

    if (mLoop)
        pw_thread_loop_stop(mLoop);

    if (mStream) {
        pw_stream_destroy(mStream);
        mStream = nullptr;
    }

    if (mLoop) {
        pw_thread_loop_destroy(mLoop);
        mLoop = nullptr;
    }

    RingDestroy();

    delete[] mLastChunk;
    mLastChunk      = nullptr;
    mLastChunkValid = false;
    mUnderrunFaded  = false;

    pw_deinit();
}

// ---------------------------------------------------------------------------
// Buffered — returns queued frames (called from main thread)
// ---------------------------------------------------------------------------

int PipeWireAudioPlayer::Buffered() {
    std::lock_guard<std::mutex> lock(mMutex);
    const int bytesPerFrame = static_cast<int>(sizeof(int16_t)) * mNumChannels;
    return RingAvailable() / bytesPerFrame;
}

// ---------------------------------------------------------------------------
// DoPlay — push PCM from the main / game thread into the ring buffer
// ---------------------------------------------------------------------------

void PipeWireAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
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
// OnProcess — PipeWire RT callback, pulls audio from ring buffer
// ---------------------------------------------------------------------------

void PipeWireAudioPlayer::OnProcess(void* userdata) {
    auto* self = static_cast<PipeWireAudioPlayer*>(userdata);

    struct pw_buffer* pwbuf = pw_stream_dequeue_buffer(self->mStream);
    if (!pwbuf)
        return;

    struct spa_buffer* buf = pwbuf->buffer;
    uint8_t*           dst = static_cast<uint8_t*>(buf->datas[0].data);
    if (!dst)
        goto done;

    {
        const int stride  = static_cast<int>(sizeof(int16_t)) * self->mNumChannels;
        int64_t n_frames  = buf->datas[0].maxsize / static_cast<uint32_t>(stride);
        if (pwbuf->requested && static_cast<uint64_t>(n_frames) > pwbuf->requested)
            n_frames = static_cast<int64_t>(pwbuf->requested);

        const int32_t n_bytes = static_cast<int32_t>(n_frames * stride);

        // trylock: this is a RT callback and must never block.
        // If the main thread holds mMutex, output silence rather than
        // deadlocking — the underrun recovery will mask the gap.
        if (!self->mMutex.try_lock()) {
            std::memset(dst, 0, static_cast<size_t>(n_bytes));
            buf->datas[0].chunk->offset = 0;
            buf->datas[0].chunk->stride = stride;
            buf->datas[0].chunk->size   = static_cast<uint32_t>(n_bytes);
            goto done;
        }

        const int32_t avail = self->RingAvailable();

        if (avail >= n_bytes) {
            // Normal path: ring buffer has enough data.
            self->RingRead(dst, n_bytes);
            self->mUnderrunFaded = false;

            // Save for underrun recovery.
            if (self->mLastChunk && n_bytes <= static_cast<int32_t>(self->mLastChunkSize)) {
                std::memcpy(self->mLastChunk, dst, static_cast<size_t>(n_bytes));
                self->mLastChunkValid = true;
            }
        } else if (self->mLastChunkValid && !self->mUnderrunFaded) {
            // Underrun recovery: replay last chunk with a linear fade-out.
            // A faded repeat is perceptually far less harsh than a silence
            // click — the same technique used in TiMidity's PipeWire backend.
            const uint32_t copy = (n_bytes <= static_cast<int32_t>(self->mLastChunkSize))
                                      ? static_cast<uint32_t>(n_bytes)
                                      : self->mLastChunkSize;
            std::memcpy(dst, self->mLastChunk, copy);
            if (copy < static_cast<uint32_t>(n_bytes))
                std::memset(dst + copy, 0, static_cast<size_t>(n_bytes) - copy);

            // Apply linear fade-out (S16 samples only — SoH always produces S16).
            for (int32_t i = 0; i < static_cast<int32_t>(n_frames); ++i) {
                const double gain = 1.0 - static_cast<double>(i) / static_cast<double>(n_frames);
                for (int ch = 0; ch < self->mNumChannels; ++ch) {
                    const int off    = (i * self->mNumChannels + ch) * static_cast<int>(sizeof(int16_t));
                    auto* s          = reinterpret_cast<int16_t*>(dst + off);
                    *s               = static_cast<int16_t>(static_cast<double>(*s) * gain);
                }
            }
            self->mUnderrunFaded = true;
        } else {
            // Complete underrun: output silence.
            std::memset(dst, 0, static_cast<size_t>(n_bytes));
        }

        self->mMutex.unlock();

        buf->datas[0].chunk->offset = 0;
        buf->datas[0].chunk->stride = stride;
        buf->datas[0].chunk->size   = static_cast<uint32_t>(n_bytes);
    }

done:
    pw_stream_queue_buffer(self->mStream, pwbuf);
}

// ---------------------------------------------------------------------------
// OnStateChanged — handle daemon disconnect gracefully
// ---------------------------------------------------------------------------

void PipeWireAudioPlayer::OnStateChanged(void* userdata,
                                         enum pw_stream_state /*old_state*/,
                                         enum pw_stream_state new_state,
                                         const char* error) {
    auto* self = static_cast<PipeWireAudioPlayer*>(userdata);

    if (!self->mRunning.load(std::memory_order_acquire))
        return;

    if (new_state == PW_STREAM_STATE_ERROR) {
        SPDLOG_ERROR("PipeWire: stream error: {}",
                     error ? error : "unknown");
        self->mRunning.store(0, std::memory_order_release);
    } else if (new_state == PW_STREAM_STATE_UNCONNECTED) {
        SPDLOG_WARN("PipeWire: stream disconnected");
        self->mRunning.store(0, std::memory_order_release);
    }
}

} // namespace Ship

#endif // #if ENABLE_PIPEWIRE
