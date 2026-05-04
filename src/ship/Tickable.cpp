#include "ship/Tickable.h"
#include "ship/Component.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace Ship {

Tickable::Tickable(const bool isTicking)
    : mIsTicking(isTicking), mActions()
#ifdef COMPONENT_THREAD_SAFE
      ,
      mMutex()
#endif
#ifdef INCLUDE_PROFILING
      ,
      mClocks()
#endif
{
}

Tickable::Tickable(const bool isTicking, const std::vector<std::shared_ptr<Action>>& actions)
    : mIsTicking(isTicking), mActions()
#ifdef COMPONENT_THREAD_SAFE
      ,
      mMutex()
#endif
#ifdef INCLUDE_PROFILING
      ,
      mClocks()
#endif
{
    for (const auto& action : actions) {
        mActions.Add(action);
    }
}

Tickable::~Tickable() = default;

bool Tickable::IsTicking() const {
    return mIsTicking;
}

bool Tickable::Start(const bool force) {
    if (mIsTicking) {
        return true;
    }
    const bool canStart = CanStart();
    if (!canStart && !force) {
        return false;
    }
    const bool forced = !canStart && force;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::mutex> lock(mMutex);
#endif
        mIsTicking = true;
    }
    Started(forced);
    if (forced) {
        auto component = dynamic_cast<Component*>(this);
        if (component != nullptr) {
            SPDLOG_WARN("Forcing Start on Component {}", component->ToString());
        }
    }
    return true;
}

bool Tickable::Stop(const bool force) {
    if (!mIsTicking) {
        return true;
    }
    const bool canStop = CanStop();
    if (!canStop && !force) {
        return false;
    }
    const bool forced = !canStop && force;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::mutex> lock(mMutex);
#endif
        mIsTicking = false;
    }
    Stopped(forced);
    if (forced) {
        auto component = dynamic_cast<Component*>(this);
        if (component != nullptr) {
            SPDLOG_WARN("Forcing Stop on Component {}", component->ToString());
        }
    }
    return true;
}

double Tickable::Run(const double durationSinceLastTick) {
#ifdef INCLUDE_PROFILING
    SetClock(ClockType::PreviousStart, GetClock(ClockType::Start));
    SetClock(ClockType::PreviousEnd, GetClock(ClockType::End));
    SetClock(ClockType::Start, std::chrono::steady_clock::now());
    SetClock(ClockType::End, {});
#endif

    if (!mIsTicking) {
        return 0.0;
    }

    auto actions = mActions.Get();
    std::stable_sort(
        actions->begin(), actions->end(),
        [](const std::shared_ptr<Action>& a, const std::shared_ptr<Action>& b) { return a->GetEventId() < b->GetEventId(); });
    for (const auto& action : *actions) {
        action->Run(durationSinceLastTick);
    }

#ifdef INCLUDE_PROFILING
    SetClock(ClockType::End, std::chrono::steady_clock::now());
    return GetTime(ClockType::End) - GetTime(ClockType::Start);
#else
    return 0.0;
#endif
}

double Tickable::Run(const double durationSinceLastTick, const std::vector<EventID>& eventIds) {
    if (!mIsTicking) {
        return 0.0;
    }
#ifdef INCLUDE_PROFILING
    const auto start = std::chrono::steady_clock::now();
#endif
    auto actions = mActions.Get(eventIds);
    std::stable_sort(
        actions->begin(), actions->end(),
        [](const std::shared_ptr<Action>& a, const std::shared_ptr<Action>& b) { return a->GetEventId() < b->GetEventId(); });
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

double Tickable::Run(const double durationSinceLastTick, EventID eventId) {
    if (!mIsTicking) {
        return 0.0;
    }
#ifdef INCLUDE_PROFILING
    const auto start = std::chrono::steady_clock::now();
#endif
    auto actions = mActions.Get(eventId);
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

ActionList& Tickable::GetActionList() {
    return mActions;
}

const ActionList& Tickable::GetActionList() const {
    return mActions;
}

bool Tickable::CanStart() {
    return true;
}

bool Tickable::CanStop() {
    return true;
}

void Tickable::Started(const bool forced) {
}

void Tickable::Stopped(const bool forced) {
}

#ifdef INCLUDE_PROFILING
double Tickable::GetTime(const ClockType clockType) const {
    return std::chrono::duration<double>(GetClock(clockType).time_since_epoch()).count();
}

#ifdef COMPONENT_THREAD_SAFE
std::mutex& Tickable::GetMutex() {
    return mMutex;
}
#endif

std::chrono::time_point<std::chrono::steady_clock> Tickable::GetClock(const ClockType clockType) const {
    return mClocks[static_cast<size_t>(clockType)];
}

Tickable& Tickable::SetClock(const ClockType clockType, std::chrono::time_point<std::chrono::steady_clock> clockValue) {
    mClocks[static_cast<size_t>(clockType)] = clockValue;
    return *this;
}
#else
#ifdef COMPONENT_THREAD_SAFE
std::mutex& Tickable::GetMutex() {
    return mMutex;
}
#endif
#endif

} // namespace Ship
