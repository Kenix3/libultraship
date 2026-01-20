#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    int32_t GetSampleRate() const {
        return mSampleRate;
    }

  private:
    // Linkwitz-Riley 4th order filter state (24dB/octave)
    struct LRFilterState {
        double Xm1 = 0, Xm2 = 0, Xm3 = 0, Xm4 = 0;
        double Ym1 = 0, Ym2 = 0, Ym3 = 0, Ym4 = 0;
    };

    // Linkwitz-Riley filter coefficients
    struct LRFilterCoeffs {
        double A0, A1, A2, A3, A4;
        double B1, B2, B3, B4;
    };

    // Phase shifter state
    struct PhaseShiftState {
        double Wp = 0, MinWp = 0, MaxWp = 0, SweepFac = 0;
        double Lx1 = 0, Ly1 = 0, Lx2 = 0, Ly2 = 0;
        double Lx3 = 0, Ly3 = 0, Lx4 = 0, Ly4 = 0;
        bool Initialized = false;
    };

    // Delay buffer
    static constexpr int gMaxDelaySamples = 1024; // Supports up to ~21ms at 48kHz
    struct DelayBuffer {
        float Buffer[gMaxDelaySamples];
        int WritePos = 0;
        int DelaySamples = 0;

        DelayBuffer() {
            std::fill(Buffer, Buffer + gMaxDelaySamples, 0.0f);
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
