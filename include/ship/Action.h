#pragma once

#include <memory>
#include <atomic>
#include <string>
#include <stdint.h>
#include <stddef.h>

#ifdef INCLUDE_PROFILING
#include <chrono>
#endif

#include "ship/Part.h"

namespace Ship {

#ifdef INCLUDE_PROFILING
/**
 * @brief Identifies a profiling time-point within an Action or Tickable.
 *
 * Used to query start/end timestamps of the current and previous execution cycles.
 */
enum class ClockType : uint64_t { Start, End, PreviousStart, PreviousEnd, ClockMax };
#endif

class Tickable;

/**
 * @brief Represents a discrete, repeatable operation run by a Tickable.
 *
 * Actions are owned by a Tickable and executed each frame/tick in type-sorted order.
 * Each Action is identified by a string event name (e.g. "Tick", "Draw") which is
 * registered with the Events component.  There are no built-in action types — all
 * event names are registered dynamically.
 *
 * The numeric type value is derived from the event name's hash for efficient sorting
 * and lookup.
 */
class Action : public Part {
  public:
    /**
     * @brief Constructs an Action with the given event name, associated with a Tickable.
     * @param eventName The event name this action corresponds to (registered with Events).
     * @param tickable The Tickable that owns and will run this Action.
     */
    Action(const std::string& eventName, std::shared_ptr<Tickable> tickable);
    virtual ~Action() = default;

    /** @brief Returns the numeric type derived from the event name hash (for sorting). */
    uint32_t GetType() const;

    /** @brief Returns the event name this action corresponds to. */
    const std::string& GetEventName() const;

    /** @brief Returns the Tickable that owns this Action, or nullptr if expired. */
    std::shared_ptr<Tickable> GetTickable() const;

    /**
     * @brief Executes the Action if it is currently running.
     * @param durationSinceLastTick Elapsed time in seconds since the last tick.
     * @return True if the Action executed successfully.
     */
    bool Run(const double durationSinceLastTick);

    /** @brief Returns true if this Action is currently in the running state. */
    bool IsRunning() const;

    /**
     * @brief Starts the Action so that subsequent Run() calls will execute it.
     * @param force If true, bypass the CanStart() check.
     * @return True if the Action was successfully started.
     */
    bool Start(const bool force = false);

    /**
     * @brief Stops the Action so that subsequent Run() calls become no-ops.
     * @param force If true, bypass the CanStop() check.
     * @return True if the Action was successfully stopped.
     */
    bool Stop(const bool force = false);

    /** @brief Permission hook; override to prevent starting. Defaults to true. */
    virtual bool CanStart();

    /** @brief Permission hook; override to prevent stopping. Defaults to true. */
    virtual bool CanStop();

    /**
     * @brief Notification hook called after the Action has been started.
     * @param forced Whether the start bypassed permission checks.
     */
    virtual void Started(const bool forced);

    /**
     * @brief Notification hook called after the Action has been stopped.
     * @param forced Whether the stop bypassed permission checks.
     */
    virtual void Stopped(const bool forced);

#ifdef INCLUDE_PROFILING
    /**
     * @brief Returns the wall-clock time for a profiling checkpoint.
     * @param clockType Which checkpoint to query.
     * @return Elapsed time in seconds since the epoch for the requested checkpoint.
     */
    double GetTime(const ClockType clockType) const;
#endif

  protected:
    /**
     * @brief Pure virtual hook invoked by Run() each cycle.
     *
     * Subclasses must implement this to define the Action's behaviour.
     * @param durationSinceLastTick Elapsed time in seconds since the last tick.
     * @return True if the action executed successfully.
     */
    virtual bool ActionRan(const double durationSinceLastTick) = 0;

  private:
#ifdef INCLUDE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> GetClock(const ClockType clockType) const;
    Action& SetClock(const ClockType clockType, std::chrono::time_point<std::chrono::steady_clock> clockValue);
#endif

    std::string mEventName;
    uint32_t mActionType;
    std::weak_ptr<Tickable> mTickable;
    bool mIsActionRunning;

#ifdef INCLUDE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> mClocks[static_cast<size_t>(ClockType::ClockMax)];
#endif
};

} // namespace Ship
