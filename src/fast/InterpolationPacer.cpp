#include "fast/InterpolationPacer.h"

#include <algorithm>

namespace Fast {

namespace {
using Clock = std::chrono::steady_clock;

// Cost-model tuning. The steady/rising budget fractions are configurable per
// pacer (SetSmoothness); the rest are fixed internals.
constexpr double kRisingThreshold = 1.20; // How much busier a window counts as "rising".
constexpr int kRisingHoldWindows = 3;     // Measurement windows to stay cautious after a spike.
constexpr auto kSampleWindow = std::chrono::milliseconds(200);
constexpr double kEmaAlpha = 0.5;       // Weight of the newest window average.
constexpr double kSpikeFactor = 1.25;   // A sub-frame this much over peak counts as a spike.
constexpr double kSpikeBlend = 0.5;     // How hard a spike pulls the peak up.
constexpr double kPeakDecay = 0.40;     // How fast the peak eases down each window.
constexpr double kLogicRiseAlpha = 0.5; // Reserved-logic EMA: rise fast when logic gets heavier...
constexpr double kLogicFallAlpha = 0.1; // ...and fall slow when it calms.
} // namespace

InterpolationPacer::Pacing InterpolationPacer::Plan(const TickInputs& inputs) {
    int32_t nativeLogicFps = inputs.nativeLogicFps >= 1 ? inputs.nativeLogicFps : 1;
    int32_t target = inputs.targetFps;

    // Bound the target to what the render budget can sustain, unless the host
    // opted into the escape hatch (always honor the requested FPS).
    if (!inputs.bypassBudget) {
        target = BudgetCappedFps(target, nativeLogicFps);
    }
    // Game-specific ceiling (e.g. music-synced cutscenes capped at native).
    if (inputs.hardCapFps > 0 && target > inputs.hardCapFps) {
        target = inputs.hardCapFps;
    }

    // Integer sub-frames per tick: floor(target / native), at least one (the
    // keyframe). Replay/demo never interpolates.
    int32_t subframes = target / nativeLogicFps;
    if (subframes < 1) {
        subframes = 1;
    }
    if (inputs.forceNative) {
        subframes = 1;
    }

    // paceFps drives the per-present wait so that subframes * 1/paceFps equals one
    // tick of game time, keeping wall-clock aligned with the game's VI cadence.
    Pacing out;
    out.subframes = subframes;
    out.paceFps = subframes * nativeLogicFps;
    return out;
}

int32_t InterpolationPacer::BudgetCappedFps(int32_t userTarget, int32_t nativeLogicFps) {
    // Base the cap on the busiest recent sub-frame, not the average: every frame
    // we promise gets drawn, so the time has to fit the worst one.
    double costUs = std::max(mPeakPerSubFrameUs, mEmaPerSubFrameUs);
    if (costUs <= 0.0) {
        return userTarget; // Nothing measured yet.
    }
    double tickBudgetUs = 1'000'000.0 / nativeLogicFps;
    // Set aside the time game logic spent, then share what's left among the
    // interpolated sub-frames so a busy tick can't overshoot its budget.
    double renderBudgetUs = tickBudgetUs - mEnvLogicUs;
    if (renderBudgetUs <= 0.0) {
        return nativeLogicFps;
    }
    double maxSubPerTick = (renderBudgetUs * mSafety) / costUs;
    if (maxSubPerTick < 1.0) {
        return nativeLogicFps;
    }
    int32_t maxFps = (int32_t)(maxSubPerTick * nativeLogicFps);
    if (maxFps < nativeLogicFps) {
        maxFps = nativeLogicFps;
    }
    return std::min(userTarget, maxFps);
}

void InterpolationPacer::ReportLogicTime(int64_t ns) {
    double us = (double)ns / 1000.0;
    if (us < 0.0) {
        return;
    }
    if (mEnvLogicUs == 0.0) {
        mEnvLogicUs = us;
    } else {
        // Rise fast when logic gets heavier, fall slow when it calms.
        double alpha = us > mEnvLogicUs ? kLogicRiseAlpha : kLogicFallAlpha;
        mEnvLogicUs = alpha * us + (1.0 - alpha) * mEnvLogicUs;
    }
}

void InterpolationPacer::ReportSubframeCost(int64_t ns) {
    double sampleUs = (double)ns / 1000.0;

    mWinRunNs += ns;
    mWinSubFrames++;
    if (sampleUs > mWinMaxUs) {
        mWinMaxUs = sampleUs;
    }

    // A sudden spike bumps the peak up immediately and keeps us cautious, so the
    // next cap already reflects it instead of waiting for the window to close.
    if (mPeakPerSubFrameUs == 0.0) {
        mPeakPerSubFrameUs = sampleUs;
        mSafety = mSafetyRising;
        mRisingHoldRemaining = kRisingHoldWindows;
    } else if (sampleUs > mPeakPerSubFrameUs * kSpikeFactor) {
        mPeakPerSubFrameUs = kSpikeBlend * sampleUs + (1.0 - kSpikeBlend) * mPeakPerSubFrameUs;
        mSafety = mSafetyRising;
        mRisingHoldRemaining = kRisingHoldWindows;
    }

    auto now = Clock::now();
    if (now - mWinStart < kSampleWindow) {
        return;
    }

    // End of a measurement window: refresh the average, decide whether the scene
    // is still getting busier, and ease the peak back down.
    double mean = (double)mWinRunNs / mWinSubFrames / 1000.0;
    mEmaPerSubFrameUs = mEmaPerSubFrameUs == 0.0 ? mean : kEmaAlpha * mean + (1.0 - kEmaAlpha) * mEmaPerSubFrameUs;

    bool rising = mPeakPerSubFrameUs > 0.0 && mWinMaxUs > mPeakPerSubFrameUs * kRisingThreshold;
    if (rising) {
        mRisingHoldRemaining = kRisingHoldWindows;
    } else if (mRisingHoldRemaining > 0) {
        mRisingHoldRemaining--;
    }
    mSafety = mRisingHoldRemaining > 0 ? mSafetyRising : mSafetySteady;

    mPeakPerSubFrameUs = std::max(mWinMaxUs, mPeakPerSubFrameUs * (1.0 - kPeakDecay) + mWinMaxUs * kPeakDecay);

    mWinRunNs = 0;
    mWinSubFrames = 0;
    mWinMaxUs = 0.0;
    mWinStart = now;
}

void InterpolationPacer::SetSmoothness(float fraction) {
    // Clamp to a sane range: below 0.5 wastes headroom, above 1.0 is impossible.
    double steady = fraction < 0.5f ? 0.5 : (fraction > 1.0f ? 1.0 : (double)fraction);
    mSafetySteady = steady;
    // Keep the cautious (rising) safety a fixed margin below steady, floored at 0.5.
    mSafetyRising = steady - 0.20 > 0.5 ? steady - 0.20 : 0.5;
}

void InterpolationPacer::Reset() {
    mEmaPerSubFrameUs = 0.0;
    mPeakPerSubFrameUs = 0.0;
    mEnvLogicUs = 0.0;
    mWinRunNs = 0;
    mWinSubFrames = 0;
    mWinMaxUs = 0.0;
    mWinStart = Clock::now();
    mSafety = mSafetySteady;
    mRisingHoldRemaining = 0;
}

} // namespace Fast
