#pragma once

#include "ship/Context.h"
#include "ship/Tickable.h"
#include "ship/Component.h"

#include <stdint.h>
#include <memory>

namespace Ship {
class Context;

enum class TickGroup:int32_t {
    TICK_GROUP_0,
    TICK_GROUP_1,
    TICK_GROUP_2,
    TICK_GROUP_3,
    TICK_GROUP_4,
    TICK_GROUP_5,
    TICK_GROUP_6,
    TICK_GROUP_7,
    TICK_GROUP_8,
    TICK_GROUP_9,
    TICK_GROUP_10,
    TICK_GROUP_11,
    TICK_GROUP_12,
    TICK_GROUP_13,
    TICK_GROUP_14,
    TICK_GROUP_15
};

class TickableComponent : public Tickable, Component {
  public:
    // You create a `TickableComponent`, and it registers itself with the Context. It also adds the Context as a parent of itself.
    // Context can sort itself at the beginning of the frame. Add a flag for whether or not the state is dirty.
    TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                      const TickGroup tickGroup = TickGroup::TICK_GROUP_0, int32_t priority = 0,
                      const bool isTicking = true,
                      const bool isDrawing = true);

    std::shared_ptr<Context> GetContext() const;
    TickGroup GetTickGroup() const;
    int32_t GetPriority() const;
    int64_t GetOrder() const;

    bool DrawDebugMenu();

  protected:
    // TODO: Not implemented.
    // The idea is that we display generic properties of the Component, then call the overridable method which will draw
    // the specific stuff. Then we create a collapsable box to show all children and recursively call DrawDebugMenu
    // There is probably going to be NO protection for cyclical references. If this happens, you will crash via
    // recursion.
    virtual bool DebugMenuDrawn() = 0;

    TickableComponent& SetTickGroup(const TickGroup tickGroup);
    TickableComponent& SetPriority(const TickGroup tickGroup);

  private:
    TickableComponent& SetContext(std::shared_ptr<Context> context);

    int32_t priority;
    TickGroup mTickGroup;
    std::shared_ptr<Context> mContext;
};

} // namespace Ship
