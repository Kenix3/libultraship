#include "ship/Tickable.h"
#include "ship/Component.h"

#include "spdlog/spdlog.h"

namespace Ship {
Tickable::Tickable(const bool isTicking, const bool isDrawing)
    : mActions(false)
#ifdef INCLUDE_MUTEX
      ,
      mMutex()
#endif
#ifdef INCLUDE_PROFILING
      ,
      mClocks()
#endif
{
    if (isTicking) {
        StartAction(ActionType::Tick);
    }

    if (isDrawing) {
        StartAction(ActionType::Draw);
    }
}

Tickable::Tickable(const std::vector<bool>& actions)
    : mActions(false)
#ifdef INCLUDE_MUTEX
,     mMutex()
#endif
#ifdef INCLUDE_PROFILING
      ,
      mClocks()
#endif
{
    auto actionCount = actions.size();
    for (size_t i = 0; i < actionCount; i++) {
        StartAction(static_cast<ActionType>(i));
    }
}

Tickable::~Tickable() = default;

bool Tickable::RunAction(const ActionType action, const double durationSinceLastTick) {
#ifdef INCLUDE_PROFILING
    // Set Previous clocks to the current clock.
    SetClock(ClockType::PreviousTickStart, GetClock(ClockType::TickStart));
    SetClock(ClockType::PreviousTickEnd, GetClock(ClockType::TickEnd));

    // Set Start clock to now, and end clocks to "empty"
    // Then run the logic. After the logic, set the end time.
    SetClock(ClockType::TickStart, std::chrono::high_resolution_clock::now());
    SetClock(ClockType::TickEnd, std::chrono::time_point<std::chrono::steady_clock>(std::chrono::system_clock::duration::zero()));
#endif

    bool val = true;
    if (IsActionRunning(action)) {
        val = ActionRan(action, durationSinceLastTick);
    }

#ifdef INCLUDE_PROFILING
    SetClock(ClockType::TickEnd, std::chrono::high_resolution_clock::now());
#endif

    return val;
}

bool Tickable::IsActionRunning(const ActionType action) {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    return mActions[action];
}

bool Tickable::StartAction(const ActionType action, const bool force) {
    if (IsActionRunning(action)) {
        return true;
    }

    const bool canStartAction = CanStartAction(action);
    if (!canStartAction && !force) {
        return false;
    }

    const bool forced = !canStartAction && force;
    {
#ifdef INCLUDE_MUTEX
        const std::lock_guard<std::mutex> lock(mMutex);
#endif
        mActions[action] = true;
    }
    ActionStarted(action, forced);

    if (forced) {
        // If this is also a component, log out.
        auto component = dynamic_cast<Component*>(this);
        if (component != nullptr) {
            SPDLOG_WARN("Forcing Action {} Start on Component {}", static_cast<uint32_t>(action), component->ToString());
        }
    }

    return true;
}

bool Tickable::StopAction(const ActionType action, const bool force) {
    if (!IsActionRunning(action)) {
        return true;
    }

    const bool canStopAction = CanStopAction(action);
    if (!canStopAction && !force) {
        return false;
    }

    const bool forced = !canStopAction && force;
    {
#ifdef INCLUDE_MUTEX
        const std::lock_guard<std::mutex> lock(mMutex);
#endif
        mActions[action] = false;
    }
    ActionStopped(action, forced);

    if (forced) {
        // If this is also a component, log out.
        auto component = dynamic_cast<Component*>(this);
        if (component != nullptr) {
            SPDLOG_WARN("Forcing Action {} Stop on Component {}", static_cast<uint32_t>(action), component->ToString());
        }
    }

    return true;
}


#ifdef INCLUDE_PROFILING
double Tickable::GetTime(const ClockType clockType) const {
    return std::chrono::duration<double>(GetClock(clockType).time_since_epoch()).count();
}
#endif

bool Tickable::ActionRan(const ActionType action, const double durationSinceLastTick) {
    return true;
}


bool Tickable::CanStartAction(const ActionType action) {
    return true;
}

bool Tickable::CanStopAction(const ActionType action) {
    return true;
}

void Tickable::ActionStarted(const ActionType action, const bool forced) {

}

void Tickable::ActionStopped(const ActionType action, const bool forced) {

}

#ifdef INCLUDE_PROFILING
std::chrono::time_point<std::chrono::steady_clock> Tickable::GetClock(const ClockType clockType) const {
    return mClocks[static_cast<unsigned long long>(clockType)];
}

Tickable& Tickable::SetClock(const ClockType clockType, std::chrono::time_point<std::chrono::steady_clock> clockValue) {
    mClocks[static_cast<unsigned long long>(clockType)] = clockValue;
    return *this;
}
#endif
} // namespace Ship
