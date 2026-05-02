#pragma once

#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <mutex>
#include <stdint.h>

#include "ship/Part.h"

namespace Ship {

/**
 * @brief Return codes for PartList add/remove operations.
 *
 * Non-negative values (>= 0) indicate the Part is guaranteed to be in the list
 * after the operation. Negative values indicate an error. A value > 0 means a
 * Part was actually added/removed; == 0 means the list is unchanged (duplicate).
 */
enum class ListReturnCode : int32_t {
    ForcedSuccess = 2,    /**< @brief Operation succeeded via force override. */
    Success = 1,          /**< @brief Operation succeeded normally. */
    Duplicate = 0,        /**< @brief Part already present; list unchanged. */
    NoItemsProvided = -1, /**< @brief The input collection was empty. */
    NotPermitted = -2,    /**< @brief Operation blocked by a permission check. */
    NotFound = -3,        /**< @brief The specified Part was not found. */
    Failed = -4           /**< @brief General failure (e.g. null pointer). */
};

/**
 * @brief A thread-safe ordered list of Parts.
 *
 * Provides add, remove, and lookup operations for a collection of Part-derived
 * objects stored as shared_ptr. All public methods are guarded by an internal
 * recursive mutex for thread safety.
 *
 * @tparam C The element type; must be derived from Part.
 */
template <typename C = Part> class PartList : public Part {
  public:
    /**
     * @brief Constructs a PartList, optionally pre-allocating storage.
     * @param initialAllocation Number of elements to reserve up front.
     */
    explicit PartList(const size_t initialAllocation = 0);
    virtual ~PartList() = default;

    /**
     * @brief Checks whether a specific Part is in the list.
     * @param part The Part to search for.
     * @return True if the Part is present.
     */
    bool Has(std::shared_ptr<C> part) const;

    /**
     * @brief Checks whether any Part of type T is in the list.
     * @tparam T The derived type to search for via dynamic_cast.
     * @return True if at least one matching Part is found.
     */
    template <typename T> bool Has() const;

    /**
     * @brief Checks whether a Part with the given ID is in the list.
     * @param id The unique Part ID to search for.
     * @return True if found.
     */
    bool Has(const uint64_t id) const;

    /** @brief Checks whether the list contains any Parts at all. */
    bool Has() const;

    /** @brief Returns the number of Parts in the list. */
    size_t GetCount() const;

    /**
     * @brief Retrieves a Part by its unique ID.
     * @param id The Part ID to look up.
     * @return The matching Part, or nullptr if not found.
     */
    std::shared_ptr<C> Get(const uint64_t id) const;

    /** @brief Returns a snapshot (copy) of all Parts in the list. */
    std::shared_ptr<std::vector<std::shared_ptr<C>>> Get() const;

    /**
     * @brief Returns all Parts that can be dynamic_cast to type T.
     * @tparam T The target derived type.
     * @return A vector of matching Parts cast to T.
     */
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> Get() const;

    /**
     * @brief Returns all Parts whose IDs appear in the given vector.
     * @param ids The IDs to filter by.
     * @return A vector of matching Parts.
     */
    std::shared_ptr<std::vector<std::shared_ptr<C>>> Get(const std::vector<uint64_t>& ids) const;

    /**
     * @brief Returns the first Part that can be dynamic_cast to T.
     * @tparam T The target derived type.
     * @return The first matching Part, or nullptr if none found.
     */
    template <typename T> std::shared_ptr<T> GetFirst() const;

    /**
     * @brief Adds a Part to the list if not already present.
     * @param part The Part to add.
     * @param force If true, bypass the CanAdd() permission check.
     * @return ListReturnCode indicating the result.
     */
    ListReturnCode Add(std::shared_ptr<C> part, const bool force = false);

    /**
     * @brief Adds multiple Parts to the list.
     * @param parts The Parts to add.
     * @param force If true, bypass the CanAdd() permission check for each Part.
     * @return The aggregate ListReturnCode for the batch operation.
     */
    ListReturnCode Add(const std::vector<std::shared_ptr<C>>& parts, const bool force = false);

    /**
     * @brief Removes a specific Part from the list.
     * @param part The Part to remove.
     * @param force If true, bypass the CanRemove() permission check.
     * @return ListReturnCode indicating the result.
     */
    ListReturnCode Remove(std::shared_ptr<C> part, const bool force = false);

    /**
     * @brief Removes a Part by its unique ID.
     * @param id The Part ID to remove.
     * @param force If true, bypass the CanRemove() permission check.
     * @return ListReturnCode indicating the result.
     */
    ListReturnCode Remove(const uint64_t id, const bool force = false);

    /**
     * @brief Removes all Parts from the list.
     * @param force If true, bypass the CanRemove() permission check for each Part.
     * @return ListReturnCode indicating the result.
     */
    ListReturnCode Remove(const bool force = false);

    /**
     * @brief Removes multiple Parts from the list.
     * @param parts The Parts to remove.
     * @param force If true, bypass the CanRemove() permission check.
     * @return The aggregate ListReturnCode for the batch operation.
     */
    ListReturnCode Remove(const std::vector<std::shared_ptr<C>>& parts, const bool force = false);

    /**
     * @brief Removes all Parts that can be dynamic_cast to type T.
     * @tparam T The derived type to match for removal.
     * @param force If true, bypass the CanRemove() permission check.
     * @return ListReturnCode indicating the result.
     */
    template <typename T> ListReturnCode Remove(const bool force = false);

    /**
     * @brief Removes all Parts whose IDs appear in the given vector.
     * @param ids The IDs to remove.
     * @param force If true, bypass the CanRemove() permission check.
     * @return The aggregate ListReturnCode for the batch operation.
     */
    ListReturnCode Remove(const std::vector<uint64_t>& ids, const bool force = false);

    /**
     * @brief Direct access to the underlying vector for efficient iteration.
     *
     * The caller must hold the PartList mutex (via GetMutex()) when using this
     * reference to ensure thread safety.
     * @return A mutable reference to the internal vector.
     */
    std::vector<std::shared_ptr<C>>& GetList();

    /** @brief Direct const access to the underlying vector. */
    const std::vector<std::shared_ptr<C>>& GetList() const;

    /**
     * @brief Returns a reference to the internal mutex for external synchronization.
     *
     * Use this when you need to hold the lock across multiple operations, e.g.
     * iterating via GetList(). Only available when COMPONENT_THREAD_SAFE is defined.
     */
#ifdef COMPONENT_THREAD_SAFE
    std::recursive_mutex& GetMutex() const;
#endif

    /**
     * @brief Permission hook called before adding a Part. Override to deny.
     * @param part The Part about to be added.
     * @return True if the addition is permitted.
     */
    virtual bool CanAdd(std::shared_ptr<C> part);

    /**
     * @brief Permission hook called before removing a Part. Override to deny.
     * @param part The Part about to be removed.
     * @return True if the removal is permitted.
     */
    virtual bool CanRemove(std::shared_ptr<C> part);

    /**
     * @brief Notification hook called after a Part has been added.
     * @param part The Part that was added.
     * @param forced Whether the addition bypassed permission checks.
     */
    virtual void Added(std::shared_ptr<C> part, const bool forced);

    /**
     * @brief Notification hook called after a Part has been removed.
     * @param part The Part that was removed.
     * @param forced Whether the removal bypassed permission checks.
     */
    virtual void Removed(std::shared_ptr<C> part, const bool forced);

  private:
    std::vector<std::shared_ptr<C>> mList;
#ifdef COMPONENT_THREAD_SAFE
    mutable std::recursive_mutex mMutex;
#endif
};

// ---- Inline implementations ----

template <typename C> PartList<C>::PartList(const size_t initialAllocation) : Part(), mList()
#ifdef COMPONENT_THREAD_SAFE
                                                                               ,
                                                                               mMutex()
#endif
{
    mList.reserve(initialAllocation);
}

template <typename C> bool PartList<C>::Has(std::shared_ptr<C> part) const {
    if (!part) {
        return false;
    }
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return std::find_if(mList.begin(), mList.end(), [&part](const std::shared_ptr<C>& item) {
               return item->GetId() == part->GetId();
           }) != mList.end();
}

template <typename C> template <typename T> bool PartList<C>::Has() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return std::find_if(mList.begin(), mList.end(), [](const std::shared_ptr<C>& item) {
               return std::dynamic_pointer_cast<T>(item) != nullptr;
           }) != mList.end();
}

template <typename C> bool PartList<C>::Has(const uint64_t id) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return std::find_if(mList.begin(), mList.end(),
                        [id](const std::shared_ptr<C>& item) { return item->GetId() == id; }) != mList.end();
}

template <typename C> bool PartList<C>::Has() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return !mList.empty();
}

template <typename C> size_t PartList<C>::GetCount() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return mList.size();
}

template <typename C> std::shared_ptr<C> PartList<C>::Get(const uint64_t id) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    auto it =
        std::find_if(mList.begin(), mList.end(), [id](const std::shared_ptr<C>& item) { return item->GetId() == id; });
    return it != mList.end() ? *it : nullptr;
}

template <typename C> std::shared_ptr<std::vector<std::shared_ptr<C>>> PartList<C>::Get() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return std::make_shared<std::vector<std::shared_ptr<C>>>(mList);
}

template <typename C> template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> PartList<C>::Get() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    auto result = std::make_shared<std::vector<std::shared_ptr<T>>>();
    for (const auto& item : mList) {
        auto typed = std::dynamic_pointer_cast<T>(item);
        if (typed) {
            result->push_back(typed);
        }
    }
    return result;
}

template <typename C> template <typename T> std::shared_ptr<T> PartList<C>::GetFirst() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    for (const auto& item : mList) {
        auto typed = std::dynamic_pointer_cast<T>(item);
        if (typed) {
            return typed;
        }
    }
    return nullptr;
}

template <typename C>
std::shared_ptr<std::vector<std::shared_ptr<C>>> PartList<C>::Get(const std::vector<uint64_t>& ids) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    auto result = std::make_shared<std::vector<std::shared_ptr<C>>>();
    for (const auto& item : mList) {
        const uint64_t id = item->GetId();
        if (std::find(ids.begin(), ids.end(), id) != ids.end()) {
            result->push_back(item);
        }
    }
    return result;
}

template <typename C> ListReturnCode PartList<C>::Add(std::shared_ptr<C> part, const bool force) {
    if (!part) {
        return ListReturnCode::Failed;
    }
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    if (std::find_if(mList.begin(), mList.end(), [&part](const std::shared_ptr<C>& item) {
            return item->GetId() == part->GetId();
        }) != mList.end()) {
        return ListReturnCode::Duplicate;
    }
    const bool canAdd = CanAdd(part);
    if (!canAdd && !force) {
        return ListReturnCode::NotPermitted;
    }
    const bool forced = !canAdd && force;
    mList.push_back(part);
    Added(part, forced);
    return forced ? ListReturnCode::ForcedSuccess : ListReturnCode::Success;
}

template <typename C> ListReturnCode PartList<C>::Add(const std::vector<std::shared_ptr<C>>& parts, const bool force) {
    if (parts.empty()) {
        return ListReturnCode::NoItemsProvided;
    }
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    ListReturnCode result = ListReturnCode::Duplicate;
    for (const auto& part : parts) {
        const ListReturnCode r = Add(part, force);
        if (static_cast<int32_t>(r) > static_cast<int32_t>(result)) {
            result = r;
        } else if (static_cast<int32_t>(r) < static_cast<int32_t>(ListReturnCode::Duplicate) &&
                   static_cast<int32_t>(r) < static_cast<int32_t>(result)) {
            result = r;
        }
    }
    return result;
}

template <typename C> ListReturnCode PartList<C>::Remove(std::shared_ptr<C> part, const bool force) {
    if (!part) {
        return ListReturnCode::Failed;
    }
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    auto it = std::find_if(mList.begin(), mList.end(),
                           [&part](const std::shared_ptr<C>& item) { return item->GetId() == part->GetId(); });
    if (it == mList.end()) {
        return ListReturnCode::NotFound;
    }
    const bool canRemove = CanRemove(part);
    if (!canRemove && !force) {
        return ListReturnCode::NotPermitted;
    }
    const bool forced = !canRemove && force;
    mList.erase(it);
    Removed(part, forced);
    return forced ? ListReturnCode::ForcedSuccess : ListReturnCode::Success;
}

template <typename C> ListReturnCode PartList<C>::Remove(const uint64_t id, const bool force) {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    auto it =
        std::find_if(mList.begin(), mList.end(), [id](const std::shared_ptr<C>& item) { return item->GetId() == id; });
    if (it == mList.end()) {
        return ListReturnCode::NotFound;
    }
    auto part = *it;
    const bool canRemove = CanRemove(part);
    if (!canRemove && !force) {
        return ListReturnCode::NotPermitted;
    }
    const bool forced = !canRemove && force;
    mList.erase(it);
    Removed(part, forced);
    return forced ? ListReturnCode::ForcedSuccess : ListReturnCode::Success;
}

template <typename C> ListReturnCode PartList<C>::Remove(const bool force) {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    if (mList.empty()) {
        return ListReturnCode::NotFound;
    }
    // Make a snapshot so hooks can safely modify the list
    auto snapshot = std::vector<std::shared_ptr<C>>(mList);
    ListReturnCode result = ListReturnCode::NotFound;
    for (const auto& part : snapshot) {
        const ListReturnCode r = Remove(part, force);
        if (static_cast<int32_t>(r) > static_cast<int32_t>(result)) {
            result = r;
        } else if (static_cast<int32_t>(r) < static_cast<int32_t>(ListReturnCode::Duplicate) &&
                   static_cast<int32_t>(r) < static_cast<int32_t>(result)) {
            result = r;
        }
    }
    return result;
}

template <typename C>
ListReturnCode PartList<C>::Remove(const std::vector<std::shared_ptr<C>>& parts, const bool force) {
    if (parts.empty()) {
        return ListReturnCode::NoItemsProvided;
    }
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    ListReturnCode result = ListReturnCode::NotFound;
    for (const auto& part : parts) {
        const ListReturnCode r = Remove(part, force);
        if (static_cast<int32_t>(r) > static_cast<int32_t>(result)) {
            result = r;
        } else if (static_cast<int32_t>(r) < static_cast<int32_t>(ListReturnCode::Duplicate) &&
                   static_cast<int32_t>(r) < static_cast<int32_t>(result)) {
            result = r;
        }
    }
    return result;
}

template <typename C> template <typename T> ListReturnCode PartList<C>::Remove(const bool force) {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    // Collect matching items first
    auto snapshot = std::vector<std::shared_ptr<C>>();
    for (const auto& item : mList) {
        if (std::dynamic_pointer_cast<T>(item) != nullptr) {
            snapshot.push_back(item);
        }
    }
    if (snapshot.empty()) {
        return ListReturnCode::NotFound;
    }
    ListReturnCode result = ListReturnCode::NotFound;
    for (const auto& part : snapshot) {
        const ListReturnCode r = Remove(part, force);
        if (static_cast<int32_t>(r) > static_cast<int32_t>(result)) {
            result = r;
        } else if (static_cast<int32_t>(r) < static_cast<int32_t>(ListReturnCode::Duplicate) &&
                   static_cast<int32_t>(r) < static_cast<int32_t>(result)) {
            result = r;
        }
    }
    return result;
}

template <typename C> ListReturnCode PartList<C>::Remove(const std::vector<uint64_t>& ids, const bool force) {
    if (ids.empty()) {
        return ListReturnCode::NoItemsProvided;
    }
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    // Collect matching items first
    auto snapshot = std::vector<std::shared_ptr<C>>();
    for (const auto& item : mList) {
        if (std::find(ids.begin(), ids.end(), item->GetId()) != ids.end()) {
            snapshot.push_back(item);
        }
    }
    if (snapshot.empty()) {
        return ListReturnCode::NotFound;
    }
    ListReturnCode result = ListReturnCode::NotFound;
    for (const auto& part : snapshot) {
        const ListReturnCode r = Remove(part, force);
        if (static_cast<int32_t>(r) > static_cast<int32_t>(result)) {
            result = r;
        } else if (static_cast<int32_t>(r) < static_cast<int32_t>(ListReturnCode::Duplicate) &&
                   static_cast<int32_t>(r) < static_cast<int32_t>(result)) {
            result = r;
        }
    }
    return result;
}

template <typename C> std::vector<std::shared_ptr<C>>& PartList<C>::GetList() {
    return mList;
}

template <typename C> const std::vector<std::shared_ptr<C>>& PartList<C>::GetList() const {
    return mList;
}

#ifdef COMPONENT_THREAD_SAFE
template <typename C> std::recursive_mutex& PartList<C>::GetMutex() const {
    return mMutex;
}
#endif

template <typename C> bool PartList<C>::CanAdd(std::shared_ptr<C> part) {
    return true;
}

template <typename C> bool PartList<C>::CanRemove(std::shared_ptr<C> part) {
    return true;
}

template <typename C> void PartList<C>::Added(std::shared_ptr<C> part, const bool forced) {
}

template <typename C> void PartList<C>::Removed(std::shared_ptr<C> part, const bool forced) {
}

} // namespace Ship
