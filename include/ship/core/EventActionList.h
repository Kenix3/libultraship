#pragma once

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ship/core/ActionList.h"
#include "ship/actions/EventAction.h"
#include "ship/events/ListenerAction.h"
#include "ship/events/EventTypes.h"

namespace Ship {

/**
 * @brief ActionList specialization with indexed EventAction lookup by EventID.
 *
 * `EventActionList` keeps all actions in the inherited generic `ActionList`
 * storage while maintaining an auxiliary map from `EventID` to the matching
 * `EventAction`s. This avoids scanning the full action list during
 * `Tickable::Tick(EventID)`.
 *
 * Non-`EventAction` entries remain fully supported in the underlying
 * `ActionList`, but are omitted from the event index.
 */
class EventActionList : public ActionList {
  public:
    using ActionList::ActionList;
    using ActionList::Get;
    using ActionList::GetFirst;
    using ActionList::Has;

    /**
     * @brief Returns true if any indexed EventAction exists for @p eventId.
     * @param eventId EventID to search for.
     * @return True if the bucket exists and is non-empty.
     */
    bool Has(EventID eventId) const;

    /**
     * @brief Returns the indexed EventActions for a single EventID.
     * @param eventId EventID to retrieve.
     * @return Snapshot of actions in dispatch order for that EventID.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> Get(EventID eventId) const;

    /**
     * @brief Returns the indexed EventActions for a set of EventIDs.
     * @param eventIds EventIDs to retrieve.
     * @return Snapshot of matching actions, grouped by the order of unique IDs in @p eventIds.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Action>>> Get(const std::vector<EventID>& eventIds) const;

  protected:
    /**
     * @brief Starts the action and updates the EventID index when applicable.
     * @param action Action that was added.
     * @param forced Whether insertion bypassed permission checks.
     */
    void Added(std::shared_ptr<Action> action, const bool forced) override;

    /**
     * @brief Stops the action and removes it from the EventID index when applicable.
     * @param action Action that was removed.
     * @param forced Whether removal bypassed permission checks.
     */
    void Removed(std::shared_ptr<Action> action, const bool forced) override;

  private:
    static bool ShouldInsertBefore(const std::shared_ptr<Action>& existing, const std::shared_ptr<Action>& incoming);
    void IndexEventAction(const std::shared_ptr<Action>& action);
    void UnindexEventAction(const std::shared_ptr<Action>& action);

    std::unordered_map<EventID, std::vector<std::shared_ptr<Action>>> mEventActions;
};

inline bool EventActionList::Has(EventID eventId) const {
    auto it = mEventActions.find(eventId);
    return it != mEventActions.end() && !it->second.empty();
}

inline std::shared_ptr<std::vector<std::shared_ptr<Action>>> EventActionList::Get(EventID eventId) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    auto it = mEventActions.find(eventId);
    if (it != mEventActions.end()) {
        result->insert(result->end(), it->second.begin(), it->second.end());
    }
    return result;
}

inline std::shared_ptr<std::vector<std::shared_ptr<Action>>> EventActionList::Get(const std::vector<EventID>& eventIds) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<Action>>>();
    std::unordered_set<EventID> seen;
    for (const auto eventId : eventIds) {
        if (!seen.insert(eventId).second) {
            continue;
        }
        auto it = mEventActions.find(eventId);
        if (it != mEventActions.end()) {
            result->insert(result->end(), it->second.begin(), it->second.end());
        }
    }
    return result;
}

inline void EventActionList::Added(std::shared_ptr<Action> action, const bool forced) {
    ActionList::Added(action, forced);
    IndexEventAction(action);
}

inline void EventActionList::Removed(std::shared_ptr<Action> action, const bool forced) {
    UnindexEventAction(action);
    ActionList::Removed(action, forced);
}

inline bool EventActionList::ShouldInsertBefore(const std::shared_ptr<Action>& existing,
                                                const std::shared_ptr<Action>& incoming) {
    auto* existingListener = dynamic_cast<ListenerAction*>(existing.get());
    auto* incomingListener = dynamic_cast<ListenerAction*>(incoming.get());
    if (existingListener == nullptr || incomingListener == nullptr) {
        return false;
    }

    if (incomingListener->GetPriority() != existingListener->GetPriority()) {
        return incomingListener->GetPriority() > existingListener->GetPriority();
    }

    return incomingListener->GetSequence() < existingListener->GetSequence();
}

inline void EventActionList::IndexEventAction(const std::shared_ptr<Action>& action) {
    auto* eventAction = dynamic_cast<EventAction*>(action.get());
    if (eventAction == nullptr) {
        return;
    }

    auto& bucket = mEventActions[eventAction->GetEventId()];
    auto insertIt = std::find_if(bucket.begin(), bucket.end(), [&action](const std::shared_ptr<Action>& existing) {
        return ShouldInsertBefore(existing, action);
    });
    bucket.insert(insertIt, action);
}

inline void EventActionList::UnindexEventAction(const std::shared_ptr<Action>& action) {
    auto* eventAction = dynamic_cast<EventAction*>(action.get());
    if (eventAction == nullptr) {
        return;
    }

    auto it = mEventActions.find(eventAction->GetEventId());
    if (it == mEventActions.end()) {
        return;
    }

    auto& bucket = it->second;
    bucket.erase(std::remove_if(bucket.begin(), bucket.end(), [&action](const std::shared_ptr<Action>& existing) {
                     return existing && action && existing->GetId() == action->GetId();
                 }),
                 bucket.end());

    if (bucket.empty()) {
        mEventActions.erase(it);
    }
}

} // namespace Ship
