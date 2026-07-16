#pragma once

#include <algorithm>
#include <mutex>

#include "ship/PartList.h"
#include "ship/TickableComponent.h"

namespace Ship {

/**
 * @brief Extends PartList<TickableComponent> with order-aware sorting.
 *
 * Maintains a sorted list of TickableComponents by their composite order
 * (TickGroup + TickPriority). Call Sort() after order changes to re-establish
 * the correct execution sequence.
 */
class TickableList : public PartList<TickableComponent> {
  public:
    using PartList<TickableComponent>::PartList;

    /**
     * @brief Sorts the list by each component's composite order value.
     * @return A reference to this for chaining.
     */
    TickableList& Sort();
};

inline TickableList& TickableList::Sort() {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(GetMutex());
#endif
    auto& list = GetList();
    std::stable_sort(list.begin(), list.end(),
                     [](const std::shared_ptr<TickableComponent>& a, const std::shared_ptr<TickableComponent>& b) {
                         return a->GetOrder() < b->GetOrder();
                     });
    return *this;
}

} // namespace Ship
