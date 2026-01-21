#pragma once

#include <cstdint>
#include <cmath>
#include <array>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Ship {

/**
 * Passive matrix decoder for stereo to 5.1 surround upmixing.
 * Implements standard audio matrix decoding techniques using:
 * - Linkwitz-Riley crossover filters for frequency band separation
 * - All-pass filters for phase manipulation
 * - Delay lines for surround channel timing
 */
class SoundMatrixDecoder {
  public:
    explicit SoundMatrixDecoder(int32_t sampleRate = 32000);
    ~SoundMatrixDecoder() = default;

    /**
     * Decode stereo to 5.1 surround
     * @param stereoInput Interleaved stereo samples [L0, R0, L1, R1, ...]
     * @param surroundOutput Interleaved 5.1 samples [FL, FR, C, LFE, SL, SR, ...]
     * @param samplePairs Number of stereo sample pairs to process
     */
    void Process(const int16_t* stereoInput, int16_t* surroundOutput, int samplePairs);

    void Reset();
    void SetSampleRate(int32_t sampleRate);
    int32_t GetSampleRate() const {
        return mSampleRate;
    }

  private:
    // 4th-order IIR filter (Linkwitz-Riley) for 24dB/octave slopes
    struct BiquadCascade {
        double X[4] = {}; // Input history
        double Y[4] = {}; // Output history
    };

    struct FilterCoefficients {
        double A[5]; // Feedforward (numerator)
        double B[4]; // Feedback (denominator, excluding b0=1)
    };

    // Sweeping all-pass for phase decorrelation
    struct AllPassChain {
        double Freq = 0;
        double FreqMin = 0;
        double FreqMax = 0;
        double SweepRate = 0;
        double XHist[4] = {};
        double YHist[4] = {};
        bool Ready = false;
    };

    // Circular delay buffer
    static constexpr int gMaxDelay = 1024;
    struct CircularDelay {
        std::array<float, gMaxDelay> Data = {};
        int Head = 0;
        int Length = 0;
    };

    // Filter design
    FilterCoefficients DesignLowPass(double frequency);
    FilterCoefficients DesignHighPass(double frequency);

    // Signal processing
    float ProcessFilter(float sample, BiquadCascade& state, const FilterCoefficients& coef);
    void PrepareAllPass(AllPassChain& chain);
    float ProcessAllPass(float sample, AllPassChain& chain, bool negate);
    float ProcessDelay(float sample, CircularDelay& buffer);

    static int16_t Saturate(float value);

    int32_t mSampleRate;
    int32_t mDelayLength;
    bool mReady = false;

    // Filter coefficients (computed once per sample rate)
    FilterCoefficients mCoefCenterHP;
    FilterCoefficients mCoefCenterLP;
    FilterCoefficients mCoefSurroundHP;
    FilterCoefficients mCoefSubLP;

    // Per-channel filter states
    BiquadCascade mCenterHighPass;
    BiquadCascade mCenterLowPass;
    BiquadCascade mSurrLeftMainHP;
    BiquadCascade mSurrLeftCrossHP;
    BiquadCascade mSurrRightMainHP;
    BiquadCascade mSurrRightCrossHP;
    BiquadCascade mSubLowPass;

    // Phase processing
    AllPassChain mPhaseLeftMain;
    AllPassChain mPhaseLeftCross;
    AllPassChain mPhaseRightMain;
    AllPassChain mPhaseRightCross;

    // Timing
    CircularDelay mDelaySurrLeft;
    CircularDelay mDelaySurrRight;
};

} // namespace Ship
