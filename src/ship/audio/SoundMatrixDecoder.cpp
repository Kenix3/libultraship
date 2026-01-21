#include "ship/audio/SoundMatrixDecoder.h"

namespace Ship {

// Standard matrix decoding gains derived from psychoacoustic principles
namespace Gains {
constexpr float gCenter = 0.7071067811865476f * 0.5f; // -3dB (1/sqrt(2)) * 0.5
constexpr float gFront = 0.5f;                        // -6dB
constexpr float gSurroundPrimary = 0.4359f;           // Primary surround contribution
constexpr float gSurroundSecondary = 0.2449f;         // Cross-feed surround contribution
} // namespace Gains

// Timing constants
namespace Timing {
constexpr int gSurroundDelayMs = 10; // ITU-R BS.775 recommends 10-25ms
}

SoundMatrixDecoder::SoundMatrixDecoder(int32_t sampleRate) : mSampleRate(sampleRate) {
    Reset();
}

void SoundMatrixDecoder::SetSampleRate(int32_t sampleRate) {
    mSampleRate = sampleRate;
    mReady = false;
}

void SoundMatrixDecoder::Reset() {
    // Compute delay length from sample rate
    mDelayLength = (mSampleRate * Timing::gSurroundDelayMs) / 1000;
    if (mDelayLength > gMaxDelay) {
        mDelayLength = gMaxDelay;
    }

    // Design filters for this sample rate
    mCoefCenterHP = DesignHighPass(70.0);    // Remove rumble from center
    mCoefCenterLP = DesignLowPass(20000.0);  // Anti-alias center
    mCoefSurroundHP = DesignHighPass(100.0); // Surround channels high-passed
    mCoefSubLP = DesignLowPass(120.0);       // LFE low-pass

    // Clear all filter states
    mCenterHighPass = {};
    mCenterLowPass = {};
    mSurrLeftMainHP = {};
    mSurrLeftCrossHP = {};
    mSurrRightMainHP = {};
    mSurrRightCrossHP = {};
    mSubLowPass = {};

    // Reset phase chains
    mPhaseLeftMain = {};
    mPhaseLeftCross = {};
    mPhaseRightMain = {};
    mPhaseRightCross = {};

    // Reset delay lines
    mDelaySurrLeft = {};
    mDelaySurrLeft.length = mDelayLength;
    mDelaySurrRight = {};
    mDelaySurrRight.length = mDelayLength;

    mReady = true;
}

SoundMatrixDecoder::FilterCoefficients SoundMatrixDecoder::DesignLowPass(double frequency) {
    FilterCoefficients coef = {};

    // Clamp to safe range for bilinear transform stability
    double maxFreq = mSampleRate * 0.475;
    if (frequency > maxFreq) {
        frequency = maxFreq;
    }

    // Linkwitz-Riley 4th order: two cascaded Butterworth 2nd order
    double omega = 2.0 * M_PI * frequency;
    double omega2 = omega * omega;
    double omega3 = omega2 * omega;
    double omega4 = omega2 * omega2;

    // Bilinear transform warping
    double kVal = omega / std::tan(M_PI * frequency / mSampleRate);
    double k2 = kVal * kVal;
    double k3 = k2 * kVal;
    double k4 = k2 * k2;

    double rt2 = std::sqrt(2.0);
    double term1 = rt2 * omega3 * kVal;
    double term2 = rt2 * omega * k3;
    double norm = 4.0 * omega2 * k2 + 2.0 * term1 + k4 + 2.0 * term2 + omega4;

    // Feedback coefficients
    coef.b[0] = (4.0 * (omega4 + term1 - k4 - term2)) / norm;
    coef.b[1] = (6.0 * omega4 - 8.0 * omega2 * k2 + 6.0 * k4) / norm;
    coef.b[2] = (4.0 * (omega4 - term1 + term2 - k4)) / norm;
    coef.b[3] = (k4 - 2.0 * term1 + omega4 - 2.0 * term2 + 4.0 * omega2 * k2) / norm;

    // Feedforward coefficients (low-pass response)
    coef.a[0] = omega4 / norm;
    coef.a[1] = 4.0 * omega4 / norm;
    coef.a[2] = 6.0 * omega4 / norm;
    coef.a[3] = coef.a[1];
    coef.a[4] = coef.a[0];

    return coef;
}

SoundMatrixDecoder::FilterCoefficients SoundMatrixDecoder::DesignHighPass(double frequency) {
    FilterCoefficients coef = {};

    double omega = 2.0 * M_PI * frequency;
    double omega2 = omega * omega;
    double omega3 = omega2 * omega;
    double omega4 = omega2 * omega2;

    double kVal = omega / std::tan(M_PI * frequency / mSampleRate);
    double k2 = kVal * kVal;
    double k3 = k2 * kVal;
    double k4 = k2 * k2;

    double rt2 = std::sqrt(2.0);
    double term1 = rt2 * omega3 * kVal;
    double term2 = rt2 * omega * k3;
    double norm = 4.0 * omega2 * k2 + 2.0 * term1 + k4 + 2.0 * term2 + omega4;

    coef.b[0] = (4.0 * (omega4 + term1 - k4 - term2)) / norm;
    coef.b[1] = (6.0 * omega4 - 8.0 * omega2 * k2 + 6.0 * k4) / norm;
    coef.b[2] = (4.0 * (omega4 - term1 + term2 - k4)) / norm;
    coef.b[3] = (k4 - 2.0 * term1 + omega4 - 2.0 * term2 + 4.0 * omega2 * k2) / norm;

    // Feedforward coefficients (high-pass response)
    coef.a[0] = k4 / norm;
    coef.a[1] = -4.0 * k4 / norm;
    coef.a[2] = 6.0 * k4 / norm;
    coef.a[3] = coef.a[1];
    coef.a[4] = coef.a[0];

    return coef;
}

float SoundMatrixDecoder::ProcessFilter(float sample, BiquadCascade& state, const FilterCoefficients& coef) {
    double in = sample;
    double out = coef.a[0] * in + coef.a[1] * state.x[0] + coef.a[2] * state.x[1] + coef.a[3] * state.x[2] +
                 coef.a[4] * state.x[3] - coef.b[0] * state.y[0] - coef.b[1] * state.y[1] - coef.b[2] * state.y[2] -
                 coef.b[3] * state.y[3];

    // Shift history
    state.x[3] = state.x[2];
    state.x[2] = state.x[1];
    state.x[1] = state.x[0];
    state.x[0] = in;
    state.y[3] = state.y[2];
    state.y[2] = state.y[1];
    state.y[1] = state.y[0];
    state.y[0] = out;

    return static_cast<float>(out);
}

void SoundMatrixDecoder::PrepareAllPass(AllPassChain& chain) {
    // Sweeping all-pass parameters for decorrelation
    constexpr double depth = 4.0;
    constexpr double baseDelay = 100.0;
    constexpr double sweepSpeed = 0.1;

    chain.freqMin = (M_PI * baseDelay) / mSampleRate;
    chain.freq = chain.freqMin;

    double range = std::pow(2.0, depth);
    chain.freqMax = (M_PI * baseDelay * range) / mSampleRate;
    chain.sweepRate = std::pow(range, sweepSpeed / (mSampleRate / 2.0));
    chain.ready = true;
}

float SoundMatrixDecoder::ProcessAllPass(float sample, AllPassChain& chain, bool negate) {
    if (!chain.ready) {
        PrepareAllPass(chain);
    }

    // First-order all-pass coefficient
    double c = (1.0 - chain.freq) / (1.0 + chain.freq);
    double input = static_cast<double>(sample);

    // Cascade of 4 first-order all-pass sections
    chain.yHist[0] = c * (chain.yHist[0] + input) - chain.xHist[0];
    chain.xHist[0] = input;

    chain.yHist[1] = c * (chain.yHist[1] + chain.yHist[0]) - chain.xHist[1];
    chain.xHist[1] = chain.yHist[0];

    chain.yHist[2] = c * (chain.yHist[2] + chain.yHist[1]) - chain.xHist[2];
    chain.xHist[2] = chain.yHist[1];

    chain.yHist[3] = c * (chain.yHist[3] + chain.yHist[2]) - chain.xHist[3];
    chain.xHist[3] = chain.yHist[2];

    double result = negate ? -chain.yHist[3] : chain.yHist[3];

    // Sweep the frequency for time-varying decorrelation
    double baseRate = std::pow(std::pow(2.0, 4.0), 0.1 / (mSampleRate / 2.0));
    chain.freq *= chain.sweepRate;

    if (chain.freq > chain.freqMax) {
        chain.sweepRate = 1.0 / baseRate;
    } else if (chain.freq < chain.freqMin) {
        chain.sweepRate = baseRate;
    }

    return static_cast<float>(result);
}

float SoundMatrixDecoder::ProcessDelay(float sample, CircularDelay& buffer) {
    float output = buffer.data[buffer.head];
    buffer.data[buffer.head] = sample;
    buffer.head = (buffer.head + 1) % buffer.length;
    return output;
}

int16_t SoundMatrixDecoder::Saturate(float value) {
    if (value > 32767.0f) {
        return 32767;
    }
    if (value < -32768.0f) {
        return -32768;
    }
    return static_cast<int16_t>(value);
}

void SoundMatrixDecoder::Process(const int16_t* stereoInput, int16_t* surroundOutput, int samplePairs) {
    if (!mReady) {
        Reset();
    }

    for (int i = 0; i < samplePairs; ++i) {
        float inL = static_cast<float>(stereoInput[i * 2]);
        float inR = static_cast<float>(stereoInput[i * 2 + 1]);

        // Center: sum of L+R, band-limited
        float ctr = (inL + inR) * Gains::gCenter;
        ctr = ProcessFilter(ctr, mCenterHighPass, mCoefCenterHP);
        ctr = ProcessFilter(ctr, mCenterLowPass, mCoefCenterLP);

        // Front channels: attenuated direct signal
        float frontL = inL * Gains::gFront;
        float frontR = inR * Gains::gFront;

        // Surround Left: L primary (inverted phase) + R secondary (shifted phase)
        float slMain = inL * Gains::gSurroundPrimary;
        slMain = ProcessFilter(slMain, mSurrLeftMainHP, mCoefSurroundHP);
        slMain = ProcessAllPass(slMain, mPhaseLeftMain, true);

        float slCross = inR * Gains::gSurroundSecondary;
        slCross = ProcessFilter(slCross, mSurrLeftCrossHP, mCoefSurroundHP);
        slCross = ProcessAllPass(slCross, mPhaseLeftCross, false);

        float surrL = ProcessDelay(slMain + slCross, mDelaySurrLeft);

        // Surround Right: R primary (shifted phase) + L secondary (inverted phase)
        float srMain = inR * Gains::gSurroundPrimary;
        srMain = ProcessFilter(srMain, mSurrRightMainHP, mCoefSurroundHP);
        srMain = ProcessAllPass(srMain, mPhaseRightMain, false);

        float srCross = inL * Gains::gSurroundSecondary;
        srCross = ProcessFilter(srCross, mSurrRightCrossHP, mCoefSurroundHP);
        srCross = ProcessAllPass(srCross, mPhaseRightCross, true);

        float surrR = ProcessDelay(srMain + srCross, mDelaySurrRight);

        // LFE: low-passed sum
        float lfe = (inL + inR) * Gains::gCenter;
        lfe = ProcessFilter(lfe, mSubLowPass, mCoefSubLP);

        // Output: FL, FR, C, LFE, SL, SR
        surroundOutput[i * 6 + 0] = Saturate(frontL);
        surroundOutput[i * 6 + 1] = Saturate(frontR);
        surroundOutput[i * 6 + 2] = Saturate(ctr);
        surroundOutput[i * 6 + 3] = Saturate(lfe);
        surroundOutput[i * 6 + 4] = Saturate(surrL);
        surroundOutput[i * 6 + 5] = Saturate(surrR);
    }
}

} // namespace Ship
