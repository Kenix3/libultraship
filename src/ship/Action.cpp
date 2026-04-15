#include "ship/Action.h"

namespace Ship {

Action::Action(const uint32_t actionType, std::shared_ptr<Tickable> tickable)
    : Part(), mActionType(actionType), mTickable(tickable), mIsActionRunning(false), mClocks() {
}

uint32_t Action::GetType() const {
    return mActionType;
}

std::shared_ptr<Tickable> Action::GetTickable() const {
    return mTickable;
}

bool Action::Run(const double durationSinceLastTick) {
    SetClock(ClockType::PreviousStart, GetClock(ClockType::Start));
    SetClock(ClockType::PreviousEnd, GetClock(ClockType::End));
    SetClock(ClockType::Start, std::chrono::steady_clock::now());
    SetClock(ClockType::End, {});

    bool result = false;
    if (IsRunning()) {
        result = ActionRan(durationSinceLastTick);
    }

    SetClock(ClockType::End, std::chrono::steady_clock::now());
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

void Action::Started(const bool forced) {}

void Action::Stopped(const bool forced) {}

double Action::GetTime(const ClockType clockType) const {
    return std::chrono::duration<double>(GetClock(clockType).time_since_epoch()).count();
}

std::chrono::time_point<std::chrono::steady_clock> Action::GetClock(const ClockType clockType) const {
    return mClocks[static_cast<size_t>(clockType)];
}

Action& Action::SetClock(const ClockType clockType,
                         std::chrono::time_point<std::chrono::steady_clock> clockValue) {
    mClocks[static_cast<size_t>(clockType)] = clockValue;
    return *this;
}

} // namespace Ship
