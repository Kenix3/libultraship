#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <stdint.h>
#include <stddef.h>
#ifdef COMPONENT_THREAD_SAFE
#include <atomic>
#endif

#include "ship/core/Action.h"
#include "ship/core/ActionList.h"
#include "ship/events/EventTypes.h"

namespace Ship {

class Component;

/**
 * @brief Manages a collection of Actions, executing them in EventID order.
 *
 * A Tickable owns zero or more Actions and provides Tick() methods that run
 * EventID-targeted subsets of those Actions each frame. Thread safety for
 * the action list is handled internally via a mutex when Thread safety is on.
 */
class Tickable : public std::enable_shared_from_this<Tickable> {
  public:
    /**
     * @brief Constructs a Tickable.
     * @param isTicking If true, the Tickable starts in the ticking state.
     */
    Tickable(const bool isTicking = true);

    /**
     * @brief Constructs a Tickable with an initial set of Actions.
     * @param isTicking If true, the Tickable starts in the ticking state.
     * @param actions The Actions to add upon construction.
     */
    Tickable(const bool isTicking, const std::vector<std::shared_ptr<Action>>& actions);
    virtual ~Tickable();

    /** @brief Returns true if this Tickable is currently ticking. */
    bool IsTicking() const;

    /**
     * @brief Starts ticking, enabling EventID-targeted Tick() calls on Actions.
     * @param force If true, bypass the CanStart() check.
     * @return True if successfully started.
     */
    bool Start(const bool force = false);

    /**
     * @brief Stops ticking; subsequent Tick() calls become no-ops.
     * @param force If true, bypass the CanStop() check.
     * @return True if successfully stopped.
     */
    bool Stop(const bool force = false);

    /**
     * @brief Ticks only Actions whose EventID matches the given ID.
     * @param eventId The EventID to include.
     * @return True if at least one matching Action ran.
     */
    bool Tick(EventID eventId);

    /**
     * @brief Ticks only Actions whose EventID matches one of the given IDs.
     * @param eventIds The EventIDs to include.
     * @return True if at least one matching Action ran.
     */
    bool Tick(const std::vector<EventID>& eventIds);

    /**
     * @brief Ticks Actions matching both type T and one of the given EventIDs.
     * @tparam T The Action subtype to filter by.
     * @param eventIds The EventIDs to include.
     * @return True if at least one matching Action ran.
     */
    template <typename T> bool Tick(const std::vector<EventID>& eventIds);

    /**
     * @brief Ticks Actions matching both type T and the given EventID.
     * @tparam T The Action subtype to filter by.
     * @param eventId The EventID to include.
     * @return True if at least one matching Action ran.
     */
    template <typename T> bool Tick(EventID eventId);

    /**
     * @brief Returns a mutable reference to the ActionList.
     */
    ActionList& GetActionList();

    /**
     * @brief Returns a const reference to the ActionList.
     */
    const ActionList& GetActionList() const;

#ifdef INCLUDE_PROFILING
    /**
     * @brief Returns the wall-clock time for a profiling checkpoint.
     * @param clockType Which checkpoint to query.
     * @return Elapsed time in seconds since the epoch for the requested checkpoint.
     */
    double GetTime(const ClockType clockType) const;
#endif

  protected:
    /** @brief Permission hook; override to prevent starting. Defaults to true. */
    virtual bool CanStart();

    /** @brief Permission hook; override to prevent stopping. Defaults to true. */
    virtual bool CanStop();

    /**
     * @brief Notification hook called after the Tickable has been started.
     * @param forced Whether the start bypassed permission checks.
     */
    virtual void Started(const bool forced);

    /**
     * @brief Notification hook called after the Tickable has been stopped.
     * @param forced Whether the stop bypassed permission checks.
     */
    virtual void Stopped(const bool forced);

    /** @brief Returns a reference to the internal mutex for synchronization.
     *  Only available when COMPONENT_THREAD_SAFE is defined. */
#ifdef COMPONENT_THREAD_SAFE
    std::mutex& GetMutex();
#endif

  private:
#ifdef INCLUDE_PROFILING
    Tickable& SetClock(const ClockType clockType, std::chrono::time_point<std::chrono::steady_clock> clockValue);
    std::chrono::time_point<std::chrono::steady_clock> GetClock(const ClockType clockType) const;
#endif

#ifdef COMPONENT_THREAD_SAFE
    std::atomic<bool> mIsTicking;
#else
    bool mIsTicking;
#endif
    ActionList mActions;
#ifdef COMPONENT_THREAD_SAFE
    mutable std::mutex mMutex;
#endif
#ifdef INCLUDE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> mClocks[static_cast<size_t>(ClockType::ClockMax)];
#endif
};

// ---- Template method implementations ----

template <typename T> bool Tickable::Tick(const std::vector<EventID>& eventIds) {
#ifdef COMPONENT_THREAD_SAFE
    if (!mIsTicking.load(std::memory_order_acquire)) {
#else
    if (!mIsTicking) {
#endif
        return false;
    }
#ifdef INCLUDE_PROFILING
    const auto start = std::chrono::steady_clock::now();
#endif
    bool ran = false;
    auto allActions = mActions.Get(eventIds);
    for (const auto& action : *allActions) {
        if (std::dynamic_pointer_cast<T>(action)) {
            ran = action->Run() || ran;
        }
    }
#ifdef INCLUDE_PROFILING
    const auto end = std::chrono::steady_clock::now();
    (void)end;
    (void)start;
#endif
    return ran;
}

template <typename T> bool Tickable::Tick(EventID eventId) {
#ifdef COMPONENT_THREAD_SAFE
    if (!mIsTicking.load(std::memory_order_acquire)) {
#else
    if (!mIsTicking) {
#endif
        return false;
    }
#ifdef INCLUDE_PROFILING
    const auto start = std::chrono::steady_clock::now();
#endif
    bool ran = false;
    auto allActions = mActions.Get(eventId);
    for (const auto& action : *allActions) {
        if (std::dynamic_pointer_cast<T>(action)) {
            ran = action->Run() || ran;
        }
    }
#ifdef INCLUDE_PROFILING
    const auto end = std::chrono::steady_clock::now();
    (void)end;
    (void)start;
#endif
    return ran;
}

} // namespace Ship
