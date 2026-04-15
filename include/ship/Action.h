#pragma once

#include <memory>
#include <atomic>
#include <chrono>
#include <stdint.h>
#include <stddef.h>

#include "ship/Part.h"

namespace Ship {

enum class ClockType : uint64_t { Start, End, PreviousStart, PreviousEnd, ClockMax };

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

    double GetTime(const ClockType clockType) const;

  protected:
    virtual bool ActionRan(const double durationSinceLastTick) = 0;

  private:
    std::chrono::time_point<std::chrono::steady_clock> GetClock(const ClockType clockType) const;
    Action& SetClock(const ClockType clockType,
                     std::chrono::time_point<std::chrono::steady_clock> clockValue);

    uint32_t mActionType;
    std::shared_ptr<Tickable> mTickable;
    bool mIsActionRunning;

    std::chrono::time_point<std::chrono::steady_clock> mClocks[static_cast<size_t>(ClockType::ClockMax)];
};

} // namespace Ship
