#pragma once

#include <memory>
#include <mutex>

#ifdef INCLDUE_TICKABLE_PROFILING
#include <chrono>
#endif

namespace Ship {
class Tickable : private std::enable_shared_from_this<Tickable> {
  public:
    Tickable(bool isTicking = true, bool isDrawing = true);
    ~Tickable();

    bool Tick(const double durationSinceLastTick = -1);
    bool Draw(const double durationSinceLastDraw = -1);

    bool IsTicking();
    bool IsDrawing();
    bool StartTicking(bool force = false);
    bool StartDrawing(bool force = false);
    bool StopTicking(bool force = false);
    bool StopDrawing(bool force = false);

#ifdef INCLDUE_TICKABLE_PROFILING
    double GetTickStartTime() const;
    double GetDrawStartTime() const;
    double GetTickEndTime() const;
    double GetDrawEndTime() const;
    double GetPreviousTickStartTime() const;
    double GetPreviousDrawStartTime() const;
    double GetPreviousTickEndTime() const;
    double GetPreviousDrawEndTime() const;
    double GetDurationSinceLastTick() const;
    double GetPreviousUpdateDuration() const;
    double GetPreviousDrawDuration() const;
    double GetPreviousDrawFullDuration() const;
#endif

protected:
#ifdef INCLDUE_TICKABLE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> GetTickStartClock() const;
    std::chrono::time_point<std::chrono::steady_clock> GetDrawStartClock() const;
    std::chrono::time_point<std::chrono::steady_clock> GetTickEndClock() const;
    std::chrono::time_point<std::chrono::steady_clock> GetDrawEndClock() const;
    std::chrono::time_point<std::chrono::steady_clock> GetPreviousTickStartClock() const;
    std::chrono::time_point<std::chrono::steady_clock> GetPreviousDrawStartClock() const;
    std::chrono::time_point<std::chrono::steady_clock> GetPreviousTickEndClock() const;
    std::chrono::time_point<std::chrono::steady_clock> GetPreviousDrawEndClock() const;
#endif

    virtual bool Ticked(const double durationSinceLastTick) = 0;
    virtual bool Drawn(const double durationSinceLastTick) = 0;
    virtual bool TickingStarted(bool forced) = 0;
    virtual bool DrawingStarted(bool forced) = 0;
    virtual bool TickingStopped(bool forced) = 0;
    virtual bool DrawingStopped(bool forced) = 0;

private:
#ifdef INCLDUE_TICKABLE_PROFILING
    Tickable& SetTickStartClock(std::chrono::time_point<std::chrono::steady_clock> tickStartClock);
    Tickable& SetDrawStartClock(std::chrono::time_point<std::chrono::steady_clock> drawStartClock);
    Tickable& SetTickEndClock(std::chrono::time_point<std::chrono::steady_clock> tickEndClock);
    Tickable& SetDrawEndClock(std::chrono::time_point<std::chrono::steady_clock> drawEndClock);
    Tickable& SetPreviousTickStartClock(std::chrono::time_point<std::chrono::steady_clock> previousTickStartClock);
    Tickable& SetPreviousDrawStartClock(std::chrono::time_point<std::chrono::steady_clock> previousDrawStartClock);
    Tickable& SetPreviousTickEndClock(std::chrono::time_point<std::chrono::steady_clock> previousTickEndClock);
    Tickable& SetPreviousDrawEndClock(std::chrono::time_point<std::chrono::steady_clock> previousDrawEndClock);
#endif

    bool mIsTicking;
    bool mIsDrawing;

#ifdef INCLUDE_TICKABLE_MULTITHREAD
    std::mutex mMutex;
#endif

#ifdef INCLDUE_TICKABLE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> mTickStartClock;
    std::chrono::time_point<std::chrono::steady_clock> mDrawStartClock;
    std::chrono::time_point<std::chrono::steady_clock> mTickEndClock;
    std::chrono::time_point<std::chrono::steady_clock> mDrawEndClock;
    std::chrono::time_point<std::chrono::steady_clock> mPreviousTickStartClock;
    std::chrono::time_point<std::chrono::steady_clock> mPreviousDrawStartClock;
    std::chrono::time_point<std::chrono::steady_clock> mPreviousTickEndClock;
    std::chrono::time_point<std::chrono::steady_clock> mPreviousDrawEndClock;
#endif
};

} // namespace Ship
