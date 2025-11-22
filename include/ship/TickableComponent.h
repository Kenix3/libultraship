#pragma once

#include "ship/Context.h"
#include "ship/Tickable.h"
#include "ship/Component.h"

#include <stdint.h>
#include <memory>

namespace Ship {
class Context;

enum class TickGroup : uint32_t {
    TickGroupDefault = 0
};

enum class TickPriority : uint32_t {
    TickPriorityDefault = 0
};

class TickableComponent : public Tickable, public Component {
  public:
    // You create a `TickableComponent`, and it registers itself with the Context. It also adds the Context as a parent of itself.
    // Context can sort itself at the beginning of the frame. Add a flag for whether or not the state is dirty.

    // TODO: DrawDebugMenu stuff. Should add some profiling stuff similar to Tickable.
    // Then get to new Context code and conversion.
    // The idea with draw debug menu that we display generic properties of the Component, then call the overridable method which will draw
    // the specific stuff. Then we create a collapsable box to show all children and recursively call DrawDebugMenu
    // There is probably going to be NO protection for cyclical references. If this happens, you will crash via
    // recursion.

    // The override of RunAction on the switch will allow us to do the children logic

    TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                      const TickGroup tickGroup = TickGroup::TickGroupDefault,
                      const TickPriority priority = TickPriority::TickPriorityDefault,
                      const bool isTicking = true,
                      const bool isDrawing = true, const bool isDebugMenuDrawing = true);
    TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                      const TickGroup tickGroup = TickGroup::TickGroupDefault,
                      const TickPriority tickPriority = TickPriority::TickPriorityDefault,
                      const std::vector<bool>& actions = {});
    ~TickableComponent();

    std::shared_ptr<Context> GetContext() const;
    TickGroup GetTickGroup() const;
    TickPriority GetTickPriority() const;
    uint64_t GetOrder() const;

    TickableComponent& SetTickGroup(const TickGroup tickGroup);
    TickableComponent& SetTickPriority(const TickPriority tickPriority);
    TickableComponent& SetContext(std::shared_ptr<Context> context);

  protected:
    virtual bool ActionRan(const ActionType action, const double durationSinceLastTick) override;
    virtual bool DebugMenuDrawn(const double durationSinceLastTick);

  private:
    TickGroup mTickGroup;
    TickPriority mTickPriority;
    std::shared_ptr<Context> mContext;
};

} // namespace Ship
