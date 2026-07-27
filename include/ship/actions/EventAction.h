#pragma once

#include <functional>
#include <stdint.h>
#include <variant>

#include "ship/core/Action.h"
#include "ship/events/EventTypes.h"

namespace Ship {

/**
 * @brief C++ callback signature for EventAction dispatch.
 *
 * @param eventId Event being dispatched.
 * @param callbackPointerData Opaque pointer-sized data supplied by EventAction.
 * @return True when callback handled successfully.
 */
using EventActionCppCallback = std::function<bool(EventID, uintptr_t)>;

/**
 * @brief C-style callback signature for EventAction dispatch.
 *
 * Stored in EventAction as uintptr_t and cast at dispatch time.
 */
using EventActionRawCallback = bool (*)(EventID, uintptr_t);

/**
 * @brief Stored callback target for EventAction.
 *
 * - std::monostate: no callback override (fallback to TickableComponent dispatch)
 * - EventActionCppCallback: C++ callback override
 * - uintptr_t: C-style function pointer override
 */
using EventActionCallback = std::variant<std::monostate, EventActionCppCallback, uintptr_t>;

/**
 * @brief Action specialization for event-driven dispatch.
 *
 * EventAction pairs an Action with an EventID. Dispatch order:
 * 1) callback override in GetCallback(),
 * 2) fallback TickableComponent::ActionRan(eventId).
 *
 * Callback pointer data is shared by both callback forms.
 */
class EventAction : public Action {
  public:
    /**
     * @brief Constructs an EventAction for the given EventID.
     * @param eventId The EventID this action handles.
     * @param tickable The Tickable that owns this Action.
     */
    EventAction(EventID eventId, std::shared_ptr<Tickable> tickable);

    /**
     * @brief Constructs an EventAction with a C++ callback override.
     * @param eventId The EventID this action handles.
     * @param tickable The Tickable that owns this Action.
     * @param callback Callback invoked directly from ActionRan().
     * @param callbackPointerData Opaque pointer-sized data passed to callback.
     */
    EventAction(EventID eventId, std::shared_ptr<Tickable> tickable, EventActionCppCallback callback,
                uintptr_t callbackPointerData = 0);

    /**
     * @brief Constructs an EventAction with a C-style callback override.
     * @param eventId The EventID this action handles.
     * @param tickable The Tickable that owns this Action.
     * @param callback Function pointer stored as uintptr_t.
     * @param callbackPointerData Opaque pointer-sized data passed to callback.
     */
    EventAction(EventID eventId, std::shared_ptr<Tickable> tickable, uintptr_t callback,
                uintptr_t callbackPointerData = 0);

    virtual ~EventAction() = default;

    /** @brief Returns the EventID this action corresponds to. */
    EventID GetEventId() const;

    /** @brief Returns true when a callback override is configured. */
    bool HasCallback() const;

    /** @brief Returns true when callback target is the C++ callback form. */
    bool GetHasCppCallback() const;

    /** @brief Returns true when callback target is the raw function-pointer form. */
    bool GetHasRawCallback() const;

    /**
     * @brief Returns the currently configured callback target.
     *
     * Use std::holds_alternative<> to inspect type.
     */
    const EventActionCallback& GetCallback() const;

    /**
     * @brief Configures callback override from callback variant.
     * @param callback Variant callback target.
     * @return This EventAction.
     */
    EventAction& SetCallback(const EventActionCallback& callback);

    /**
     * @brief Configures callback override from callback variant.
     * @param callback Variant callback target.
     * @param callbackPointerData Opaque pointer-sized data passed on invocation.
     * @return This EventAction.
     */
    EventAction& SetCallback(const EventActionCallback& callback, uintptr_t callbackPointerData);

    /**
     * @brief Configures a C++ callback override.
     * @param callback Callback target.
     * @return This EventAction.
     */
    EventAction& SetCallback(EventActionCppCallback callback);

    /**
     * @brief Configures a C++ callback override.
     * @param callback Callback target.
     * @param callbackPointerData Opaque pointer-sized data passed on invocation.
     * @return This EventAction.
     */
    EventAction& SetCallback(EventActionCppCallback callback, uintptr_t callbackPointerData);

    /**
     * @brief Configures a C-style callback override.
     * @param callback Function pointer stored as uintptr_t.
     * @return This EventAction.
     */
    EventAction& SetCallback(uintptr_t callback);

    /**
     * @brief Configures a C-style callback override.
     * @param callback Function pointer stored as uintptr_t.
     * @param callbackPointerData Opaque pointer-sized data passed on invocation.
     * @return This EventAction.
     */
    EventAction& SetCallback(uintptr_t callback, uintptr_t callbackPointerData);

    /**
     * @brief Clears callback override and restores fallback dispatch.
     * @return This EventAction.
     */
    EventAction& ClearCallback();

    /** @brief Returns the callback pointer-sized data used by both callback forms. */
    uintptr_t GetCallbackPointerData() const;

    /**
     * @brief Updates callback pointer-sized data without changing callback target.
     * @param callbackPointerData Opaque pointer-sized data.
     * @return This EventAction.
     */
    EventAction& SetCallbackPointerData(uintptr_t callbackPointerData);

  protected:
    /**
     * @brief Dispatches callback override when present, else delegates to TickableComponent::ActionRan().
     * @return True if the action executed successfully.
     */
    bool ActionRan() override;

  private:
    EventID mEventId;
    EventActionCallback mCallback;
    uintptr_t mCallbackPointerData;
};

} // namespace Ship
