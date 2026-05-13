#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <stdint.h>

#include "ship/PartList.h"
#include "ship/Action.h"

namespace Ship {

/**
 * @brief Extends PartList<Action> with EventID-based lookup helpers.
 *
 * Provides overloaded Has() and Get() methods that filter Actions by their
 * numeric EventID. Automatically starts Actions when added
 * and stops them when removed.
 */
class ActionList : public PartList<Action> {
  public:
    using PartList<Action>::PartList;
    using PartList<Action>::Has;
    using PartList<Action>::Get;

    /**
     * @brief Checks whether any Action with the given EventID is in the list.
     * @param eventId The EventID to search for.
     * @return True if at least one matching Action is present.
     */
    bool Has(EventID eventId) const;

    /**
     * @brief Returns all Actions with the given EventID.
     * @param eventId The EventID to filter by.
     * @return A vector of matching Actions.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> Get(EventID eventId) const;

    /**
     * @brief Returns all Actions matching any of the given EventIDs.
     * @param eventIds A vector of EventIDs to filter by.
     * @return A vector of matching Actions.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> Get(const std::vector<EventID>& eventIds) const;

  protected:
    /**
     * @brief Starts the Action after it has been added to the list.
     * @param action The Action that was added.
     * @param forced Whether the addition bypassed permission checks.
     */
    void Added(std::shared_ptr<Action> action, const bool forced) override;

    /**
     * @brief Stops the Action after it has been removed from the list.
     * @param action The Action that was removed.
     * @param forced Whether the removal bypassed permission checks.
     */
    void Removed(std::shared_ptr<Action> action, const bool forced) override;
};

inline bool ActionList::Has(EventID eventId) const {
    const auto& list = this->GetList();
    return std::find_if(list.begin(), list.end(), [eventId](const std::shared_ptr<Action>& action) {
               return action->GetEventId() == eventId;
           }) != list.end();
}

inline std::shared_ptr<std::vector<std::shared_ptr<Action>>> ActionList::Get(EventID eventId) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    for (const auto& action : this->GetList()) {
        if (action->GetEventId() == eventId) {
            result->push_back(action);
        }
    }
    return result;
}

inline std::shared_ptr<std::vector<std::shared_ptr<Action>>>
ActionList::Get(const std::vector<EventID>& eventIds) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    for (const auto& action : this->GetList()) {
        if (std::find(eventIds.begin(), eventIds.end(), action->GetEventId()) != eventIds.end()) {
            result->push_back(action);
        }
    }
    return result;
}

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
