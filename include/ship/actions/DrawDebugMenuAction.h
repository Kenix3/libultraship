#pragma once

#include "ship/Action.h"

namespace Ship {

class DrawDebugMenuAction : public Action {
  public:
    DrawDebugMenuAction(std::shared_ptr<Tickable> tickable);
    virtual ~DrawDebugMenuAction() = default;

  protected:
    virtual bool ActionRan(const double durationSinceLastTick) override;
};

} // namespace Ship
