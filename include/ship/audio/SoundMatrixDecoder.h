#pragma once

#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <tuple>

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
    /**
     * Construct and initialize the decoder with a specific sample rate.
     * @param sampleRate The audio sample rate in Hz
     */
    SoundMatrixDecoder(int32_t sampleRate);
    ~SoundMatrixDecoder() = default;

    /**
     * Reset filter states without recomputing coefficients.
     * Useful when audio is interrupted to prevent clicks.
     */
    void ResetState();

    /**
     * Decode stereo to 5.1 surround
     * @param stereoInput Interleaved stereo samples [L0, R0, L1, R1, ...]
     * @param samplePairs Number of stereo sample pairs to process
     * @return Pointer to internal buffer with interleaved 5.1 samples [FL, FR, C, LFE, SL, SR, ...]
     */
    std::tuple<const uint8_t*, int> Process(const uint8_t* buf, size_t len);

  private:
    /**
     * @brief State for a 4th-order IIR (Linkwitz-Riley) biquad cascade filter.
     *
     * Stores four samples of input and output history used to evaluate the
     * difference equation with 24 dB/octave slopes.
     */
    struct BiquadCascade {
        double X[4] = {}; // Input history
        double Y[4] = {}; // Output history
    };

    /**
     * @brief Coefficients for a 4th-order IIR filter (Linkwitz-Riley).
     *
     * A holds the five feedforward (numerator) coefficients and B holds
     * the four feedback (denominator) coefficients, assuming b0 = 1.
     */
    struct FilterCoefficients {
        double A[5]; // Feedforward (numerator)
        double B[4]; // Feedback (denominator, excluding b0=1)
    };

    /**
     * @brief State for a sweeping all-pass filter used for phase decorrelation.
     *
     * The filter sweeps its centre frequency between FreqMin and FreqMax at
     * SweepRate to decorrelate the surround channels from the front channels,
     * preventing hard cancellation artifacts.
     */
    struct AllPassChain {
        double Freq = 0;
        double FreqMin = 0;
        double FreqMax = 0;
        double SweepRate = 0;
        double XHist[4] = {};
        double YHist[4] = {};
        bool Ready = false;
    };

    /**
     * @brief Fixed-length circular delay buffer for surround channel timing.
     */
    static constexpr int gMaxDelay = 1024;
    struct CircularDelay {
        std::array<float, gMaxDelay> Data = {};
        int Head = 0;
        int Length = 0;
    };

    /**
     * @brief Designs a 4th-order Linkwitz-Riley low-pass filter.
     * @param frequency Cutoff frequency in Hz.
     * @param sampleRate Audio sample rate in Hz.
     * @return Computed filter coefficients.
     */
    FilterCoefficients DesignLowPass(double frequency, int32_t sampleRate);

    /**
     * @brief Designs a 4th-order Linkwitz-Riley high-pass filter.
     * @param frequency Cutoff frequency in Hz.
     * @param sampleRate Audio sample rate in Hz.
     * @return Computed filter coefficients.
     */
    FilterCoefficients DesignHighPass(double frequency, int32_t sampleRate);

    /**
     * @brief Runs a single sample through a 4th-order IIR biquad cascade.
     * @param sample Input sample value.
     * @param state Per-channel filter state (updated in place).
     * @param coef Filter coefficients to apply.
     * @return Filtered output sample.
     */
    float ProcessFilter(float sample, BiquadCascade& state, const FilterCoefficients& coef);

    /**
     * @brief Initializes an AllPassChain's sweep parameters for the given sample rate.
     * @param chain All-pass chain state to initialize.
     * @param sampleRate Audio sample rate in Hz.
     */
    void PrepareAllPass(AllPassChain& chain, int32_t sampleRate);

    /**
     * @brief Processes a sample through a sweeping all-pass filter for phase decorrelation.
     * @param sample Input sample value.
     * @param chain All-pass chain state (updated in place).
     * @param negate If true, inverts the output to create an anti-phase signal.
     * @return Phase-shifted output sample.
     */
    float ProcessAllPass(float sample, AllPassChain& chain, bool negate);

    /**
     * @brief Delays a sample by the configured surround delay length.
     * @param sample Input sample value.
     * @param buffer Circular delay buffer state (updated in place).
     * @return Delayed output sample.
     */
    float ProcessDelay(float sample, CircularDelay& buffer);

    /**
     * @brief Clamps a floating-point sample to the int16_t range.
     * @param value Input sample value.
     * @return Saturated 16-bit integer sample.
     */
    static int16_t Saturate(float value);

    int32_t mDelayLength = 0;
    double mAllPassBaseRate = 1.0; // Precomputed for ProcessAllPass

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

    // Output buffer
    std::vector<int16_t> mSurroundBuffer;
};

} // namespace Ship
