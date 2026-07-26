#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <stdint.h>

#include "ship/core/PartList.h"
#include "ship/core/Action.h"
#include "ship/events/EventTypes.h"
#include "ship/actions/EventAction.h"

namespace Ship {

/**
 * @brief Extends PartList<Action> with EventAction-based lookup helpers.
 *
 * Provides overloaded Has() and Get() methods that filter EventActions by their
 * numeric EventID. Automatically starts Actions when added
 * and stops them when removed.
 */
class ActionList : public PartList<Action, std::shared_ptr<Action>> {
  public:
    using PartList<Action, std::shared_ptr<Action>>::PartList;
    using PartList<Action, std::shared_ptr<Action>>::Has;
    using PartList<Action, std::shared_ptr<Action>>::Get;
    using PartList<Action, std::shared_ptr<Action>>::GetFirst;

    /**
     * @brief Checks whether any EventAction with the given EventID is in the list.
     * @param eventId The EventID to search for.
     * @return True if at least one matching EventAction is present.
     */
    bool Has(EventID eventId) const;

    /**
     * @brief Returns all EventActions with the given EventID.
     * @param eventId The EventID to filter by.
     * @return A vector of matching EventActions.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> Get(EventID eventId) const;

    /**
     * @brief Returns all EventActions matching any of the given EventIDs.
     * @param eventIds A vector of EventIDs to filter by.
     * @return A vector of matching EventActions.
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
               auto* ea = dynamic_cast<EventAction*>(action.get());
               return ea && ea->GetEventId() == eventId;
           }) != list.end();
}

inline std::shared_ptr<std::vector<std::shared_ptr<Action>>> ActionList::Get(EventID eventId) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    for (const auto& action : this->GetList()) {
        auto* ea = dynamic_cast<EventAction*>(action.get());
        if (ea && ea->GetEventId() == eventId) {
            result->push_back(action);
        }
    }
    return result;
}

inline std::shared_ptr<std::vector<std::shared_ptr<Action>>>
ActionList::Get(const std::vector<EventID>& eventIds) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    for (const auto& action : this->GetList()) {
        auto* ea = dynamic_cast<EventAction*>(action.get());
        if (ea && std::find(eventIds.begin(), eventIds.end(), ea->GetEventId()) != eventIds.end()) {
            result->push_back(action);
        }
    }
    return result;
}

inline void ActionList::Added(std::shared_ptr<Action> action, const bool forced) {
    if (action) {
        action->Start();
    }
    // Keep the list sorted by EventID (for EventActions) so Run() never needs to sort.
    auto& list = GetList();
    std::stable_sort(list.begin(), list.end(), [](const std::shared_ptr<Action>& a, const std::shared_ptr<Action>& b) {
        auto* ea = dynamic_cast<EventAction*>(a.get());
        auto* eb = dynamic_cast<EventAction*>(b.get());
        if (ea && eb) {
            return ea->GetEventId() < eb->GetEventId();
        }
        // Non-EventActions stay in insertion order
        return false;
    });
}

inline void ActionList::Removed(std::shared_ptr<Action> action, const bool forced) {
    if (action) {
        action->Stop();
    }
}

} // namespace Ship
