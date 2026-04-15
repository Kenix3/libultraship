#pragma once

#include <memory>
#include <atomic>
#include <stdint.h>
#include <stddef.h>

#ifdef INCLUDE_PROFILING
#include <chrono>
#endif

#include "ship/Part.h"

namespace Ship {

#ifdef INCLUDE_PROFILING
enum class ClockType : uint64_t { Start, End, PreviousStart, PreviousEnd, ClockMax };
#endif

enum class ActionType : uint32_t { Tick = 0, Draw = 1000000, DrawDebugMenu = 2000000 };

class Tickable;

class Action : public Part {
  public:
    Action(const uint32_t actionType, std::shared_ptr<Tickable> tickable);
    virtual ~Action() = default;

    uint32_t GetType() const;
    std::shared_ptr<Tickable> GetTickable() const;

    bool Run(const double durationSinceLastTick);
    bool IsRunning() const;
    bool Start(const bool force = false);
    bool Stop(const bool force = false);

    virtual bool CanStart();
    virtual bool CanStop();
    virtual void Started(const bool forced);
    virtual void Stopped(const bool forced);

#ifdef INCLUDE_PROFILING
    double GetTime(const ClockType clockType) const;
#endif

  protected:
    virtual bool ActionRan(const double durationSinceLastTick) = 0;

  private:
#ifdef INCLUDE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> GetClock(const ClockType clockType) const;
    Action& SetClock(const ClockType clockType,
                     std::chrono::time_point<std::chrono::steady_clock> clockValue);
#endif

    uint32_t mActionType;
    std::shared_ptr<Tickable> mTickable;
    bool mIsActionRunning;

#ifdef INCLUDE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> mClocks[static_cast<size_t>(ClockType::ClockMax)];
#endif
};

} // namespace Ship
