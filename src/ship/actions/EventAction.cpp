#include "ship/actions/EventAction.h"
#include "ship/core/TickableComponent.h"

#include <utility>

namespace Ship {

EventAction::EventAction(EventID eventId, std::shared_ptr<Tickable> tickable)
    : Action(tickable), mEventId(eventId), mCallback(std::monostate{}), mCallbackPointerData(0) {
}

EventAction::EventAction(EventID eventId, std::shared_ptr<Tickable> tickable, EventActionCppCallback callback,
                         uintptr_t callbackPointerData)
    : Action(tickable), mEventId(eventId), mCallback(std::monostate{}), mCallbackPointerData(0) {
    SetCallback(std::move(callback), callbackPointerData);
}

EventAction::EventAction(EventID eventId, std::shared_ptr<Tickable> tickable, uintptr_t callback,
                         uintptr_t callbackPointerData)
    : Action(tickable), mEventId(eventId), mCallback(std::monostate{}), mCallbackPointerData(0) {
    SetCallback(callback, callbackPointerData);
}

EventID EventAction::GetEventId() const {
    return mEventId;
}

bool EventAction::HasCallback() const {
    return !std::holds_alternative<std::monostate>(mCallback);
}

bool EventAction::GetHasCppCallback() const {
    return std::holds_alternative<EventActionCppCallback>(mCallback);
}

bool EventAction::GetHasRawCallback() const {
    return std::holds_alternative<uintptr_t>(mCallback) && std::get<uintptr_t>(mCallback) != 0;
}

const EventActionCallback& EventAction::GetCallback() const {
    return mCallback;
}

EventAction& EventAction::SetCallback(const EventActionCallback& callback) {
    return SetCallback(callback, 0);
}

EventAction& EventAction::SetCallback(const EventActionCallback& callback, uintptr_t callbackPointerData) {
    if (std::holds_alternative<std::monostate>(callback)) {
        return ClearCallback();
    }

    if (std::holds_alternative<EventActionCppCallback>(callback)) {
        return SetCallback(std::get<EventActionCppCallback>(callback), callbackPointerData);
    }

    return SetCallback(std::get<uintptr_t>(callback), callbackPointerData);
}

EventAction& EventAction::SetCallback(EventActionCppCallback callback) {
    return SetCallback(std::move(callback), 0);
}

EventAction& EventAction::SetCallback(EventActionCppCallback callback, uintptr_t callbackPointerData) {
    if (callback) {
        mCallback = std::move(callback);
        mCallbackPointerData = callbackPointerData;
    } else {
        mCallback = std::monostate{};
        mCallbackPointerData = 0;
    }

    return *this;
}

EventAction& EventAction::SetCallback(uintptr_t callback) {
    return SetCallback(callback, 0);
}

EventAction& EventAction::SetCallback(uintptr_t callback, uintptr_t callbackPointerData) {
    if (callback != 0) {
        mCallback = callback;
        mCallbackPointerData = callbackPointerData;
    } else {
        mCallback = std::monostate{};
        mCallbackPointerData = 0;
    }

    return *this;
}

EventAction& EventAction::ClearCallback() {
    mCallback = std::monostate{};
    mCallbackPointerData = 0;
    return *this;
}

uintptr_t EventAction::GetCallbackPointerData() const {
    return mCallbackPointerData;
}

EventAction& EventAction::SetCallbackPointerData(uintptr_t callbackPointerData) {
    mCallbackPointerData = callbackPointerData;
    return *this;
}

bool EventAction::ActionRan(const double durationSinceLastTick) {
    if (std::holds_alternative<EventActionCppCallback>(mCallback)) {
        return std::get<EventActionCppCallback>(mCallback)(mEventId, durationSinceLastTick, mCallbackPointerData);
    }

    if (std::holds_alternative<uintptr_t>(mCallback)) {
        const auto callbackAddr = std::get<uintptr_t>(mCallback);
        if (callbackAddr != 0) {
            const auto callback = reinterpret_cast<EventActionRawCallback>(callbackAddr);
            return callback(mEventId, durationSinceLastTick, mCallbackPointerData);
        }
    }

    auto tickable = GetTickable();
    if (auto* tc = dynamic_cast<TickableComponent*>(tickable.get())) {
        return tc->ActionRan(mEventId, durationSinceLastTick);
    }
    return true;
}

} // namespace Ship
