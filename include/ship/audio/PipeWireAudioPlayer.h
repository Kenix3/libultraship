#pragma once

#if ENABLE_PIPEWIRE

#include "ship/audio/AudioPlayer.h"

#include <atomic>
#include <cstdint>
#include <mutex>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

namespace Ship {

/*
 * PipeWireAudioPlayer — native Linux audio backend for libultraship.
 *
 * Architecture mirrors CoreAudioAudioPlayer:
 *   - Ring buffer bridges the push model (DoPlay, main thread) and the
 *     pull model (on_process callback, PipeWire RT thread).
 *   - pw_thread_loop owns the RT thread; we never create one ourselves.
 *   - On underrun the last good chunk is replayed with a linear fade-out
 *     instead of inserting silence — this masks loading spikes perceptually.
 *
 * Buffer sizing:
 *   Ring buffer = 4 * SampleLength frames, expressed in bytes.
 *   This matches TiMidity's "3× frag + 1 spare" heuristic, rounded up
 *   to a power of two by the ring buffer implementation.
 */
class PipeWireAudioPlayer : public AudioPlayer {
  public:
    explicit PipeWireAudioPlayer(AudioSettings settings);
    ~PipeWireAudioPlayer() override;

    bool DoInit() override;
    void DoClose() override;
    int Buffered() override;
    void DoPlay(const uint8_t* buf, size_t len) override;

  private:
    /* PipeWire objects */
    struct pw_thread_loop* mLoop   = nullptr;
    struct pw_stream*      mStream = nullptr;

    int mNumChannels = 2;

    /* Ring buffer — power-of-two size, unbounded read/write pointers */
    struct RingBuffer {
        uint8_t* data     = nullptr;
        uint32_t size     = 0;   /* always power of 2 */
        uint32_t mask     = 0;   /* size - 1 */
        int64_t  rdptr    = 0;
        int64_t  wrptr    = 0;
    } mRing;

    std::mutex       mMutex;
    std::atomic<int> mRunning { 0 };

    /* Underrun recovery: last successfully played chunk */
    uint8_t* mLastChunk      = nullptr;
    uint32_t mLastChunkSize  = 0;
    bool     mLastChunkValid = false;
    bool     mUnderrunFaded  = false;

    /* Helpers */
    static uint32_t NextPow2(uint32_t v);
    void            RingInit(uint32_t bytes);
    void            RingDestroy();
    int32_t         RingAvailable() const;
    int32_t         RingSpace() const;
    int32_t         RingRead(uint8_t* dst, int32_t bytes);
    int32_t         RingWrite(const uint8_t* src, int32_t bytes);

    /* PipeWire callbacks (static, dispatched to instance) */
    static void OnProcess(void* userdata);
    static void OnStateChanged(void* userdata,
                               enum pw_stream_state old_state,
                               enum pw_stream_state new_state,
                               const char* error);

    static struct pw_stream_events sStreamEvents;
};

} // namespace Ship

#endif // #if ENABLE_PIPEWIRE
