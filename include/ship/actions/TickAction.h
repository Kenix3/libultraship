#pragma once

#include "ship/Action.h"

namespace Ship {

class TickAction : public Action {
  public:
    TickAction(std::shared_ptr<Tickable> tickable);
    virtual ~TickAction() = default;

  protected:
    virtual bool ActionRan(const double durationSinceLastTick) override;
};

} // namespace Ship
