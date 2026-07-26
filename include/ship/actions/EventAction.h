#pragma once

#include "ship/core/Action.h"
#include "ship/events/EventTypes.h"

namespace Ship {

/**
 * @brief Action specialization for event-driven dispatch.
 *
 * EventAction pairs an Action with an EventID, enabling dynamic dispatch
 * to TickableComponent::ActionRan(eventId, dt). This separates event-driven
 * behavior from the base Action abstraction.
 *
 * EventIDs are registered dynamically with the Events component.
 */
class EventAction : public Action {
  public:
    /**
     * @brief Constructs an EventAction for the given EventID.
     * @param eventId The EventID this action handles.
     * @param tickable The Tickable that owns this Action.
     */
    EventAction(EventID eventId, std::shared_ptr<Tickable> tickable);
    virtual ~EventAction() = default;

    /** @brief Returns the EventID this action corresponds to. */
    EventID GetEventId() const;

  protected:
    /**
     * @brief Delegates to TickableComponent::ActionRan() with the EventID.
     * @param durationSinceLastTick Elapsed time in seconds since the last tick.
     * @return True if the action executed successfully.
     */
    bool ActionRan(const double durationSinceLastTick) override;

  private:
    EventID mEventId;
};

} // namespace Ship
