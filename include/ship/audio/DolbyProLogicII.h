#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>

namespace Ship {

// =============================================================================
// Dolby Pro Logic II Decoder
// Source: https://stackoverflow.com/a - Posted by Sam, modified by community
// License: CC BY-SA 3.0
// =============================================================================

class DolbyProLogicIIDecoder {
  public:
    DolbyProLogicIIDecoder(int32_t sampleRate = 32000);
    ~DolbyProLogicIIDecoder() = default;

    // Process stereo input buffer and produce 5.1 surround output
    // stereoIn: interleaved stereo samples (L, R, L, R, ...)
    // surroundOut: interleaved 5.1 samples (FL, FR, C, LFE, RL, RR, ...)
    // numSamples: number of stereo sample pairs
    void Process(const int16_t* stereoIn, int16_t* surroundOut, int numSamples);

    // Reset all filter states
    void Reset();

    // Set sample rate (requires Reset() after changing)
    void SetSampleRate(int32_t sampleRate);
    int32_t GetSampleRate() const { return mSampleRate; }

  private:
    // Linkwitz-Riley 4th order filter state (24dB/octave)
    struct LRFilterState {
        double xm1 = 0, xm2 = 0, xm3 = 0, xm4 = 0;
        double ym1 = 0, ym2 = 0, ym3 = 0, ym4 = 0;
    };

    // Linkwitz-Riley filter coefficients
    struct LRFilterCoeffs {
        double a0, a1, a2, a3, a4;
        double b1, b2, b3, b4;
    };

    // Phase shifter state
    struct PhaseShiftState {
        double wp = 0, min_wp = 0, max_wp = 0, sweepfac = 0;
        double lx1 = 0, ly1 = 0, lx2 = 0, ly2 = 0;
        double lx3 = 0, ly3 = 0, lx4 = 0, ly4 = 0;
        bool initialized = false;
    };

    // Delay buffer
    static constexpr int MAX_DELAY_SAMPLES = 1024;  // Supports up to ~21ms at 48kHz
    struct DelayBuffer {
        float buffer[MAX_DELAY_SAMPLES];
        int writePos = 0;
        int delaySamples = 0;

        DelayBuffer() {
            std::fill(buffer, buffer + MAX_DELAY_SAMPLES, 0.0f);
        }
    };

    // Filter coefficient calculation
    LRFilterCoeffs CalcLRLowPassCoeffs(double cutoff);
    LRFilterCoeffs CalcLRHighPassCoeffs(double cutoff);

    // Filter processing
    float ApplyLRFilter(float input, LRFilterState& state, const LRFilterCoeffs& c);
    void InitPhaseShift(PhaseShiftState& state);
    float ApplyPhaseShift(float input, PhaseShiftState& state, bool invert);
    float ApplyDelay(float input, DelayBuffer& delay);

    // Clamp to int16 range
    static int16_t ClampToS16(float v);

    // Sample rate
    int32_t mSampleRate;

    // Delay in samples (10ms)
    int32_t mDelaySamples;

    // Pre-computed filter coefficients
    LRFilterCoeffs mCenterHPCoeffs;
    LRFilterCoeffs mCenterLPCoeffs;
    LRFilterCoeffs mSurroundHPCoeffs;
    LRFilterCoeffs mLFELPCoeffs;

    // Center channel filters
    LRFilterState mCenterHP;
    LRFilterState mCenterLP;

    // Surround Left filters
    LRFilterState mSLLeftHP;
    LRFilterState mSLRightHP;
    PhaseShiftState mSLLeftPhase;
    PhaseShiftState mSLRightPhase;
    DelayBuffer mSLDelay;

    // Surround Right filters
    LRFilterState mSRLeftHP;
    LRFilterState mSRRightHP;
    PhaseShiftState mSRLeftPhase;
    PhaseShiftState mSRRightPhase;
    DelayBuffer mSRDelay;

    // LFE filter
    LRFilterState mLFELP;

    bool mInitialized = false;
};

} // namespace Ship
