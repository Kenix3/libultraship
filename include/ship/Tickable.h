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

class Tickable : public std::enable_shared_from_this<Tickable> {
  public:
    Tickable(const bool isTicking = true);
    Tickable(const bool isTicking, const std::vector<std::shared_ptr<Action>>& actions);
    virtual ~Tickable();

    bool IsTicking() const;
    bool Start(const bool force = false);
    bool Stop(const bool force = false);

    // Runs all running Actions sorted by action type (low → high).
    double Tick(const double durationSinceLastTick);
    template <typename T> double Tick(const double durationSinceLastTick);
    double Tick(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes);
    template <typename T>
    double Tick(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes);
    double Tick(const double durationSinceLastTick, const uint32_t actionType);
    template <typename T> double Tick(const double durationSinceLastTick, const uint32_t actionType);

    bool HasAction(std::shared_ptr<Action> action) const;
    size_t CountActions() const;
    bool AddAction(std::shared_ptr<Action> action, const bool force = false);
    bool RemoveAction(std::shared_ptr<Action> action, const bool force = false);
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions() const;
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions() const;
    std::shared_ptr<std::vector<std::shared_ptr<Action>>>
    GetActions(const std::vector<uint32_t>& actionTypes) const;
    template <typename T>
    std::shared_ptr<std::vector<std::shared_ptr<Action>>>
    GetActions(const std::vector<uint32_t>& actionTypes) const;
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions(const uint32_t actionType) const;
    template <typename T>
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions(const uint32_t actionType) const;

    virtual bool CanAddAction(std::shared_ptr<Action> action);
    virtual bool CanRemoveAction(std::shared_ptr<Action> action);
    virtual void AddedAction(std::shared_ptr<Action> action, const bool forced);
    virtual void RemovedAction(std::shared_ptr<Action> action, const bool forced);

#ifdef INCLUDE_PROFILING
    double GetTime(const ClockType clockType) const;
#endif

  protected:
    virtual bool CanStart();
    virtual bool CanStop();
    virtual void Started(const bool forced);
    virtual void Stopped(const bool forced);

    std::mutex& GetMutex();

  private:
#ifdef INCLUDE_PROFILING
    Tickable& SetClock(const ClockType clockType,
                       std::chrono::time_point<std::chrono::steady_clock> clockValue);
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

template <typename T>
double Tickable::Tick(const double durationSinceLastTick) {
    auto actions = GetActions<T>();
    for (const auto& action : *actions) {
        action->Run(durationSinceLastTick);
    }
#ifdef INCLUDE_PROFILING
    return GetTime(ClockType::End) - GetTime(ClockType::Start);
#else
    return 0.0;
#endif
}

template <typename T>
double Tickable::Tick(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes) {
    auto allActions = GetActions(actionTypes);
    for (const auto& action : *allActions) {
        if (std::dynamic_pointer_cast<T>(action)) {
            action->Run(durationSinceLastTick);
        }
    }
#ifdef INCLUDE_PROFILING
    return GetTime(ClockType::End) - GetTime(ClockType::Start);
#else
    return 0.0;
#endif
}

template <typename T>
double Tickable::Tick(const double durationSinceLastTick, const uint32_t actionType) {
    auto allActions = GetActions(actionType);
    for (const auto& action : *allActions) {
        if (std::dynamic_pointer_cast<T>(action)) {
            action->Run(durationSinceLastTick);
        }
    }
#ifdef INCLUDE_PROFILING
    return GetTime(ClockType::End) - GetTime(ClockType::Start);
#else
    return 0.0;
#endif
}

template <typename T>
std::shared_ptr<std::vector<std::shared_ptr<Action>>> Tickable::GetActions() const {
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
std::shared_ptr<std::vector<std::shared_ptr<Action>>>
Tickable::GetActions(const uint32_t actionType) const {
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


