#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <stdint.h>
#include <stddef.h>

#include "ship/Action.h"
#include "ship/ActionList.h"

namespace Ship {

class Component;

/**
 * @brief Manages a collection of Actions, executing them in type-sorted order.
 *
 * A Tickable owns zero or more Actions and provides Run() methods that run all
 * (or a filtered subset of) those Actions each frame. Actions are executed in
 * ascending action-type order. Thread safety for the action list is handled
 * internally via a mutex.
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
     * @brief Starts ticking, enabling Run() on all Actions.
     * @param force If true, bypass the CanStart() check.
     * @return True if successfully started.
     */
    bool Start(const bool force = false);

    /**
     * @brief Stops ticking; subsequent Run() calls become no-ops.
     * @param force If true, bypass the CanStop() check.
     * @return True if successfully stopped.
     */
    bool Stop(const bool force = false);

    /**
     * @brief Runs all running Actions sorted by action type (low to high).
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    double Run(const double durationSinceLastTick);

    /**
     * @brief Runs only Actions that can be dynamic_cast to type T.
     * @tparam T The Action subtype to filter by.
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    template <typename T> double Run(const double durationSinceLastTick);

    /**
     * @brief Runs only Actions whose type matches one of the given action types.
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @param actionTypes The action type IDs to include.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    double Run(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes);

    /**
     * @brief Runs Actions matching both type T and one of the given action types.
     * @tparam T The Action subtype to filter by.
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @param actionTypes The action type IDs to include.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    template <typename T> double Run(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes);

    /**
     * @brief Runs only Actions whose type matches the given action type.
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @param actionType The action type ID to include.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    double Run(const double durationSinceLastTick, const uint32_t actionType);

    /**
     * @brief Runs Actions matching both type T and the given action type.
     * @tparam T The Action subtype to filter by.
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @param actionType The action type ID to include.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    template <typename T> double Run(const double durationSinceLastTick, const uint32_t actionType);

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

    /** @brief Returns a reference to the internal mutex for synchronization. */
    std::mutex& GetMutex();

  private:
#ifdef INCLUDE_PROFILING
    Tickable& SetClock(const ClockType clockType, std::chrono::time_point<std::chrono::steady_clock> clockValue);
    std::chrono::time_point<std::chrono::steady_clock> GetClock(const ClockType clockType) const;
#endif

    ActionList mActions;
    bool mIsTicking;
    mutable std::mutex mMutex;
#ifdef INCLUDE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> mClocks[static_cast<size_t>(ClockType::ClockMax)];
#endif
};

// ---- Template method implementations ----

template <typename T> double Tickable::Run(const double durationSinceLastTick) {
    if (!mIsTicking) {
        return 0.0;
    }
#ifdef INCLUDE_PROFILING
    const auto start = std::chrono::steady_clock::now();
#endif
    const std::lock_guard<std::mutex> lock(mMutex);
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    for (const auto& action : mActions.GetList()) {
        if (std::dynamic_pointer_cast<T>(action)) {
            result->push_back(action);
        }
    }
    for (const auto& action : *result) {
        action->Run(durationSinceLastTick);
    }
#ifdef INCLUDE_PROFILING
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(end - start).count();
#else
    return 0.0;
#endif
}

template <typename T>
double Tickable::Run(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes) {
    if (!mIsTicking) {
        return 0.0;
    }
#ifdef INCLUDE_PROFILING
    const auto start = std::chrono::steady_clock::now();
#endif
    const std::lock_guard<std::mutex> lock(mMutex);
    auto allActions = mActions.Get(actionTypes);
    for (const auto& action : *allActions) {
        if (std::dynamic_pointer_cast<T>(action)) {
            action->Run(durationSinceLastTick);
        }
    }
#ifdef INCLUDE_PROFILING
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(end - start).count();
#else
    return 0.0;
#endif
}

template <typename T> double Tickable::Run(const double durationSinceLastTick, const uint32_t actionType) {
    if (!mIsTicking) {
        return 0.0;
    }
#ifdef INCLUDE_PROFILING
    const auto start = std::chrono::steady_clock::now();
#endif
    const std::lock_guard<std::mutex> lock(mMutex);
    auto allActions = mActions.Get(actionType);
    for (const auto& action : *allActions) {
        if (std::dynamic_pointer_cast<T>(action)) {
            action->Run(durationSinceLastTick);
        }
    }
#ifdef INCLUDE_PROFILING
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(end - start).count();
#else
    return 0.0;
#endif
}

} // namespace Ship
