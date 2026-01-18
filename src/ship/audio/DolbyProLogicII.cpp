#include "ship/audio/DolbyProLogicII.h"
#include <cstring>

namespace Ship {

// Dolby Pro Logic II coefficients
static constexpr float COEF_CENTER = 0.35355339059327376220042218105242f;      // 1/(2*sqrt(2))
static constexpr float COEF_FRONT = 0.5f;
static constexpr float COEF_SURR_MAIN = 0.43588989435406735522369819838596f;   // sqrt(19)/10
static constexpr float COEF_SURR_CROSS = 0.24494897427831780981972840747059f;  // sqrt(6)/10
static constexpr int DELAY_MS = 10;

DolbyProLogicIIDecoder::DolbyProLogicIIDecoder(int32_t sampleRate)
    : mSampleRate(sampleRate) {
    Reset();
}

void DolbyProLogicIIDecoder::SetSampleRate(int32_t sampleRate) {
    mSampleRate = sampleRate;
    mInitialized = false;
}

void DolbyProLogicIIDecoder::Reset() {
    // Calculate delay in samples (10ms)
    mDelaySamples = (mSampleRate / 1000) * DELAY_MS;
    if (mDelaySamples > MAX_DELAY_SAMPLES) {
        mDelaySamples = MAX_DELAY_SAMPLES;
    }

    // Calculate filter coefficients
    mCenterHPCoeffs = CalcLRHighPassCoeffs(70.0);
    mCenterLPCoeffs = CalcLRLowPassCoeffs(20000.0);
    mSurroundHPCoeffs = CalcLRHighPassCoeffs(100.0);
    mLFELPCoeffs = CalcLRLowPassCoeffs(120.0);

    // Reset filter states
    mCenterHP = LRFilterState{};
    mCenterLP = LRFilterState{};
    mSLLeftHP = LRFilterState{};
    mSLRightHP = LRFilterState{};
    mSRLeftHP = LRFilterState{};
    mSRRightHP = LRFilterState{};
    mLFELP = LRFilterState{};

    // Reset phase shift states
    mSLLeftPhase = PhaseShiftState{};
    mSLRightPhase = PhaseShiftState{};
    mSRLeftPhase = PhaseShiftState{};
    mSRRightPhase = PhaseShiftState{};

    // Reset delay buffers
    mSLDelay = DelayBuffer{};
    mSLDelay.delaySamples = mDelaySamples;
    mSRDelay = DelayBuffer{};
    mSRDelay.delaySamples = mDelaySamples;

    mInitialized = true;
}

DolbyProLogicIIDecoder::LRFilterCoeffs DolbyProLogicIIDecoder::CalcLRLowPassCoeffs(double cutoff) {
    LRFilterCoeffs c{};
    double wc = 2.0 * M_PI * cutoff;
    double wc2 = wc * wc;
    double wc3 = wc2 * wc;
    double wc4 = wc2 * wc2;
    double k = wc / tan(M_PI * cutoff / mSampleRate);
    double k2 = k * k;
    double k3 = k2 * k;
    double k4 = k2 * k2;
    double sqrt2 = sqrt(2.0);
    double sq_tmp1 = sqrt2 * wc3 * k;
    double sq_tmp2 = sqrt2 * wc * k3;
    double a_tmp = 4.0 * wc2 * k2 + 2.0 * sq_tmp1 + k4 + 2.0 * sq_tmp2 + wc4;

    c.b1 = (4.0 * (wc4 + sq_tmp1 - k4 - sq_tmp2)) / a_tmp;
    c.b2 = (6.0 * wc4 - 8.0 * wc2 * k2 + 6.0 * k4) / a_tmp;
    c.b3 = (4.0 * (wc4 - sq_tmp1 + sq_tmp2 - k4)) / a_tmp;
    c.b4 = (k4 - 2.0 * sq_tmp1 + wc4 - 2.0 * sq_tmp2 + 4.0 * wc2 * k2) / a_tmp;

    c.a0 = wc4 / a_tmp;
    c.a1 = 4.0 * wc4 / a_tmp;
    c.a2 = 6.0 * wc4 / a_tmp;
    c.a3 = c.a1;
    c.a4 = c.a0;
    return c;
}

DolbyProLogicIIDecoder::LRFilterCoeffs DolbyProLogicIIDecoder::CalcLRHighPassCoeffs(double cutoff) {
    LRFilterCoeffs c{};
    double wc = 2.0 * M_PI * cutoff;
    double wc2 = wc * wc;
    double wc3 = wc2 * wc;
    double wc4 = wc2 * wc2;
    double k = wc / tan(M_PI * cutoff / mSampleRate);
    double k2 = k * k;
    double k3 = k2 * k;
    double k4 = k2 * k2;
    double sqrt2 = sqrt(2.0);
    double sq_tmp1 = sqrt2 * wc3 * k;
    double sq_tmp2 = sqrt2 * wc * k3;
    double a_tmp = 4.0 * wc2 * k2 + 2.0 * sq_tmp1 + k4 + 2.0 * sq_tmp2 + wc4;

    c.b1 = (4.0 * (wc4 + sq_tmp1 - k4 - sq_tmp2)) / a_tmp;
    c.b2 = (6.0 * wc4 - 8.0 * wc2 * k2 + 6.0 * k4) / a_tmp;
    c.b3 = (4.0 * (wc4 - sq_tmp1 + sq_tmp2 - k4)) / a_tmp;
    c.b4 = (k4 - 2.0 * sq_tmp1 + wc4 - 2.0 * sq_tmp2 + 4.0 * wc2 * k2) / a_tmp;

    c.a0 = k4 / a_tmp;
    c.a1 = -4.0 * k4 / a_tmp;
    c.a2 = 6.0 * k4 / a_tmp;
    c.a3 = c.a1;
    c.a4 = c.a0;
    return c;
}

float DolbyProLogicIIDecoder::ApplyLRFilter(float input, LRFilterState& state, const LRFilterCoeffs& c) {
    double tempx = input;
    double tempy = c.a0 * tempx + c.a1 * state.xm1 + c.a2 * state.xm2 + c.a3 * state.xm3 + c.a4 * state.xm4
                 - c.b1 * state.ym1 - c.b2 * state.ym2 - c.b3 * state.ym3 - c.b4 * state.ym4;
    state.xm4 = state.xm3;
    state.xm3 = state.xm2;
    state.xm2 = state.xm1;
    state.xm1 = tempx;
    state.ym4 = state.ym3;
    state.ym3 = state.ym2;
    state.ym2 = state.ym1;
    state.ym1 = tempy;
    return static_cast<float>(tempy);
}

void DolbyProLogicIIDecoder::InitPhaseShift(PhaseShiftState& state) {
    double depth = 4.0;
    double delay = 100.0;
    double rate = 0.1;
    state.wp = state.min_wp = (M_PI * delay) / mSampleRate;
    double range = pow(2.0, depth);
    state.max_wp = (M_PI * delay * range) / mSampleRate;
    rate = pow(range, rate / (mSampleRate / 2.0));
    state.sweepfac = rate;
    state.initialized = true;
}

float DolbyProLogicIIDecoder::ApplyPhaseShift(float input, PhaseShiftState& state, bool invert) {
    if (!state.initialized) {
        InitPhaseShift(state);
    }

    double coef = (1.0 - state.wp) / (1.0 + state.wp);
    double x1 = static_cast<double>(input);

    state.ly1 = coef * (state.ly1 + x1) - state.lx1;
    state.lx1 = x1;
    state.ly2 = coef * (state.ly2 + state.ly1) - state.lx2;
    state.lx2 = state.ly1;
    state.ly3 = coef * (state.ly3 + state.ly2) - state.lx3;
    state.lx3 = state.ly2;
    state.ly4 = coef * (state.ly4 + state.ly3) - state.lx4;
    state.lx4 = state.ly3;

    double outval = invert ? -state.ly4 : state.ly4;

    // Adjust sweep frequency
    double rate = pow(pow(2.0, 4.0), 0.1 / (mSampleRate / 2.0));
    state.wp *= state.sweepfac;
    if (state.wp > state.max_wp) {
        state.sweepfac = 1.0 / rate;
    } else if (state.wp < state.min_wp) {
        state.sweepfac = rate;
    }

    return static_cast<float>(outval);
}

float DolbyProLogicIIDecoder::ApplyDelay(float input, DelayBuffer& delay) {
    float output = delay.buffer[delay.writePos];
    delay.buffer[delay.writePos] = input;
    delay.writePos = (delay.writePos + 1) % delay.delaySamples;
    return output;
}

int16_t DolbyProLogicIIDecoder::ClampToS16(float v) {
    if (v > 32767.0f) return 32767;
    if (v < -32768.0f) return -32768;
    return static_cast<int16_t>(v);
}

void DolbyProLogicIIDecoder::Process(const int16_t* stereoIn, int16_t* surroundOut, int numSamples) {
    if (!mInitialized) {
        Reset();
    }

    for (int i = 0; i < numSamples; i++) {
        float left = static_cast<float>(stereoIn[i * 2]);
        float right = static_cast<float>(stereoIn[i * 2 + 1]);

        // Center channel: (L + R) * 0.707/2, HP 70Hz, LP 20kHz
        float center = (left * COEF_CENTER) + (right * COEF_CENTER);
        center = ApplyLRFilter(center, mCenterHP, mCenterHPCoeffs);
        center = ApplyLRFilter(center, mCenterLP, mCenterLPCoeffs);

        // Front Left: L * 0.5
        float frontLeft = left * COEF_FRONT;

        // Front Right: R * 0.5
        float frontRight = right * COEF_FRONT;

        // Surround Left: L * 0.436 (HP, phase inverted) + R * 0.245 (HP, phase shifted), delayed
        float slL = left * COEF_SURR_MAIN;
        slL = ApplyLRFilter(slL, mSLLeftHP, mSurroundHPCoeffs);
        slL = ApplyPhaseShift(slL, mSLLeftPhase, true);  // inverted

        float slR = right * COEF_SURR_CROSS;
        slR = ApplyLRFilter(slR, mSLRightHP, mSurroundHPCoeffs);
        slR = ApplyPhaseShift(slR, mSLRightPhase, false);

        float surroundLeft = ApplyDelay(slL + slR, mSLDelay);

        // Surround Right: L * 0.245 (HP, phase inverted) + R * 0.436 (HP, phase shifted), delayed
        float srL = left * COEF_SURR_CROSS;
        srL = ApplyLRFilter(srL, mSRLeftHP, mSurroundHPCoeffs);
        srL = ApplyPhaseShift(srL, mSRLeftPhase, true);  // inverted

        float srR = right * COEF_SURR_MAIN;
        srR = ApplyLRFilter(srR, mSRRightHP, mSurroundHPCoeffs);
        srR = ApplyPhaseShift(srR, mSRRightPhase, false);

        float surroundRight = ApplyDelay(srL + srR, mSRDelay);

        // LFE: (L + R) * 0.707/2, LP 120Hz
        float lfe = (left * COEF_CENTER) + (right * COEF_CENTER);
        lfe = ApplyLRFilter(lfe, mLFELP, mLFELPCoeffs);

        // Output (5.1 channel order: FL, FR, C, LFE, RL, RR)
        surroundOut[i * 6 + 0] = ClampToS16(frontLeft);
        surroundOut[i * 6 + 1] = ClampToS16(frontRight);
        surroundOut[i * 6 + 2] = ClampToS16(center);
        surroundOut[i * 6 + 3] = ClampToS16(lfe);
        surroundOut[i * 6 + 4] = ClampToS16(surroundLeft);
        surroundOut[i * 6 + 5] = ClampToS16(surroundRight);
    }
}

} // namespace Ship
