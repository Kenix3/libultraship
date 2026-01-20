#include "ship/audio/DolbyProLogicII.h"
#include <cstring>

namespace Ship {

// Dolby Pro Logic II coefficients
static constexpr float gCoefCenter = 0.35355339059327376220042218105242f; // 1/(2*sqrt(2))
static constexpr float gCoefFront = 0.5f;
static constexpr float gCoefSurrMain = 0.43588989435406735522369819838596f;  // sqrt(19)/10
static constexpr float gCoefSurrCross = 0.24494897427831780981972840747059f; // sqrt(6)/10
static constexpr int gDelayMs = 10;

DolbyProLogicIIDecoder::DolbyProLogicIIDecoder(int32_t sampleRate) : mSampleRate(sampleRate) {
    Reset();
}

void DolbyProLogicIIDecoder::SetSampleRate(int32_t sampleRate) {
    mSampleRate = sampleRate;
    mInitialized = false;
}

void DolbyProLogicIIDecoder::Reset() {
    // Calculate delay in samples (10ms)
    mDelaySamples = (mSampleRate / 1000) * gDelayMs;
    if (mDelaySamples > gMaxDelaySamples) {
        mDelaySamples = gMaxDelaySamples;
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
    mSLDelay.DelaySamples = mDelaySamples;
    mSRDelay = DelayBuffer{};
    mSRDelay.DelaySamples = mDelaySamples;

    mInitialized = true;
}

DolbyProLogicIIDecoder::LRFilterCoeffs DolbyProLogicIIDecoder::CalcLRLowPassCoeffs(double cutoff) {
    LRFilterCoeffs c{};
    // Clamp cutoff to 95% of Nyquist to avoid numerical issues with bilinear transform
    double maxCutoff = mSampleRate * 0.475;
    if (cutoff > maxCutoff) {
        cutoff = maxCutoff;
    }
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

    c.B1 = (4.0 * (wc4 + sq_tmp1 - k4 - sq_tmp2)) / a_tmp;
    c.B2 = (6.0 * wc4 - 8.0 * wc2 * k2 + 6.0 * k4) / a_tmp;
    c.B3 = (4.0 * (wc4 - sq_tmp1 + sq_tmp2 - k4)) / a_tmp;
    c.B4 = (k4 - 2.0 * sq_tmp1 + wc4 - 2.0 * sq_tmp2 + 4.0 * wc2 * k2) / a_tmp;

    c.A0 = wc4 / a_tmp;
    c.A1 = 4.0 * wc4 / a_tmp;
    c.A2 = 6.0 * wc4 / a_tmp;
    c.A3 = c.A1;
    c.A4 = c.A0;
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

    c.B1 = (4.0 * (wc4 + sq_tmp1 - k4 - sq_tmp2)) / a_tmp;
    c.B2 = (6.0 * wc4 - 8.0 * wc2 * k2 + 6.0 * k4) / a_tmp;
    c.B3 = (4.0 * (wc4 - sq_tmp1 + sq_tmp2 - k4)) / a_tmp;
    c.B4 = (k4 - 2.0 * sq_tmp1 + wc4 - 2.0 * sq_tmp2 + 4.0 * wc2 * k2) / a_tmp;

    c.A0 = k4 / a_tmp;
    c.A1 = -4.0 * k4 / a_tmp;
    c.A2 = 6.0 * k4 / a_tmp;
    c.A3 = c.A1;
    c.A4 = c.A0;
    return c;
}

float DolbyProLogicIIDecoder::ApplyLRFilter(float input, LRFilterState& state, const LRFilterCoeffs& c) {
    double tempx = input;
    double tempy = c.A0 * tempx + c.A1 * state.Xm1 + c.A2 * state.Xm2 + c.A3 * state.Xm3 + c.A4 * state.Xm4 -
                   c.B1 * state.Ym1 - c.B2 * state.Ym2 - c.B3 * state.Ym3 - c.B4 * state.Ym4;
    state.Xm4 = state.Xm3;
    state.Xm3 = state.Xm2;
    state.Xm2 = state.Xm1;
    state.Xm1 = tempx;
    state.Ym4 = state.Ym3;
    state.Ym3 = state.Ym2;
    state.Ym2 = state.Ym1;
    state.Ym1 = tempy;
    return static_cast<float>(tempy);
}

void DolbyProLogicIIDecoder::InitPhaseShift(PhaseShiftState& state) {
    double depth = 4.0;
    double delay = 100.0;
    double rate = 0.1;
    state.Wp = state.MinWp = (M_PI * delay) / mSampleRate;
    double range = pow(2.0, depth);
    state.MaxWp = (M_PI * delay * range) / mSampleRate;
    rate = pow(range, rate / (mSampleRate / 2.0));
    state.SweepFac = rate;
    state.Initialized = true;
}

float DolbyProLogicIIDecoder::ApplyPhaseShift(float input, PhaseShiftState& state, bool invert) {
    if (!state.Initialized) {
        InitPhaseShift(state);
    }

    double coef = (1.0 - state.Wp) / (1.0 + state.Wp);
    double x1 = static_cast<double>(input);

    state.Ly1 = coef * (state.Ly1 + x1) - state.Lx1;
    state.Lx1 = x1;
    state.Ly2 = coef * (state.Ly2 + state.Ly1) - state.Lx2;
    state.Lx2 = state.Ly1;
    state.Ly3 = coef * (state.Ly3 + state.Ly2) - state.Lx3;
    state.Lx3 = state.Ly2;
    state.Ly4 = coef * (state.Ly4 + state.Ly3) - state.Lx4;
    state.Lx4 = state.Ly3;

    double outval = invert ? -state.Ly4 : state.Ly4;

    // Adjust sweep frequency
    double rate = pow(pow(2.0, 4.0), 0.1 / (mSampleRate / 2.0));
    state.Wp *= state.SweepFac;
    if (state.Wp > state.MaxWp) {
        state.SweepFac = 1.0 / rate;
    } else if (state.Wp < state.MinWp) {
        state.SweepFac = rate;
    }

    return static_cast<float>(outval);
}

float DolbyProLogicIIDecoder::ApplyDelay(float input, DelayBuffer& delay) {
    float output = delay.Buffer[delay.WritePos];
    delay.Buffer[delay.WritePos] = input;
    delay.WritePos = (delay.WritePos + 1) % delay.DelaySamples;
    return output;
}

int16_t DolbyProLogicIIDecoder::ClampToS16(float v) {
    if (v > 32767.0f) {
        return 32767;
    }
    if (v < -32768.0f) {
        return -32768;
    }
    return static_cast<int16_t>(v);
}

void DolbyProLogicIIDecoder::Process(const int16_t* stereoIn, int16_t* surroundOut, int numSamples) {
    bool initialized = mInitialized;
    if (!mInitialized) {
        Reset();
    }

    for (int i = 0; i < numSamples; i++) {
        float left = static_cast<float>(stereoIn[i * 2]);
        float right = static_cast<float>(stereoIn[i * 2 + 1]);

        // Center channel: (L + R) * 0.707/2, HP 70Hz, LP 20kHz
        float center = (left * gCoefCenter) + (right * gCoefCenter);
        center = ApplyLRFilter(center, mCenterHP, mCenterHPCoeffs);
        center = ApplyLRFilter(center, mCenterLP, mCenterLPCoeffs);

        // Front Left: L * 0.5
        float frontLeft = left * gCoefFront;

        // Front Right: R * 0.5
        float frontRight = right * gCoefFront;

        // Surround Left: L * 0.436 (HP, phase inverted) + R * 0.245 (HP, phase shifted), delayed
        float slL = left * gCoefSurrMain;
        slL = ApplyLRFilter(slL, mSLLeftHP, mSurroundHPCoeffs);
        slL = ApplyPhaseShift(slL, mSLLeftPhase, true); // inverted

        float slR = right * gCoefSurrCross;
        slR = ApplyLRFilter(slR, mSLRightHP, mSurroundHPCoeffs);
        slR = ApplyPhaseShift(slR, mSLRightPhase, false);

        float surroundLeft = ApplyDelay(slL + slR, mSLDelay);

        // Surround Right: L * 0.245 (HP, phase inverted) + R * 0.436 (HP, phase shifted), delayed
        float srL = left * gCoefSurrCross;
        srL = ApplyLRFilter(srL, mSRLeftHP, mSurroundHPCoeffs);
        srL = ApplyPhaseShift(srL, mSRLeftPhase, true); // inverted

        float srR = right * gCoefSurrMain;
        srR = ApplyLRFilter(srR, mSRRightHP, mSurroundHPCoeffs);
        srR = ApplyPhaseShift(srR, mSRRightPhase, false);

        float surroundRight = ApplyDelay(srL + srR, mSRDelay);

        // LFE: (L + R) * 0.707/2, LP 120Hz
        float lfe = (left * gCoefCenter) + (right * gCoefCenter);
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
