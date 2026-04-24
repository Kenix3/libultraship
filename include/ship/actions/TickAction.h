#pragma once

#include "ship/Action.h"

namespace Ship {

/**
 * @brief Concrete Action for game logic ticks.
 *
 * TickAction delegates to the owning Tickable's tick logic each time it is run.
 */
class TickAction : public Action {
  public:
    /**
     * @brief Constructs a TickAction associated with the given Tickable.
     * @param tickable The Tickable that owns this tick Action.
     */
    TickAction(std::shared_ptr<Tickable> tickable);
    virtual ~TickAction() = default;

  protected:
    /**
     * @brief Called each cycle to execute game logic.
     * @param durationSinceLastTick Elapsed time in seconds since the last tick.
     * @return True if the tick completed successfully.
     */
    virtual bool ActionRan(const double durationSinceLastTick) override;
};

} // namespace Ship
