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
 * A Tickable owns zero or more Actions and provides Tick() methods that run all
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
     * @brief Stops ticking; subsequent Tick() calls become no-ops.
     * @param force If true, bypass the CanStop() check.
     * @return True if successfully stopped.
     */
    bool Stop(const bool force = false);

    /**
     * @brief Runs all running Actions sorted by action type (low to high).
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    double Tick(const double durationSinceLastTick);

    /**
     * @brief Runs only Actions that can be dynamic_cast to type T.
     * @tparam T The Action subtype to filter by.
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    template <typename T> double Tick(const double durationSinceLastTick);

    /**
     * @brief Runs only Actions whose type matches one of the given action types.
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @param actionTypes The action type IDs to include.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    double Tick(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes);

    /**
     * @brief Runs Actions matching both type T and one of the given action types.
     * @tparam T The Action subtype to filter by.
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @param actionTypes The action type IDs to include.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    template <typename T> double Tick(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes);

    /**
     * @brief Runs only Actions whose type matches the given action type.
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @param actionType The action type ID to include.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    double Tick(const double durationSinceLastTick, const uint32_t actionType);

    /**
     * @brief Runs Actions matching both type T and the given action type.
     * @tparam T The Action subtype to filter by.
     * @param durationSinceLastTick Elapsed time in seconds since the previous tick.
     * @param actionType The action type ID to include.
     * @return Duration of the tick in seconds (non-zero only with INCLUDE_PROFILING).
     */
    template <typename T> double Tick(const double durationSinceLastTick, const uint32_t actionType);

    /**
     * @brief Checks whether a specific Action is registered.
     * @param action The Action to look for.
     * @return True if the Action is in this Tickable's action list.
     */
    bool HasAction(std::shared_ptr<Action> action) const;

    /** @brief Returns the number of registered Actions. */
    size_t CountActions() const;

    /**
     * @brief Registers an Action with this Tickable.
     * @param action The Action to add.
     * @param force If true, bypass the CanAddAction() check.
     * @return True if the Action was successfully added.
     */
    bool AddAction(std::shared_ptr<Action> action, const bool force = false);

    /**
     * @brief Unregisters an Action from this Tickable.
     * @param action The Action to remove.
     * @param force If true, bypass the CanRemoveAction() check.
     * @return True if the Action was successfully removed.
     */
    bool RemoveAction(std::shared_ptr<Action> action, const bool force = false);

    /** @brief Returns a snapshot of all registered Actions. */
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions() const;

    /**
     * @brief Returns all Actions that can be dynamic_cast to type T.
     * @tparam T The Action subtype to filter by.
     */
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions() const;

    /**
     * @brief Returns all Actions whose type matches one of the given action types.
     * @param actionTypes The action type IDs to include.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions(const std::vector<uint32_t>& actionTypes) const;

    /**
     * @brief Returns Actions matching both type T and one of the given action types.
     * @tparam T The Action subtype to filter by.
     * @param actionTypes The action type IDs to include.
     */
    template <typename T>
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions(const std::vector<uint32_t>& actionTypes) const;

    /**
     * @brief Returns all Actions whose type matches the given action type.
     * @param actionType The action type ID to filter by.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions(const uint32_t actionType) const;

    /**
     * @brief Returns Actions matching both type T and the given action type.
     * @tparam T The Action subtype to filter by.
     * @param actionType The action type ID to filter by.
     */
    template <typename T>
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions(const uint32_t actionType) const;

    /**
     * @brief Permission hook; override to prevent adding an Action.
     * @param action The Action about to be added.
     * @return True if the addition is permitted.
     */
    virtual bool CanAddAction(std::shared_ptr<Action> action);

    /**
     * @brief Permission hook; override to prevent removing an Action.
     * @param action The Action about to be removed.
     * @return True if the removal is permitted.
     */
    virtual bool CanRemoveAction(std::shared_ptr<Action> action);

    /**
     * @brief Notification hook called after an Action has been added.
     * @param action The Action that was added.
     * @param forced Whether the addition bypassed permission checks.
     */
    virtual void AddedAction(std::shared_ptr<Action> action, const bool forced);

    /**
     * @brief Notification hook called after an Action has been removed.
     * @param action The Action that was removed.
     * @param forced Whether the removal bypassed permission checks.
     */
    virtual void RemovedAction(std::shared_ptr<Action> action, const bool forced);

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

template <typename T> double Tickable::Tick(const double durationSinceLastTick) {
    if (!mIsTicking) {
        return 0.0;
    }
#ifdef INCLUDE_PROFILING
    const auto start = std::chrono::steady_clock::now();
#endif
    auto actions = GetActions<T>();
    for (const auto& action : *actions) {
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
double Tickable::Tick(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes) {
    if (!mIsTicking) {
        return 0.0;
    }
#ifdef INCLUDE_PROFILING
    const auto start = std::chrono::steady_clock::now();
#endif
    auto allActions = GetActions(actionTypes);
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

template <typename T> double Tickable::Tick(const double durationSinceLastTick, const uint32_t actionType) {
    if (!mIsTicking) {
        return 0.0;
    }
#ifdef INCLUDE_PROFILING
    const auto start = std::chrono::steady_clock::now();
#endif
    auto allActions = GetActions(actionType);
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

template <typename T> std::shared_ptr<std::vector<std::shared_ptr<Action>>> Tickable::GetActions() const {
    const std::lock_guard<std::mutex> lock(mMutex);
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    for (const auto& action : mActions.GetList()) {
        if (std::dynamic_pointer_cast<T>(action)) {
            result->push_back(action);
        }
    }
    return result;
}

template <typename T>
std::shared_ptr<std::vector<std::shared_ptr<Action>>>
Tickable::GetActions(const std::vector<uint32_t>& actionTypes) const {
    auto filtered = GetActions(actionTypes);
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    for (const auto& action : *filtered) {
        if (std::dynamic_pointer_cast<T>(action)) {
            result->push_back(action);
        }
    }
    return result;
}

template <typename T>
std::shared_ptr<std::vector<std::shared_ptr<Action>>> Tickable::GetActions(const uint32_t actionType) const {
    auto filtered = GetActions(actionType);
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    for (const auto& action : *filtered) {
        if (std::dynamic_pointer_cast<T>(action)) {
            result->push_back(action);
        }
    }
    return result;
}

} // namespace Ship
