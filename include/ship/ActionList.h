#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <stdint.h>

#include "ship/PartList.h"
#include "ship/Action.h"

namespace Ship {

/**
 * @brief Extends PartList<Action> with action-type-based lookup helpers.
 *
 * Provides overloaded Has() and Get() methods that filter Actions by their
 * numeric type identifier (see ActionType).
 */
class ActionList : public PartList<Action> {
  public:
    using PartList<Action>::PartList;
    using PartList<Action>::Has;
    using PartList<Action>::Get;

    /**
     * @brief Checks whether any Action of the given type is in the list.
     * @param actionType The numeric action type to search for.
     * @return True if at least one matching Action is present.
     */
    bool Has(const uint32_t actionType) const;

    /**
     * @brief Returns all Actions of the given type.
     * @param actionType The numeric action type to filter by.
     * @return A vector of matching Actions.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> Get(const uint32_t actionType) const;

    /**
     * @brief Returns all Actions matching any of the given types.
     * @param actionTypes A vector of numeric action types to filter by.
     * @return A vector of matching Actions.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> Get(const std::vector<uint32_t>& actionTypes) const;
};

inline bool ActionList::Has(const uint32_t actionType) const {
    const auto& list = this->GetList();
    return std::find_if(list.begin(), list.end(), [actionType](const std::shared_ptr<Action>& action) {
               return action->GetType() == actionType;
           }) != list.end();
}

inline std::shared_ptr<std::vector<std::shared_ptr<Action>>> ActionList::Get(const uint32_t actionType) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    for (const auto& action : this->GetList()) {
        if (action->GetType() == actionType) {
            result->push_back(action);
        }
    }
    return result;
}

inline std::shared_ptr<std::vector<std::shared_ptr<Action>>>
ActionList::Get(const std::vector<uint32_t>& actionTypes) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    for (const auto& action : this->GetList()) {
        if (std::find(actionTypes.begin(), actionTypes.end(), action->GetType()) != actionTypes.end()) {
            result->push_back(action);
        }
    }
    return result;
}

} // namespace Ship
