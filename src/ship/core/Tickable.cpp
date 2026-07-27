#include "ship/core/Tickable.h"
#include "ship/core/Component.h"

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
#ifdef COMPONENT_THREAD_SAFE
    return mIsTicking.load(std::memory_order_acquire);
#else
    return mIsTicking;
#endif
}

bool Tickable::Start(const bool force) {
#ifdef COMPONENT_THREAD_SAFE
    if (mIsTicking.load(std::memory_order_acquire)) {
#else
    if (mIsTicking) {
#endif
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
        mIsTicking.store(true, std::memory_order_release);
#else
        mIsTicking = true;
#endif
    }
    Started(forced);
    if (forced) {
        auto component = dynamic_cast<Component*>(this);
        if (component != nullptr) {
            SPDLOG_WARN("Forcing Start on Component {}", component->ToString());
        } else {
            SPDLOG_WARN("Forcing Start on unnamed Tickable");
        }
    }
    return true;
}

bool Tickable::Stop(const bool force) {
#ifdef COMPONENT_THREAD_SAFE
    if (!mIsTicking.load(std::memory_order_acquire)) {
#else
    if (!mIsTicking) {
#endif
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
        mIsTicking.store(false, std::memory_order_release);
#else
        mIsTicking = false;
#endif
    }
    Stopped(forced);
    if (forced) {
        auto component = dynamic_cast<Component*>(this);
        if (component != nullptr) {
            SPDLOG_WARN("Forcing Stop on Component {}", component->ToString());
        } else {
            SPDLOG_WARN("Forcing Stop on unnamed Tickable");
        }
    }
    return true;
}

bool Tickable::Tick(EventID eventId) {
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
    auto actions = mActions.Get(eventId);
    for (const auto& action : *actions) {
        ran = action->Run() || ran;
    }
#ifdef INCLUDE_PROFILING
    const auto end = std::chrono::steady_clock::now();
    (void)end;
    (void)start;
#endif
    return ran;
}

bool Tickable::Tick(const std::vector<EventID>& eventIds) {
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
    auto actions = mActions.Get(eventIds);
    for (const auto& action : *actions) {
        ran = action->Run() || ran;
    }
#ifdef INCLUDE_PROFILING
    const auto end = std::chrono::steady_clock::now();
    (void)end;
    (void)start;
#endif
    return ran;
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
