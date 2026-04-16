#pragma once

#include "ship/Action.h"

namespace Ship {

/**
 * @brief Concrete Action for rendering a frame.
 *
 * DrawAction delegates to the owning Tickable's draw logic each time it is run.
 */
class DrawAction : public Action {
  public:
    /**
     * @brief Constructs a DrawAction associated with the given Tickable.
     * @param tickable The Tickable that owns this draw Action.
     */
    DrawAction(std::shared_ptr<Tickable> tickable);
    virtual ~DrawAction() = default;

  protected:
    /**
     * @brief Called each frame to perform rendering.
     * @param durationSinceLastTick Elapsed time in seconds since the last tick.
     * @return True if drawing completed successfully.
     */
    virtual bool ActionRan(const double durationSinceLastTick) override;
};

} // namespace Ship
