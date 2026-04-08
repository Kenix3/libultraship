#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <stdint.h>
#incldue <stddef.h>

#define INCLUDE_PROFILING 1
#define INCLUDE_MUTEX 1

#ifdef INCLUDE_PROFILING
#include <chrono>
#endif

namespace Ship {
class Action;

#ifdef INCLUDE_PROFILING
// TODO: New Clock type. Should be on Action and Tickable
enum class ClockType : uint64_t { Start, End, PreviousStart, PreviousEnd, ClockMax };
#endif

enum class ActionType : size_t;

class Tickable : public std::enable_shared_from_this<Tickable> {
    // TODO: Basically all functions need to be reimplemented.
  public:
    Tickable(const bool isTicking = true);
    Tickable(const bool isTicking, const std::vector<std::shared_ptr<Action>>& actions);
    // TODO: Destructor needs to RemoveActions first like Components
    ~Tickable();

    bool IsTicking();
    bool Start(const bool force = false);
    bool Stop(const bool force = false);

    // Should sort by action type, low to high.
    double Tick(const double durationSinceLastTick);
    template <typename T> double Tick(const double durationSinceLastTick);
    double Tick(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes);
    template <typename T> double Tick(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes);
    double Tick(const double durationSinceLastTick, uint32_t actionType);
    template <typename T> double Tick(const double durationSinceLastTick, const uint32_t actionType);

    // TODO: All of this action stuff is really just an ID'd thread safe list. This could probably be common between Context (tickablecomponents), Tickable (actions), and Component (components)
    // I need a searchable container class that has all of these functions and maybe some more. It can be sub-classable to search.
    // Maybe move all of this new LUS stuff to a new core folder?
    // We probably need to make all Tickables also be Components.
    // There should also be an enabled class in addition to the "has identifier" and "has list" class.
    // TODO: Also fix the crash on close. It's due to spdlog being destructed before all of the Context is done.
    bool HasAction(std::shared_ptr<Action> action);
    size_t CountActions();
    bool AddAction(std::shared_ptr<Action> action, const bool force = false);
    bool RemoveAction(std::shared_ptr<Action> action, const bool force = false);
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions();
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions();
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions(const std::vector<uint32_t>& actionTypes);
    template <typename T>
    std::shared_ptr<std::vector<std::shared_ptr<Action>>>
    GetActions(const std::vector<uint32_t>& actionTypes);
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions(const uint32_t actionType);
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<Action>>> GetActions(const uint32_t actionType);

    bool CanAddAction(std::shared_ptr<Action> action);
    bool CanRemoveAction(std::shared_ptr<Action> action);
    void AddedAction(std::shared_ptr<Action> action, const bool forced);
    void RemovedAction(std::shared_ptr<Action> action, const bool forced);

#ifdef INCLUDE_PROFILING
    double GetTime(const ClockType clockType) const;
#endif

protected:
    std::mutex& GetMutex();

private:
#ifdef INCLUDE_PROFILING
    Tickable& SetClock(const ClockType clockType, std::chrono::time_point<std::chrono::steady_clock> clockValue);
#endif

    std::vector<std::shared_ptr<Action>> mActions;
    bool mIsTicking;

#ifdef INCLUDE_MUTEX
    std::mutex mMutex;
#endif
#ifdef INCLUDE_PROFILING
    std::chrono::time_point<std::chrono::steady_clock> mClocks[static_cast<unsigned long long>(ClockType::ClockMax)];
#endif
};

} // namespace Ship
