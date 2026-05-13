#include "ship/Action.h"

#include <functional>

namespace Ship {

Action::Action(EventID eventId, std::shared_ptr<Tickable> tickable)
    : Part(), mEventId(eventId), mTickable(tickable), mIsActionRunning(false)
#ifdef INCLUDE_PROFILING
      ,
      mClocks()
#endif
{
}

EventID Action::GetEventId() const {
    return mEventId;
}

std::shared_ptr<Tickable> Action::GetTickable() const {
    return mTickable.lock();
}

bool Action::Run(const double durationSinceLastTick) {
#ifdef INCLUDE_PROFILING
    SetClock(ClockType::PreviousStart, GetClock(ClockType::Start));
    SetClock(ClockType::PreviousEnd, GetClock(ClockType::End));
    SetClock(ClockType::Start, std::chrono::steady_clock::now());
    SetClock(ClockType::End, {});
#endif

    bool result = false;
    if (IsRunning()) {
        result = ActionRan(durationSinceLastTick);
    }

#ifdef INCLUDE_PROFILING
    SetClock(ClockType::End, std::chrono::steady_clock::now());
#endif
    return result;
}

bool Action::IsRunning() const {
    return mIsActionRunning;
}

bool Action::Start(const bool force) {
    if (IsRunning()) {
        return true;
    }
    const bool canStart = CanStart();
    if (!canStart && !force) {
        return false;
    }
    const bool forced = !canStart && force;
    mIsActionRunning = true;
    Started(forced);
    return true;
}

bool Action::Stop(const bool force) {
    if (!IsRunning()) {
        return true;
    }
    const bool canStop = CanStop();
    if (!canStop && !force) {
        return false;
    }
    const bool forced = !canStop && force;
    mIsActionRunning = false;
    Stopped(forced);
    return true;
}

bool Action::CanStart() {
    return true;
}

bool Action::CanStop() {
    return true;
}

void Action::Started(const bool forced) {
}

void Action::Stopped(const bool forced) {
}

#ifdef INCLUDE_PROFILING
double Action::GetTime(const ClockType clockType) const {
    return std::chrono::duration<double>(GetClock(clockType).time_since_epoch()).count();
}

std::chrono::time_point<std::chrono::steady_clock> Action::GetClock(const ClockType clockType) const {
    return mClocks[static_cast<size_t>(clockType)];
}

Action& Action::SetClock(const ClockType clockType, std::chrono::time_point<std::chrono::steady_clock> clockValue) {
    mClocks[static_cast<size_t>(clockType)] = clockValue;
    return *this;
}
#endif

} // namespace Ship
