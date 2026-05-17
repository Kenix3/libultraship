#include "ship/audio/AudioResampler.h"

#include <cmath>
#include <cassert>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Ship {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

AudioResampler::AudioResampler(int32_t inRate, int32_t outRate, int32_t numChannels)
    : mInRate(inRate), mOutRate(outRate), mNumChannels(numChannels), mPhase(0) {

    int32_t g = GCD(inRate, outRate);
    mP = outRate / g;   /* upsample factor   (e.g. 3 for 32k→48k) */
    mQ = inRate  / g;   /* downsample factor (e.g. 2 for 32k→48k) */
    mNumPhases = mP;

    BuildFilter();

    /* History: kTapsPerPhase-1 past frames per channel, zero-initialized
     * so the first output frames fade in cleanly from silence. */
    mHistory.assign((kTapsPerPhase - 1) * mNumChannels, 0.0f);
}

// ---------------------------------------------------------------------------
// Filter construction — windowed-sinc lowpass, polyphase decomposition
// ---------------------------------------------------------------------------

float AudioResampler::BesselI0(float x) {
    /* Modified Bessel function of the first kind, order 0.
     * Used for Kaiser window computation.
     * Series expansion — converges well for x < 20 (beta up to ~14). */
    float sum = 1.0f;
    float term = 1.0f;
    float half_x = x * 0.5f;
    for (int k = 1; k <= 30; k++) {
        term *= (half_x / (float)k);
        term *= (half_x / (float)k);
        sum += term;
        if (term < 1e-12f * sum) break;
    }
    return sum;
}

float AudioResampler::KaiserWindow(int n, int N, float beta) {
    /* Kaiser window of length N+1, sample n in [0, N].
     * beta=6 gives ~60 dB stopband attenuation — good balance for audio. */
    float r = 2.0f * (float)n / (float)N - 1.0f;   /* normalise to [-1, 1] */
    float inside = 1.0f - r * r;
    if (inside < 0.0f) inside = 0.0f;
    return BesselI0(beta * sqrtf(inside)) / BesselI0(beta);
}

float AudioResampler::Sinc(float x) {
    if (fabsf(x) < 1e-8f) return 1.0f;
    float px = (float)M_PI * x;
    return sinf(px) / px;
}

void AudioResampler::BuildFilter() {
    /* Total filter length: P * kTapsPerPhase taps.
     * Cutoff at fc = 0.5 / max(P, Q) in normalised frequency (relative to
     * the upsampled rate P*inRate = P*outRate/Q). For 32k→48k: P=3, Q=2,
     * fc = 0.5/3 ≈ 0.167. */
    const int totalTaps = mNumPhases * kTapsPerPhase;
    const float fc = 0.5f / (float)std::max(mP, mQ);
    const float beta = 6.0f;
    const int N = totalTaps - 1;

    std::vector<float> h(totalTaps);

    /* Windowed sinc prototype filter */
    for (int i = 0; i < totalTaps; i++) {
        float x = (float)i - (float)N * 0.5f;
        h[i] = 2.0f * fc * Sinc(2.0f * fc * x) * KaiserWindow(i, N, beta);
    }

    /* Polyphase decomposition: interleave into mNumPhases banks.
     * Phase p contains taps h[p], h[p+P], h[p+2P], ...
     * Normalise by P so energy is preserved after upsampling. */
    mCoeffs.resize(mNumPhases * kTapsPerPhase);
    for (int phase = 0; phase < mNumPhases; phase++) {
        for (int tap = 0; tap < kTapsPerPhase; tap++) {
            mCoeffs[phase * kTapsPerPhase + tap] =
                h[phase + tap * mNumPhases] * (float)mP;
        }
    }
}

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

void AudioResampler::Reset() {
    std::fill(mHistory.begin(), mHistory.end(), 0.0f);
    mPhase = 0;
}

// ---------------------------------------------------------------------------
// MaxOutputFrames
// ---------------------------------------------------------------------------

int32_t AudioResampler::MaxOutputFrames(int32_t inFrames) const {
    /* ceil((inFrames * P) / Q) */
    return (int32_t)(((int64_t)inFrames * mP + mQ - 1) / mQ);
}

// ---------------------------------------------------------------------------
// Process — the core resampling loop
//
// Algorithm:
//   We conceptually upsample by P (insert P-1 zeros between each input sample)
//   then lowpass filter and downsample by Q.
//   The polyphase decomposition lets us do this efficiently without computing
//   the zero-padded samples: we advance through phases and only advance the
//   input pointer when we complete Q phases.
//
//   For each output sample:
//     1. Apply polyphase filter bank[mPhase] to the last kTapsPerPhase input frames.
//     2. Advance mPhase by Q. If mPhase >= P, subtract P and advance input by 1.
// ---------------------------------------------------------------------------

int32_t AudioResampler::Process(const int16_t* inBuf, int32_t inFrames,
                                int16_t* outBuf, int32_t maxOutFrames) {
    const int histLen = kTapsPerPhase - 1;
    const int ch = mNumChannels;

    /* Build a contiguous float window: history + new input.
     * history holds the last (kTapsPerPhase-1) input frames as float. */
    const int windowFrames = histLen + inFrames;
    std::vector<float> window(windowFrames * ch);

    /* Copy history */
    for (int i = 0; i < histLen * ch; i++) {
        window[i] = mHistory[i];
    }

    /* Convert new input S16 → float, normalised to [-1, 1] for arithmetic,
     * then back to S16 range at output. We keep S16 range (±32768) throughout
     * to avoid an extra normalisation multiply — clampf handles the final clip. */
    for (int i = 0; i < inFrames * ch; i++) {
        window[histLen * ch + i] = (float)inBuf[i];
    }

    /* Resample */
    int32_t outFrames = 0;
    int32_t inPos = 0;   /* current input frame position in window[] */
    int32_t phase = mPhase;

    while (inPos + kTapsPerPhase <= windowFrames && outFrames < maxOutFrames) {
        const float* coeffs = &mCoeffs[phase * kTapsPerPhase];

        for (int c = 0; c < ch; c++) {
            float acc = 0.0f;
            for (int tap = 0; tap < kTapsPerPhase; tap++) {
                acc += window[(inPos + tap) * ch + c] * coeffs[tap];
            }
            /* Clamp to S16 range */
            acc = acc < -32768.0f ? -32768.0f : (acc > 32767.0f ? 32767.0f : acc);
            outBuf[outFrames * ch + c] = (int16_t)acc;
        }
        outFrames++;

        /* Advance phase by Q; when phase wraps, consume one input frame */
        phase += mQ;
        if (phase >= mP) {
            phase -= mP;
            inPos++;
        }
    }

    /* Save tail of window as new history */
    const int consumed = inPos;  /* input frames consumed from window */
    const int remaining = histLen - (inFrames - consumed);

    if (inFrames >= histLen) {
        /* Enough new input to fill history entirely from inBuf */
        for (int i = 0; i < histLen * ch; i++) {
            mHistory[i] = window[(windowFrames - histLen) * ch + i];
        }
    } else {
        /* Partial update: shift old history and append new input */
        const int keep = histLen - inFrames;
        for (int i = 0; i < keep * ch; i++) {
            mHistory[i] = mHistory[inFrames * ch + i];
        }
        for (int i = 0; i < inFrames * ch; i++) {
            mHistory[keep * ch + i] = window[histLen * ch + i];
        }
    }

    mPhase = phase;
    return outFrames;
}

} // namespace Ship
