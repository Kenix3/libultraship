#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

#include "ship/Tickable.h"
#include "ship/Component.h"
#include "ship/Action.h"

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
 */
class TickableComponent : public Tickable, public Component {
  public:
    /**
     * @brief Constructs a TickableComponent with convenience booleans.
     *
     * Call RegisterWithContext() after the object is owned by a shared_ptr.
     * @param name Human-readable name for the Component.
     * @param context The Context to register with.
     * @param tickGroup The TickGroup this component belongs to.
     * @param tickPriority Execution priority within the TickGroup.
     * @param isTicking Whether the component starts in the ticking state.
     * @param isDrawing Whether a DrawAction is created automatically.
     * @param isDrawingDebugMenu Whether a DrawDebugMenuAction is created automatically.
     */
    TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                      const TickGroup tickGroup = TickGroup::TickGroupDefault,
                      const TickPriority tickPriority = TickPriority::TickPriorityDefault, const bool isTicking = true,
                      const bool isDrawing = true, const bool isDrawingDebugMenu = true);

    /**
     * @brief Constructs a TickableComponent with an explicit list of Actions.
     * @param name Human-readable name for the Component.
     * @param context The Context to register with.
     * @param tickGroup The TickGroup this component belongs to.
     * @param tickPriority Execution priority within the TickGroup.
     * @param actions The initial set of Actions to register.
     */
    TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                      const TickGroup tickGroup = TickGroup::TickGroupDefault,
                      const TickPriority tickPriority = TickPriority::TickPriorityDefault,
                      const std::vector<std::shared_ptr<Action>>& actions = {});
    virtual ~TickableComponent();

    /**
     * @brief Registers this component with its Context.
     *
     * Must be called after the object is managed by a shared_ptr.
     */
    void RegisterWithContext();

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
     * @brief Virtual hook invoked when an Action runs on this component.
     *
     * Subclasses override this to respond to Tick, Draw, and DrawDebugMenu action types.
     * Called by TickAction, DrawAction, and DrawDebugMenuAction after casting the owning
     * Tickable to TickableComponent.
     * @param action The numeric action type that executed (see ActionType enum).
     * @param durationSinceLastTick Elapsed time in seconds since the last tick.
     * @return True if the action executed successfully.
     */
    virtual bool ActionRan(const uint32_t action, const double durationSinceLastTick);

    /**
     * @brief Virtual hook invoked when the debug menu draw Action runs.
     * @param durationSinceLastTick Elapsed time in seconds since the last tick.
     * @return True if the debug menu was drawn successfully.
     */
    virtual bool DebugMenuDrawn(const double durationSinceLastTick);

  protected:
  private:
    TickGroup mTickGroup;
    TickPriority mTickPriority;
    std::shared_ptr<Context> mContext;
    bool mPendingTicking;
    bool mPendingDrawing;
    bool mPendingDrawingDebugMenu;
};

} // namespace Ship
