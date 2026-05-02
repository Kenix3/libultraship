#pragma once

#include "ship/Action.h"

namespace Ship {

/**
 * @brief Concrete Action for rendering debug menu UI.
 *
 * DrawDebugMenuAction delegates to the owning Tickable's debug menu draw logic
 * each time it is run.
 */
class DrawDebugMenuAction : public Action {
  public:
    /**
     * @brief Constructs a DrawDebugMenuAction associated with the given Tickable.
     * @param tickable The Tickable that owns this debug menu draw Action.
     */
    DrawDebugMenuAction(std::shared_ptr<Tickable> tickable);
    virtual ~DrawDebugMenuAction() = default;

  protected:
    /**
     * @brief Called each frame to render the debug menu.
     * @param durationSinceLastTick Elapsed time in seconds since the last tick.
     * @return True if the debug menu was drawn successfully.
     */
    virtual bool ActionRan(const double durationSinceLastTick) override;
};

} // namespace Ship
