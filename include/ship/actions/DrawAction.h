#pragma once

#include "ship/Action.h"

namespace Ship {

class DrawAction : public Action {
  public:
    DrawAction(std::shared_ptr<Tickable> tickable);
    virtual ~DrawAction() = default;

  protected:
    virtual bool ActionRan(const double durationSinceLastTick) override;
};

} // namespace Ship
