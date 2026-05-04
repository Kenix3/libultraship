// Standalone repro for the macOS CoreAudio crackling bugs in
// libultraship/src/ship/audio/CoreAudioAudioPlayer.cpp.
//
// We can't drive a real audio device deterministically from a unit test, so
// instead we lift the exact ring-buffer + render-callback logic out of the
// CoreAudio backend and feed it a known signal (a 1 kHz sine wave produced as
// a strictly increasing sample stream). Anything the "render callback" emits
// is then checked for:
//
//   * dropouts:    samples replaced with the silence padding (memset 0)
//   * total loss:  ring filled exactly => Buffered() returns 0 because the
//                  empty/full state is ambiguous
//   * chunk drops: DoPlay throws away an entire incoming chunk on overflow
//
// We run the same scenarios against a "fixed" implementation (lock-free
// SPSC ring buffer with a one-slot reserve so empty != full) so the diff
// between the two columns is the proof that the patch removes the bugs.
//
// Build:
//   clang++ -std=c++17 -O2 -pthread repro.cpp -o repro
// Run:
//   ./repro

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <type_traits>
#include <vector>

namespace {

constexpr int kNumChannels = 2;
constexpr int kBytesPerFrame = sizeof(int16_t) * kNumChannels;
constexpr int kRingFrames = 6000;                             // matches CoreAudioAudioPlayer
constexpr size_t kRingBytes = kRingFrames * kBytesPerFrame;

// Producer emits a strictly monotone sample-index encoded as int16_t in both
// channels so the consumer can reconstruct the producer's timeline and detect
// gaps/zero-fills. We use only the low 15 bits + a parity bit so a "0" sample
// never naturally appears -- any 0 we see in the output is a silence pad.
inline int16_t encode(uint64_t idx) {
    int16_t v = static_cast<int16_t>((idx & 0x3FFF) + 1); // 1..16384, never 0
    return v;
}

// =====================================================================
// 1. EXACT COPY of the buggy ring-buffer logic from
//    libultraship/src/ship/audio/CoreAudioAudioPlayer.cpp
//    (lines 116..208 at libultraship@fdcaf63)
// =====================================================================
class BuggyPlayer {
  public:
    BuggyPlayer() {
        mRingBufferSize = kRingBytes;
        mRingBuffer = new uint8_t[mRingBufferSize];
        mRingBufferReadPos = 0;
        mRingBufferWritePos = 0;
    }
    ~BuggyPlayer() { delete[] mRingBuffer; }

    int Buffered() {
        std::lock_guard<std::mutex> lk(mMutex);
        size_t buffered;
        if (mRingBufferWritePos >= mRingBufferReadPos) {
            buffered = mRingBufferWritePos - mRingBufferReadPos;
        } else {
            buffered = mRingBufferSize - (mRingBufferReadPos - mRingBufferWritePos);
        }
        return buffered / kBytesPerFrame;
    }

    void DoPlay(const uint8_t* buf, size_t len) {
        std::lock_guard<std::mutex> lk(mMutex);

        size_t available;
        if (mRingBufferWritePos >= mRingBufferReadPos) {
            available = mRingBufferSize - (mRingBufferWritePos - mRingBufferReadPos);
        } else {
            available = mRingBufferReadPos - mRingBufferWritePos;
        }

        // Bug: drops the ENTIRE chunk on overflow.
        if (available >= len) {
            size_t writeEnd = mRingBufferWritePos + len;
            if (writeEnd <= mRingBufferSize) {
                memcpy(mRingBuffer + mRingBufferWritePos, buf, len);
            } else {
                size_t firstChunk = mRingBufferSize - mRingBufferWritePos;
                memcpy(mRingBuffer + mRingBufferWritePos, buf, firstChunk);
                memcpy(mRingBuffer, buf + firstChunk, len - firstChunk);
            }
            // Bug: when writeEnd lands exactly on read pos, write%size == read,
            // and the next Buffered() call returns 0 (treated as empty).
            mRingBufferWritePos = (mRingBufferWritePos + len) % mRingBufferSize;
        } else {
            mChunksDropped++;
        }
    }

    // Mirrors the CoreAudio render callback: pulls bytesToWrite, pads with 0.
    void Render(uint8_t* outputBuffer, uint32_t bytesToWrite) {
        std::lock_guard<std::mutex> lk(mMutex);

        size_t available;
        if (mRingBufferWritePos >= mRingBufferReadPos) {
            available = mRingBufferWritePos - mRingBufferReadPos;
        } else {
            available = mRingBufferSize - (mRingBufferReadPos - mRingBufferWritePos);
        }

        uint32_t bytesToCopy = bytesToWrite;
        if (bytesToCopy > available) bytesToCopy = available;

        if (bytesToCopy > 0) {
            size_t readEnd = mRingBufferReadPos + bytesToCopy;
            if (readEnd <= mRingBufferSize) {
                memcpy(outputBuffer, mRingBuffer + mRingBufferReadPos, bytesToCopy);
            } else {
                size_t firstChunk = mRingBufferSize - mRingBufferReadPos;
                memcpy(outputBuffer, mRingBuffer + mRingBufferReadPos, firstChunk);
                memcpy(outputBuffer + firstChunk, mRingBuffer, bytesToCopy - firstChunk);
            }
            mRingBufferReadPos = (mRingBufferReadPos + bytesToCopy) % mRingBufferSize;
        }
        if (bytesToCopy < bytesToWrite) {
            memset(outputBuffer + bytesToCopy, 0, bytesToWrite - bytesToCopy);
        }
    }

    int chunksDropped() const { return mChunksDropped; }

  private:
    uint8_t* mRingBuffer;
    size_t mRingBufferSize;
    size_t mRingBufferReadPos;
    size_t mRingBufferWritePos;
    std::mutex mMutex; // stand-in for pthread_mutex_t
    int mChunksDropped = 0;
};

// =====================================================================
// 2. PROPOSED FIX: lock-free SPSC ring buffer.
//    - One-slot reserved so write==read means empty, never full.
//    - On overflow, write what fits instead of dropping the whole chunk.
//    - No mutex acquired in the consumer "render callback" path.
// =====================================================================
class FixedPlayer {
  public:
    FixedPlayer() {
        // +1 frame for the empty-slot reserve, so usable capacity stays 6000.
        mRingBufferSize = kRingBytes + kBytesPerFrame;
        mRingBuffer = new uint8_t[mRingBufferSize];
        mRead.store(0, std::memory_order_relaxed);
        mWrite.store(0, std::memory_order_relaxed);
    }
    ~FixedPlayer() { delete[] mRingBuffer; }

    int Buffered() const {
        size_t r = mRead.load(std::memory_order_acquire);
        size_t w = mWrite.load(std::memory_order_acquire);
        size_t buffered = (w >= r) ? (w - r) : (mRingBufferSize - (r - w));
        return buffered / kBytesPerFrame;
    }

    void DoPlay(const uint8_t* buf, size_t len) {
        size_t r = mRead.load(std::memory_order_acquire);
        size_t w = mWrite.load(std::memory_order_relaxed);
        size_t freeSpace = (w >= r) ? (mRingBufferSize - (w - r) - kBytesPerFrame)
                                    : (r - w - kBytesPerFrame);
        size_t toWrite = len <= freeSpace ? len : freeSpace;
        if (toWrite == 0) {
            mChunksDropped++;
            return;
        }
        if (toWrite < len) mPartialDrops++;

        size_t writeEnd = w + toWrite;
        if (writeEnd <= mRingBufferSize) {
            memcpy(mRingBuffer + w, buf, toWrite);
        } else {
            size_t firstChunk = mRingBufferSize - w;
            memcpy(mRingBuffer + w, buf, firstChunk);
            memcpy(mRingBuffer, buf + firstChunk, toWrite - firstChunk);
        }
        mWrite.store((w + toWrite) % mRingBufferSize, std::memory_order_release);
    }

    void Render(uint8_t* outputBuffer, uint32_t bytesToWrite) {
        size_t r = mRead.load(std::memory_order_relaxed);
        size_t w = mWrite.load(std::memory_order_acquire);
        size_t available = (w >= r) ? (w - r) : (mRingBufferSize - (r - w));

        uint32_t bytesToCopy = bytesToWrite;
        if (bytesToCopy > available) bytesToCopy = available;

        if (bytesToCopy > 0) {
            size_t readEnd = r + bytesToCopy;
            if (readEnd <= mRingBufferSize) {
                memcpy(outputBuffer, mRingBuffer + r, bytesToCopy);
            } else {
                size_t firstChunk = mRingBufferSize - r;
                memcpy(outputBuffer, mRingBuffer + r, firstChunk);
                memcpy(outputBuffer + firstChunk, mRingBuffer, bytesToCopy - firstChunk);
            }
            mRead.store((r + bytesToCopy) % mRingBufferSize, std::memory_order_release);
        }
        if (bytesToCopy < bytesToWrite) {
            memset(outputBuffer + bytesToCopy, 0, bytesToWrite - bytesToCopy);
        }
    }

    int chunksDropped() const { return mChunksDropped; }
    int partialDrops() const { return mPartialDrops; }

  private:
    uint8_t* mRingBuffer;
    size_t mRingBufferSize;
    std::atomic<size_t> mRead;
    std::atomic<size_t> mWrite;
    int mChunksDropped = 0;
    int mPartialDrops = 0;
};

// =====================================================================
// Scenarios
// =====================================================================

struct Stats {
    int dropouts = 0;       // sample slots that were silence-padded (value 0)
    int discontinuities = 0;// non-monotone jumps in the recovered stream
    int chunksDropped = 0;
    int partialDrops = 0;
};

template <class Player>
Stats scenarioExactFill() {
    // Hand the player exactly ringBufferSize bytes in one shot, then drain.
    // BuggyPlayer: write%size == read so Buffered() reports 0 -> total loss.
    // FixedPlayer: capacity is size-frame so a true "full" write copies
    //              kRingFrames frames with a one-slot reserve.
    Player p;
    std::vector<uint8_t> chunk(kRingBytes);
    int16_t* s = reinterpret_cast<int16_t*>(chunk.data());
    for (int i = 0; i < kRingFrames; ++i) {
        int16_t v = encode(i);
        s[i * kNumChannels + 0] = v;
        s[i * kNumChannels + 1] = v;
    }
    p.DoPlay(chunk.data(), kRingBytes);

    std::vector<uint8_t> out(kRingBytes);
    p.Render(out.data(), kRingBytes);

    Stats st;
    int16_t* o = reinterpret_cast<int16_t*>(out.data());
    int prev = 0;
    for (int i = 0; i < kRingFrames; ++i) {
        int16_t v = o[i * kNumChannels];
        if (v == 0) { st.dropouts++; prev = 0; continue; }
        if (prev != 0) {
            int expected = (prev % 16384) + 1;
            if (v != expected) st.discontinuities++;
        }
        prev = v;
    }
    if constexpr (std::is_same_v<Player, BuggyPlayer>) {
        st.chunksDropped = p.chunksDropped();
    } else {
        st.chunksDropped = p.chunksDropped();
        st.partialDrops = p.partialDrops();
    }
    return st;
}

template <class Player>
Stats scenarioOverflow() {
    // Fill ring near-full, then push a chunk that doesn't fit.
    // BuggyPlayer: discards the whole chunk -> chunksDropped++.
    // FixedPlayer: writes what fits, partialDrops++ for the tail.
    Player p;
    constexpr int prefillFrames = kRingFrames - 100; // leave room for 100 frames
    std::vector<uint8_t> prefill(prefillFrames * kBytesPerFrame);
    int16_t* s = reinterpret_cast<int16_t*>(prefill.data());
    for (int i = 0; i < prefillFrames; ++i) {
        int16_t v = encode(i);
        s[i * kNumChannels + 0] = v;
        s[i * kNumChannels + 1] = v;
    }
    p.DoPlay(prefill.data(), prefill.size());

    // chunk of 500 frames - way more than the 100-frame headroom.
    std::vector<uint8_t> oversize(500 * kBytesPerFrame);
    int16_t* o = reinterpret_cast<int16_t*>(oversize.data());
    for (int i = 0; i < 500; ++i) {
        int16_t v = encode(prefillFrames + i);
        o[i * kNumChannels + 0] = v;
        o[i * kNumChannels + 1] = v;
    }
    p.DoPlay(oversize.data(), oversize.size());

    Stats st;
    if constexpr (std::is_same_v<Player, BuggyPlayer>) {
        st.chunksDropped = p.chunksDropped();
    } else {
        st.chunksDropped = p.chunksDropped();
        st.partialDrops = p.partialDrops();
    }
    return st;
}

template <class Player>
Stats scenarioCycledFills() {
    // Repeatedly fill the ring exactly and drain it exactly, with no gap.
    // BuggyPlayer: every cycle, write%size lands on read => Buffered() reads
    //              0, the render callback writes silence, and the entire
    //              cycle's data is lost. We expect ~kCycles * kRingFrames
    //              dropouts and zero recovered samples.
    // FixedPlayer: capacity = ringSize - 1 frame, so write never collides
    //              with read; every cycle round-trips losslessly.
    Player p;
    constexpr int kCycles = 32;
    std::vector<uint8_t> chunk(kRingBytes);
    std::vector<uint8_t> out(kRingBytes);

    Stats st;
    uint64_t produced_idx = 0;
    int prev = 0;
    for (int c = 0; c < kCycles; ++c) {
        int16_t* s = reinterpret_cast<int16_t*>(chunk.data());
        for (int f = 0; f < kRingFrames; ++f) {
            int16_t v = encode(produced_idx++);
            s[f * kNumChannels + 0] = v;
            s[f * kNumChannels + 1] = v;
        }
        p.DoPlay(chunk.data(), kRingBytes);
        p.Render(out.data(), kRingBytes);

        int16_t* o = reinterpret_cast<int16_t*>(out.data());
        for (int f = 0; f < kRingFrames; ++f) {
            int16_t v = o[f * kNumChannels];
            if (v == 0) {
                st.dropouts++;
                prev = 0;
                continue;
            }
            if (prev != 0) {
                int expected = (prev % 16384) + 1;
                if (v != expected) st.discontinuities++;
            }
            prev = v;
        }
    }
    if constexpr (std::is_same_v<Player, BuggyPlayer>) {
        st.chunksDropped = p.chunksDropped();
    } else {
        st.chunksDropped = p.chunksDropped();
        st.partialDrops = p.partialDrops();
    }
    return st;
}

void printRow(const char* name, const Stats& b, const Stats& f) {
    printf("  %-22s buggy: dropouts=%5d discont=%5d chunkDrops=%4d   "
           "fixed: dropouts=%5d discont=%5d chunkDrops=%4d partial=%4d\n",
           name,
           b.dropouts, b.discontinuities, b.chunksDropped,
           f.dropouts, f.discontinuities, f.chunksDropped, f.partialDrops);
}

} // namespace

int main() {
    printf("CoreAudio crackling repro -- buggy vs fixed ring-buffer logic\n");
    printf("=============================================================\n");

    auto b1 = scenarioExactFill<BuggyPlayer>();
    auto f1 = scenarioExactFill<FixedPlayer>();
    printRow("exact-fill wrap", b1, f1);

    auto b2 = scenarioOverflow<BuggyPlayer>();
    auto f2 = scenarioOverflow<FixedPlayer>();
    printRow("overflow chunk drop", b2, f2);

    auto b3 = scenarioCycledFills<BuggyPlayer>();
    auto f3 = scenarioCycledFills<FixedPlayer>();
    printRow("32 fill/drain cycles", b3, f3);

    // Verdict
    bool buggyHasIssues = (b1.dropouts > 0) || (b2.chunksDropped > 0) ||
                          (b3.dropouts > 0);
    bool fixedClean = (f1.dropouts == 0) && (f2.chunksDropped == 0) &&
                      (f3.dropouts == 0) && (f3.discontinuities == 0);
    printf("\n%s buggy implementation reproduces crackling artifacts\n",
           buggyHasIssues ? "[OK]" : "[??]");
    printf("%s fixed implementation: clean stream, no dropouts/discontinuities\n",
           fixedClean ? "[OK]" : "[FAIL]");
    return (buggyHasIssues && fixedClean) ? 0 : 1;
}
