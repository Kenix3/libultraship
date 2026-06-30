#pragma once

#include <chrono>
#include <cstdint>

namespace Fast {

/**
 * @brief Decides how many interpolated sub-frames to render between two
 * fixed-rate game ticks, bounded by the measured render budget.
 *
 * Frame interpolation renders extra "tween" frames between a game's authentic,
 * fixed-rate logic ticks. On a single-threaded port, rendering more tweens than
 * the hardware can draw in a tick drags the whole loop -- and therefore the game
 * logic -- into slow-motion. This bounds the sub-frame count to what the render
 * budget can actually sustain, so a heavy scene falls back toward the native rate
 * instead of stalling.
 *
 * The host owns the game-specific interpolation *recording*; this owns the pacing
 * math and the render-cost model. Each tick the host reports how long game logic
 * took (ReportLogicTime) and the cost of every rendered sub-frame
 * (ReportSubframeCost), then asks Plan() how many sub-frames fit and at what
 * present rate.
 */
class InterpolationPacer {
  public:
    /** @brief Per-tick inputs supplied by the host (game-specific state resolved). */
    struct TickInputs {
        // Required -- the host must supply these. There is no universal default:
        // the native sim rate is game-specific (e.g. 30 Hz for BK/SM64, 20 Hz for
        // OoT/MM) and can vary per tick (cutscene/pause), so the port computes it.
        // The 0 defaults are an unset sentinel, not a usable rate.
        int32_t targetFps = 0;      ///< User-requested interpolation target (host-supplied).
        int32_t nativeLogicFps = 0; ///< Authentic sim rate this tick, e.g. 60 / viPerTick (host-supplied).
        // Optional, with safe defaults.
        int32_t hardCapFps = 0;    ///< Game-specific ceiling (e.g. cutscene); 0 = none.
        bool bypassBudget = false; ///< Escape hatch: honor targetFps regardless of cost.
        bool forceNative = false;  ///< Replay/demo: never interpolate (one render/tick).
    };

    /** @brief The pacing decision for a tick. */
    struct Pacing {
        int32_t subframes = 1; ///< Renders to emit this tick (>= 1; the last is the keyframe).
        int32_t paceFps = 30;  ///< Present-pace fps to feed Window::SetTargetFps.
    };

    /** @brief Compute the pacing for the tick about to render. */
    Pacing Plan(const TickInputs& inputs);

    /** @brief Report the wall-time one rendered sub-frame took, in nanoseconds. */
    void ReportSubframeCost(int64_t ns);

    /**
     * @brief Report the wall-time game logic took this tick before drawing began,
     * in nanoseconds. Reserved from the budget so a busy tick can't overschedule.
     */
    void ReportLogicTime(int64_t ns);

    /**
     * @brief Set how much of each tick's render budget to spend on interpolated
     * frames, as a fraction in [0.5, 1.0]. Higher = smoother (more tweens, closer
     * to the budget edge); lower = steadier (more headroom). The budget itself is
     * always enforced -- this only tunes how aggressively it is filled.
     */
    void SetSmoothness(float fraction);

    /** @brief Forget past measurements (e.g. on game/scene load) so pacing starts fresh. */
    void Reset();

  private:
    int32_t BudgetCappedFps(int32_t userTarget, int32_t nativeLogicFps);

    double mEmaPerSubFrameUs = 0.0;  // Average sub-frame draw time.
    double mPeakPerSubFrameUs = 0.0; // Busiest recent sub-frame; the cap must fit the worst one.
    double mEnvLogicUs = 0.0;        // Reserved game-update time per tick.
    int64_t mWinRunNs = 0;           // Accumulators for the current measurement window.
    int32_t mWinSubFrames = 0;
    double mWinMaxUs = 0.0;
    std::chrono::steady_clock::time_point mWinStart = std::chrono::steady_clock::now();
    double mSafetySteady = 0.95;      // Budget fraction spent once steady (set by SetSmoothness).
    double mSafetyRising = 0.75;      // Budget fraction while a scene is getting busier (cautious).
    double mSafety = 0.95;            // Current safety, oscillating between steady and rising.
    int32_t mRisingHoldRemaining = 0; // Windows left to stay cautious after a load spike.
};

} // namespace Fast
