#pragma once

#include "ship/Action.h"

namespace Ship {

/**
 * @brief Generic Action that delegates to a TickableComponent for any event type.
 *
 * EventAction replaces the former TickAction, DrawAction, and DrawDebugMenuAction
 * classes. It delegates execution to the owning TickableComponent's ActionRan()
 * virtual, passing the event name so the component can dispatch accordingly.
 *
 * Event names (e.g. "Tick", "Draw", "DrawDebugMenu") are registered dynamically
 * with the Events component rather than being hardcoded enum values.
 */
class EventAction : public Action {
  public:
    /**
     * @brief Constructs an EventAction for the given event name.
     * @param eventName The event name this action handles (e.g. "Tick", "Draw").
     * @param tickable  The Tickable that owns this Action.
     */
    EventAction(const std::string& eventName, std::shared_ptr<Tickable> tickable);
    virtual ~EventAction() = default;

  protected:
    /**
     * @brief Delegates to TickableComponent::ActionRan() with the event name.
     * @param durationSinceLastTick Elapsed time in seconds since the last tick.
     * @return True if the action executed successfully.
     */
    bool ActionRan(const double durationSinceLastTick) override;
};

} // namespace Ship
