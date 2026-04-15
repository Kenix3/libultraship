#include "ship/Tickable.h"
#include "ship/Component.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace Ship {

Tickable::Tickable(const bool isTicking)
    : mIsTicking(isTicking), mActions(), mMutex(), mClocks() {}

Tickable::Tickable(const bool isTicking, const std::vector<std::shared_ptr<Action>>& actions)
    : mIsTicking(isTicking), mActions(), mMutex(), mClocks() {
    for (const auto& action : actions) {
        AddAction(action);
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
        const std::lock_guard<std::mutex> lock(mMutex);
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
        const std::lock_guard<std::mutex> lock(mMutex);
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

double Tickable::Tick(const double durationSinceLastTick) {
    SetClock(ClockType::PreviousStart, GetClock(ClockType::Start));
    SetClock(ClockType::PreviousEnd, GetClock(ClockType::End));
    SetClock(ClockType::Start, std::chrono::steady_clock::now());
    SetClock(ClockType::End, {});

    if (!mIsTicking) {
        return 0.0;
    }

    auto actions = GetActions();
    std::stable_sort(actions->begin(), actions->end(),
        [](const std::shared_ptr<Action>& a, const std::shared_ptr<Action>& b) {
            return a->GetType() < b->GetType();
        });
    for (const auto& action : *actions) {
        action->Run(durationSinceLastTick);
    }

    SetClock(ClockType::End, std::chrono::steady_clock::now());
    return GetTime(ClockType::End) - GetTime(ClockType::Start);
}

double Tickable::Tick(const double durationSinceLastTick, const std::vector<uint32_t>& actionTypes) {
    if (!mIsTicking) {
        return 0.0;
    }
    auto actions = GetActions(actionTypes);
    std::stable_sort(actions->begin(), actions->end(),
        [](const std::shared_ptr<Action>& a, const std::shared_ptr<Action>& b) {
            return a->GetType() < b->GetType();
        });
    for (const auto& action : *actions) {
        action->Run(durationSinceLastTick);
    }
    return 0.0;
}

double Tickable::Tick(const double durationSinceLastTick, const uint32_t actionType) {
    if (!mIsTicking) {
        return 0.0;
    }
    auto actions = GetActions(actionType);
    for (const auto& action : *actions) {
        action->Run(durationSinceLastTick);
    }
    return 0.0;
}

bool Tickable::HasAction(std::shared_ptr<Action> action) const {
    const std::lock_guard<std::mutex> lock(mMutex);
    return mActions.Has(action);
}

size_t Tickable::CountActions() const {
    const std::lock_guard<std::mutex> lock(mMutex);
    return mActions.GetCount();
}

bool Tickable::AddAction(std::shared_ptr<Action> action, const bool force) {
    if (!action) {
        return false;
    }
    const bool canAdd = CanAddAction(action);
    if (!canAdd && !force) {
        return false;
    }
    const bool forced = !canAdd && force;
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        if (mActions.Has(action)) {
            return true;
        }
        const ListReturnCode result = mActions.Add(action);
        if (static_cast<int32_t>(result) < 0) {
            return false;
        }
    }
    action->Start();
    AddedAction(action, forced);
    if (forced) {
        auto component = dynamic_cast<Component*>(this);
        if (component != nullptr) {
            SPDLOG_WARN("Forcing AddAction {} on Component {}", action->GetId(), component->ToString());
        }
    }
    return true;
}

bool Tickable::RemoveAction(std::shared_ptr<Action> action, const bool force) {
    if (!action) {
        return false;
    }
    const bool canRemove = CanRemoveAction(action);
    if (!canRemove && !force) {
        return false;
    }
    const bool forced = !canRemove && force;
    {
        const std::lock_guard<std::mutex> lock(mMutex);
        const ListReturnCode result = mActions.Remove(action);
        if (result == ListReturnCode::NotFound) {
            return true;
        }
        if (static_cast<int32_t>(result) < 0) {
            return false;
        }
    }
    action->Stop();
    RemovedAction(action, forced);
    if (forced) {
        auto component = dynamic_cast<Component*>(this);
        if (component != nullptr) {
            SPDLOG_WARN("Forcing RemoveAction {} on Component {}", action->GetId(),
                        component->ToString());
        }
    }
    return true;
}

std::shared_ptr<std::vector<std::shared_ptr<Action>>> Tickable::GetActions() const {
    const std::lock_guard<std::mutex> lock(mMutex);
    return mActions.Get();
}

std::shared_ptr<std::vector<std::shared_ptr<Action>>>
Tickable::GetActions(const std::vector<uint32_t>& actionTypes) const {
    const std::lock_guard<std::mutex> lock(mMutex);
    return mActions.Get(actionTypes);
}

std::shared_ptr<std::vector<std::shared_ptr<Action>>>
Tickable::GetActions(const uint32_t actionType) const {
    const std::lock_guard<std::mutex> lock(mMutex);
    return mActions.Get(actionType);
}

bool Tickable::CanAddAction(std::shared_ptr<Action> action) {
    return true;
}

bool Tickable::CanRemoveAction(std::shared_ptr<Action> action) {
    return true;
}

void Tickable::AddedAction(std::shared_ptr<Action> action, const bool forced) {}

void Tickable::RemovedAction(std::shared_ptr<Action> action, const bool forced) {}

bool Tickable::CanStart() {
    return true;
}

bool Tickable::CanStop() {
    return true;
}

void Tickable::Started(const bool forced) {}

void Tickable::Stopped(const bool forced) {}

double Tickable::GetTime(const ClockType clockType) const {
    return std::chrono::duration<double>(GetClock(clockType).time_since_epoch()).count();
}

std::mutex& Tickable::GetMutex() {
    return mMutex;
}

std::chrono::time_point<std::chrono::steady_clock> Tickable::GetClock(const ClockType clockType) const {
    return mClocks[static_cast<size_t>(clockType)];
}

Tickable& Tickable::SetClock(const ClockType clockType,
                              std::chrono::time_point<std::chrono::steady_clock> clockValue) {
    mClocks[static_cast<size_t>(clockType)] = clockValue;
    return *this;
}

} // namespace Ship

