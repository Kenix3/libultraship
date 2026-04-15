#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

#include "ship/Tickable.h"
#include "ship/Component.h"
#include "ship/Action.h"

namespace Ship {

class Context;

enum class TickGroup : uint32_t { TickGroupDefault = 0 };

enum class TickPriority : uint32_t { TickPriorityDefault = 0 };

class TickableComponent : public Tickable, public Component {
  public:
    // Creates a TickableComponent and registers it with the given Context.
    TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                      const TickGroup tickGroup = TickGroup::TickGroupDefault,
                      const TickPriority priority = TickPriority::TickPriorityDefault, const bool isTicking = true,
                      const bool isDrawing = true, const bool isDrawingDebugMenu = true);
    TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                      const TickGroup tickGroup = TickGroup::TickGroupDefault,
                      const TickPriority tickPriority = TickPriority::TickPriorityDefault,
                      const std::vector<std::shared_ptr<Action>>& actions = {});
    virtual ~TickableComponent();

    std::shared_ptr<Context> GetContext() const;
    TickGroup GetTickGroup() const;
    TickPriority GetTickPriority() const;
    uint64_t GetOrder() const;

    TickableComponent& SetTickGroup(const TickGroup tickGroup);
    TickableComponent& SetTickPriority(const TickPriority tickPriority);
    TickableComponent& SetContext(std::shared_ptr<Context> context);

  protected:
    virtual bool ActionRan(const uint32_t action, const double durationSinceLastTick);
    virtual bool DebugMenuDrawn(const double durationSinceLastTick);

  private:
    TickGroup mTickGroup;
    TickPriority mTickPriority;
    std::shared_ptr<Context> mContext;
};

} // namespace Ship
