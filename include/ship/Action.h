#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <stdint.h>
#incldue <stddef.h>

#define INCLUDE_PROFILING 1
#define INCLUDE_MUTEX 1

namespace Ship {

enum class ActionType : size_t { 
    Tick = 0, Draw = 1000000, DrawDebugMenu = 2000000
};

class Tickable;

class Action {
    // TODO: Action should use the tickable's mutex.
public:
    Action(const uint32_t actionType, std::shared_ptr<Tickable> tickable);

    uint32_t GetId() const;
    std::shared_ptr<Tickable> GetTickable() const;

    bool Run(const double durationSinceLastTick);
    bool IsRunning();
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
#ifdef INCLUDE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> GetClock(const ClockType clockType) const;
#endif
    virtual bool ActionRan(const double durationSinceLastTick) = 0;

  private:
#ifdef INCLUDE_PROFILING
    Tickable& SetClock(const ClockType clockType, std::chrono::time_point<std::chrono::steady_clock> clockValue);
#endif

    static std::atomic_uint NextActionId;

    uint32_t mActionId;
    uint32_t mActionType;
    std::shared_ptr<Tickable> mTickable;
    bool mIsActionRunning;

#ifdef INCLUDE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> mClocks[static_cast<unsigned long long>(ClockType::ClockMax)];
#endif
};
} // namespace Ship