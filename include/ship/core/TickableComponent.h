#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

#include "ship/core/Tickable.h"
#include "ship/core/Component.h"
#include "ship/core/Action.h"
#include "ship/events/EventTypes.h"

namespace Ship {

class Context;

/** @brief Groups TickableComponents into logical tick categories. */
enum class TickGroup : uint32_t { TickGroupDefault = 0 };

/** @brief Controls execution order within a TickGroup (lower runs first). */
enum class TickPriority : uint32_t { TickPriorityDefault = 0 };

/**
 * @brief Combines Tickable and Component, auto-registering with a Context.
 *
 * TickableComponent is the primary building block for objects that need both a
 * name/hierarchy (Component) and per-frame tick/draw execution (Tickable).
 * After construction and shared_ptr ownership, call RegisterWithContext() to
 * wire the component into the Context's tick loop.
 *
 * Event names (e.g. "Tick", "Draw", "DrawDebugMenu") are registered dynamically
 * with the Events component rather than using hardcoded enum values. When
 * RegisterWithContext() is called, EventActions are created for each event name
 * the component subscribes to.
 *
 * **Context TickableList semantics:**
 * A TickableComponent is present in the Context's TickableList (and therefore
 * executed each frame) as long as it has at least one parent. It is added to
 * the list when its first parent is assigned and removed when its last parent
 * is removed.  This is managed automatically by ComponentList when the
 * COMPONENT_THREAD_SAFE or default parent/child relationship hooks fire.
 */
class TickableComponent : public Tickable,
                          public Component,
                          public std::enable_shared_from_this<TickableComponent> {
  public:
    /**
     * @brief Constructs a TickableComponent with EventID subscriptions.
     *
     * Call RegisterWithContext() after the object is owned by a shared_ptr.
     * @param name Human-readable name for the Component.
     * @param context The Context to register with.
     * @param tickGroup The TickGroup this component belongs to.
     * @param tickPriority Execution priority within the TickGroup.
     * @param eventIds List of EventIDs to create EventActions for.
     */
    TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                      const TickGroup tickGroup = TickGroup::TickGroupDefault,
                      const TickPriority tickPriority = TickPriority::TickPriorityDefault,
                      const std::vector<EventID>& eventIds = {});

    /**
     * @brief Constructs a TickableComponent with an explicit list of Actions.
     * @param name Human-readable name for the Component.
     * @param context The Context to register with.
     * @param tickGroup The TickGroup this component belongs to.
     * @param tickPriority Execution priority within the TickGroup.
     * @param actions The initial set of Actions to register.
     */
    TickableComponent(const std::string& name, std::shared_ptr<Context> context, const TickGroup tickGroup,
                      const TickPriority tickPriority, const std::vector<std::shared_ptr<Action>>& actions);
    virtual ~TickableComponent();

    /**
     * @brief Registers this component with its Context.
     *
     * Must be called after the component has shared ownership and is reachable
     * from the component hierarchy so self can be resolved internally.
     * Creates EventActions for all pending event names.
     * @return True on successful registration, false when self cannot be resolved.
     */
    bool RegisterWithContext();

    /** @brief Unregisters this component from its Context. */
    void UnregisterFromContext();

    /** @brief Returns the Context this component is registered with. */
    std::shared_ptr<Context> GetContext() const;

    /** @brief Returns the TickGroup this component belongs to. */
    TickGroup GetTickGroup() const;

    /** @brief Returns the tick priority within the TickGroup. */
    TickPriority GetTickPriority() const;

    /** @brief Returns a composite order value derived from TickGroup and TickPriority. */
    uint64_t GetOrder() const;

    /**
     * @brief Sets the TickGroup for this component.
     * @param tickGroup The new TickGroup.
     * @return A reference to this for chaining.
     */
    TickableComponent& SetTickGroup(const TickGroup tickGroup);

    /**
     * @brief Sets the tick priority for this component.
     * @param tickPriority The new TickPriority.
     * @return A reference to this for chaining.
     */
    TickableComponent& SetTickPriority(const TickPriority tickPriority);

    /**
     * @brief Changes the Context this component is associated with.
     * @param context The new Context.
     * @return A reference to this for chaining.
     */
    TickableComponent& SetContext(std::shared_ptr<Context> context);

    /**
     * @brief Virtual hook invoked when an EventAction runs on this component.
     *
     * Subclasses override this to respond to specific EventIDs.
     * @param eventId The EventID that executed.
     * @param durationSinceLastTick Elapsed time in seconds since the last tick.
     * @return True if the action executed successfully.
     */
    virtual bool ActionRan(EventID eventId, const double durationSinceLastTick);
  private:
    TickGroup mTickGroup;
    TickPriority mTickPriority;
    std::vector<EventID> mPendingEventIds;
    std::vector<std::shared_ptr<Action>> mPendingActions;
};

} // namespace Ship
