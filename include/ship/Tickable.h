#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#define INCLUDE_PROFILING 1
#define INCLUDE_MUTEX 1


#ifdef INCLUDE_PROFILING
#include <chrono>
#endif

namespace Ship {
#ifdef INCLUDE_PROFILING
enum class ClockType : uint64_t {
    TickStart,
    DrawStart,
    DebugMenuDrawStart,
    TickEnd,
    DrawEnd,
    DebugMenuDrawEnd,
    PreviousTickStart,
    PreviousDrawStart,
    PreviousDebugMenuDrawStart,
    PreviousTickEnd,
    PreviousDrawEnd,
    PreviousDebugMenuDrawEnd,
    ClockMax
};
#endif

enum class ActionType : size_t {
    Tick,
    Draw,
    DrawDebugMenu
};

class Tickable {
  public:
    Tickable(const std::vector<bool>& actions);
    Tickable(const bool isTicking = true, const bool isDrawing = true);
    ~Tickable();

    bool RunAction(const ActionType action, const double durationSinceLastTick);
    bool IsActionRunning(const ActionType action);
    bool StartAction(const ActionType action, const bool force = false);
    bool StopAction(const ActionType action, const bool force = false);

    virtual bool ActionRan(const ActionType action, const double durationSinceLastTick);

#ifdef INCLUDE_PROFILING
    double GetTime(const ClockType clockType) const;

    double GetDurationSinceLastTick() const;
    double GetPreviousUpdateDuration() const;
    double GetPreviousDrawDuration() const;
#endif

protected:
#ifdef INCLUDE_PROFILING
  std::chrono::time_point<std::chrono::steady_clock> GetClock(const ClockType clockType) const;
#endif

    virtual bool CanStartAction(const ActionType action);
    virtual bool CanStopAction(const ActionType action);
    virtual void ActionStarted(const ActionType action, const bool forced);
    virtual void ActionStopped(const ActionType action, const bool forced);

private:
#ifdef INCLUDE_PROFILING
    Tickable& SetClock(const ClockType clockType, std::chrono::time_point<std::chrono::steady_clock> clockValue);
#endif

    std::unordered_map<ActionType, bool> mActions;

#ifdef INCLUDE_MUTEX
    std::mutex mMutex;
#endif
#ifdef INCLUDE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> mClocks[static_cast<unsigned long long>(ClockType::ClockMax)];
#endif
};

} // namespace Ship
