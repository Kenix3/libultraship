#pragma once

#include <memory>

#include "ship/core/PartList.h"
#include "ship/core/Action.h"

namespace Ship {

/**
 * @brief Generic ordered container of `Action` instances.
 *
 * `ActionList` provides start/stop lifecycle behavior for contained actions but
 * does not perform any `EventID` indexing or filtering. Event-targeted lookup is
 * implemented by `EventActionList`.
 */
class ActionList : public PartList<Action, std::shared_ptr<Action>> {
  public:
    using PartList<Action, std::shared_ptr<Action>>::PartList;
    using PartList<Action, std::shared_ptr<Action>>::Has;
    using PartList<Action, std::shared_ptr<Action>>::Get;
    using PartList<Action, std::shared_ptr<Action>>::GetFirst;

  protected:
    /**
     * @brief Starts the action after insertion.
     * @param action  Action that was added.
     * @param forced  Whether insertion bypassed permission checks.
     */
    void Added(std::shared_ptr<Action> action, const bool forced) override;

    /**
     * @brief Stops the action after removal.
     * @param action  Action that was removed.
     * @param forced  Whether removal bypassed permission checks.
     */
    void Removed(std::shared_ptr<Action> action, const bool forced) override;
};

inline void ActionList::Added(std::shared_ptr<Action> action, const bool forced) {
    if (action) {
        action->Start();
    }
}

inline void ActionList::Removed(std::shared_ptr<Action> action, const bool forced) {
    if (action) {
        action->Stop();
    }
}

} // namespace Ship
