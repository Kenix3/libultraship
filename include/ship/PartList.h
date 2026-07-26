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

template <typename C, typename StoredPtr> struct PartListStoredPtrTraits;

/**
 * @brief Pointer-traits adapter for strong storage (`std::shared_ptr`).
 *
 * Converts stored pointers to usable shared pointers, reports expiry state,
 * and converts incoming pointers into the stored type.
 */
template <typename C> struct PartListStoredPtrTraits<C, std::shared_ptr<C>> {
    /**
     * @brief Returns the stored pointer unchanged.
     * @param ptr Stored strong pointer.
     * @return Same strong pointer instance.
     */
    static std::shared_ptr<C> Lock(const std::shared_ptr<C>& ptr) {
        return ptr;
    }

    /**
     * @brief Indicates whether the stored pointer is considered expired.
     * @param ptr Stored strong pointer.
     * @return True when pointer is null.
     */
    static bool IsExpired(const std::shared_ptr<C>& ptr) {
        return ptr == nullptr;
    }

    /**
     * @brief Converts an API pointer into strong stored form.
     * @param ptr Incoming shared pointer.
     * @return Same shared pointer for storage.
     */
    static std::shared_ptr<C> Store(const std::shared_ptr<C>& ptr) {
        return ptr;
    }
};

/**
 * @brief Pointer-traits adapter for weak storage (`std::weak_ptr`).
 *
 * Locks weak pointers for access, reports weak expiry, and converts incoming
 * shared pointers into weak stored form.
 */
template <typename C> struct PartListStoredPtrTraits<C, std::weak_ptr<C>> {
    /**
     * @brief Locks a weak pointer.
     * @param ptr Stored weak pointer.
     * @return Shared pointer if still alive, otherwise nullptr.
     */
    static std::shared_ptr<C> Lock(const std::weak_ptr<C>& ptr) {
        return ptr.lock();
    }

    /**
     * @brief Indicates whether the weak pointer is expired.
     * @param ptr Stored weak pointer.
     * @return True when pointed object no longer exists.
     */
    static bool IsExpired(const std::weak_ptr<C>& ptr) {
        return ptr.expired();
    }

    /**
     * @brief Converts an API pointer into weak stored form.
     * @param ptr Incoming shared pointer.
     * @return Weak pointer referring to the same object.
     */
    static std::weak_ptr<C> Store(const std::shared_ptr<C>& ptr) {
        return ptr;
    }
};

/**
 * @brief A thread-safe ordered list of Parts.
 *
 * Provides add, remove, and lookup operations for a collection of Part-derived
 * objects. Storage pointer type is configurable (shared_ptr or weak_ptr), while
 * the public API remains shared_ptr-based.
 *
 * @tparam C         The element type; must be derived from Part.
 * @tparam StoredPtr The stored pointer type (shared_ptr<C> or weak_ptr<C>).
 */
template <typename C = Part, typename StoredPtr = std::shared_ptr<C>> class PartList : public Part {
  public:
    static_assert(std::is_same<StoredPtr, std::shared_ptr<C>>::value ||
                      std::is_same<StoredPtr, std::weak_ptr<C>>::value,
                  "StoredPtr must be std::shared_ptr<C> or std::weak_ptr<C>");

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

    /** @brief Returns the number of currently live Parts in the list. */
    size_t GetCount() const;

    /**
     * @brief Returns a monotonically increasing counter that increments on every add or remove.
     *
     * Callers can store this value and compare it on subsequent frames to detect
     * whether the list has changed since they last inspected it.
     */
    uint64_t GetMutationVersion() const {
        return mMutationVersion;
    }

    /**
     * @brief Retrieves a Part by its unique ID.
     * @param id The Part ID to look up.
     * @return The matching Part, or nullptr if not found.
     */
    std::shared_ptr<C> Get(const uint64_t id) const;

    /** @brief Returns a snapshot (copy) of all currently live Parts in the list. */
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

  protected:
    /**
     * @brief Direct access to the underlying vector for strong-storage lists.
     *
     * Available only when `StoredPtr` is `std::shared_ptr<C>`.
     */
    template <typename P = StoredPtr>
    typename std::enable_if<std::is_same<P, std::shared_ptr<C>>::value, std::vector<P>&>::type GetList();

    /**
     * @brief Direct const access to the underlying vector for strong-storage lists.
     *
     * Available only when `StoredPtr` is `std::shared_ptr<C>`.
     */
    template <typename P = StoredPtr>
    typename std::enable_if<std::is_same<P, std::shared_ptr<C>>::value, const std::vector<P>&>::type GetList() const;

#ifdef COMPONENT_THREAD_SAFE
    /** @brief Returns the internal recursive mutex used to guard list access. */
    std::recursive_mutex& GetMutex() const;
#endif

    /**
     * @brief Permission hook called before adding a Part. Override to deny.
     *
     * Implementations must not perform cross-list add/remove operations. This
     * hook may be called outside the target list's lock to avoid lock-order
     * inversions between lists.
     *
     * @param part The Part about to be added.
     * @return True if the addition is permitted.
     */
    virtual bool CanAdd(std::shared_ptr<C> part);

    /**
     * @brief Permission hook called before removing a Part. Override to deny.
     *
     * Implementations must not perform cross-list add/remove operations. This
     * hook may be called outside the target list's lock to avoid lock-order
     * inversions between lists.
     *
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
    using PtrTraits = PartListStoredPtrTraits<C, StoredPtr>;

    std::shared_ptr<C> LockPtr(const StoredPtr& ptr) const;
    StoredPtr StorePtr(const std::shared_ptr<C>& ptr) const;
    bool IsExpiredPtr(const StoredPtr& ptr) const;
    void PruneExpired();
    bool ContainsIdUnlocked(const uint64_t id) const;
    bool EraseByIdUnlocked(const uint64_t id, std::shared_ptr<C>* removedPart = nullptr);

    std::vector<StoredPtr> mList;
    uint64_t mMutationVersion = 0;
#ifdef COMPONENT_THREAD_SAFE
    mutable std::recursive_mutex mMutex;
#endif
};

// ---- Inline implementations ----

template <typename C, typename StoredPtr>
PartList<C, StoredPtr>::PartList(const size_t initialAllocation)
    : Part(), mList()
#ifdef COMPONENT_THREAD_SAFE
      ,
      mMutex()
#endif
{
    mList.reserve(initialAllocation);
}

template <typename C, typename StoredPtr>
std::shared_ptr<C> PartList<C, StoredPtr>::LockPtr(const StoredPtr& ptr) const {
    return PtrTraits::Lock(ptr);
}

template <typename C, typename StoredPtr>
StoredPtr PartList<C, StoredPtr>::StorePtr(const std::shared_ptr<C>& ptr) const {
    return PtrTraits::Store(ptr);
}

template <typename C, typename StoredPtr> bool PartList<C, StoredPtr>::IsExpiredPtr(const StoredPtr& ptr) const {
    return PtrTraits::IsExpired(ptr);
}

template <typename C, typename StoredPtr> void PartList<C, StoredPtr>::PruneExpired() {
    const auto oldSize = mList.size();
    mList.erase(std::remove_if(mList.begin(), mList.end(), [this](const StoredPtr& ptr) { return IsExpiredPtr(ptr); }),
                mList.end());
    if (mList.size() != oldSize) {
        ++mMutationVersion;
    }
}

template <typename C, typename StoredPtr> bool PartList<C, StoredPtr>::ContainsIdUnlocked(const uint64_t id) const {
    return std::find_if(mList.begin(), mList.end(), [this, id](const StoredPtr& item) {
               auto locked = LockPtr(item);
               return locked && locked->GetId() == id;
           }) != mList.end();
}

template <typename C, typename StoredPtr>
bool PartList<C, StoredPtr>::EraseByIdUnlocked(const uint64_t id, std::shared_ptr<C>* removedPart) {
    auto it = std::find_if(mList.begin(), mList.end(), [this, id](const StoredPtr& item) {
        auto locked = LockPtr(item);
        return locked && locked->GetId() == id;
    });

    if (it == mList.end()) {
        return false;
    }

    if (removedPart != nullptr) {
        *removedPart = LockPtr(*it);
    }

    mList.erase(it);
    ++mMutationVersion;
    return true;
}

template <typename C, typename StoredPtr>
ListReturnCode PartList<C, StoredPtr>::Add(std::shared_ptr<C> part, const bool force) {
    if (!part) {
        return ListReturnCode::Failed;
    }

    const bool canAdd = CanAdd(part);
    if (!canAdd && !force) {
        return ListReturnCode::NotPermitted;
    }

    const bool forced = !canAdd && force;
    const uint64_t id = part->GetId();
    uint64_t opVersion = 0;

    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        PruneExpired();
        if (ContainsIdUnlocked(id)) {
            return ListReturnCode::Duplicate;
        }
        mList.push_back(StorePtr(part));
        opVersion = ++mMutationVersion;
    }

    bool shouldRunHooks = false;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        shouldRunHooks = (mMutationVersion == opVersion) && ContainsIdUnlocked(id);
    }

    if (!shouldRunHooks) {
        return forced ? ListReturnCode::ForcedSuccess : ListReturnCode::Success;
    }

    try {
        Added(part, forced);
        part->OnAdded(forced);
    } catch (...) {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        PruneExpired();
        EraseByIdUnlocked(id);
        return ListReturnCode::Failed;
    }

    return forced ? ListReturnCode::ForcedSuccess : ListReturnCode::Success;
}

template <typename C, typename StoredPtr>
ListReturnCode PartList<C, StoredPtr>::Add(const std::vector<std::shared_ptr<C>>& parts, const bool force) {
    if (parts.empty()) {
        return ListReturnCode::NoItemsProvided;
    }

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

template <typename C, typename StoredPtr>
ListReturnCode PartList<C, StoredPtr>::Remove(std::shared_ptr<C> part, const bool force) {
    if (!part) {
        return ListReturnCode::Failed;
    }

    const bool canRemove = CanRemove(part);
    if (!canRemove && !force) {
        return ListReturnCode::NotPermitted;
    }

    const bool forced = !canRemove && force;
    const uint64_t id = part->GetId();
    std::shared_ptr<C> removedPart = nullptr;
    uint64_t opVersion = 0;

    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        PruneExpired();
        if (!EraseByIdUnlocked(id, &removedPart)) {
            return ListReturnCode::NotFound;
        }
        opVersion = mMutationVersion;
    }

    if (!removedPart) {
        return ListReturnCode::NotFound;
    }

    bool shouldRunHooks = false;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        shouldRunHooks = (mMutationVersion == opVersion) && !ContainsIdUnlocked(id);
    }

    if (!shouldRunHooks) {
        return forced ? ListReturnCode::ForcedSuccess : ListReturnCode::Success;
    }

    try {
        Removed(removedPart, forced);
        removedPart->OnRemoved(forced);
    } catch (...) {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        PruneExpired();
        if (!ContainsIdUnlocked(id)) {
            mList.push_back(StorePtr(removedPart));
            ++mMutationVersion;
        }
        return ListReturnCode::Failed;
    }

    return forced ? ListReturnCode::ForcedSuccess : ListReturnCode::Success;
}

template <typename C, typename StoredPtr>
ListReturnCode PartList<C, StoredPtr>::Remove(const uint64_t id, const bool force) {
    std::shared_ptr<C> candidate = nullptr;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        PruneExpired();
        if (!ContainsIdUnlocked(id)) {
            return ListReturnCode::NotFound;
        }
        auto found = std::find_if(mList.begin(), mList.end(), [this, id](const StoredPtr& item) {
            auto locked = LockPtr(item);
            return locked && locked->GetId() == id;
        });
        candidate = found != mList.end() ? LockPtr(*found) : nullptr;
    }

    if (!candidate) {
        return ListReturnCode::NotFound;
    }

    const bool canRemove = CanRemove(candidate);
    if (!canRemove && !force) {
        return ListReturnCode::NotPermitted;
    }

    const bool forced = !canRemove && force;
    std::shared_ptr<C> removedPart = nullptr;
    uint64_t opVersion = 0;

    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        PruneExpired();
        if (!EraseByIdUnlocked(id, &removedPart)) {
            return ListReturnCode::NotFound;
        }
        opVersion = mMutationVersion;
    }

    if (!removedPart) {
        return ListReturnCode::NotFound;
    }

    bool shouldRunHooks = false;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        shouldRunHooks = (mMutationVersion == opVersion) && !ContainsIdUnlocked(id);
    }

    if (!shouldRunHooks) {
        return forced ? ListReturnCode::ForcedSuccess : ListReturnCode::Success;
    }

    try {
        Removed(removedPart, forced);
        removedPart->OnRemoved(forced);
    } catch (...) {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        PruneExpired();
        if (!ContainsIdUnlocked(id)) {
            mList.push_back(StorePtr(removedPart));
            ++mMutationVersion;
        }
        return ListReturnCode::Failed;
    }

    return forced ? ListReturnCode::ForcedSuccess : ListReturnCode::Success;
}

template <typename C, typename StoredPtr> ListReturnCode PartList<C, StoredPtr>::Remove(const bool force) {
    std::vector<std::shared_ptr<C>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        PruneExpired();
        if (mList.empty()) {
            return ListReturnCode::NotFound;
        }

        snapshot.reserve(mList.size());
        for (const auto& item : mList) {
            auto locked = LockPtr(item);
            if (locked) {
                snapshot.push_back(locked);
            }
        }
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

template <typename C, typename StoredPtr>
ListReturnCode PartList<C, StoredPtr>::Remove(const std::vector<std::shared_ptr<C>>& parts, const bool force) {
    if (parts.empty()) {
        return ListReturnCode::NoItemsProvided;
    }

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

template <typename C, typename StoredPtr>
template <typename T>
ListReturnCode PartList<C, StoredPtr>::Remove(const bool force) {
    std::vector<std::shared_ptr<C>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        PruneExpired();

        snapshot.reserve(mList.size());
        for (const auto& item : mList) {
            auto locked = LockPtr(item);
            if (locked && std::dynamic_pointer_cast<T>(locked) != nullptr) {
                snapshot.push_back(locked);
            }
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

template <typename C, typename StoredPtr>
ListReturnCode PartList<C, StoredPtr>::Remove(const std::vector<uint64_t>& ids, const bool force) {
    if (ids.empty()) {
        return ListReturnCode::NoItemsProvided;
    }

    std::vector<std::shared_ptr<C>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
        PruneExpired();

        snapshot.reserve(mList.size());
        for (const auto& item : mList) {
            auto locked = LockPtr(item);
            if (locked && std::find(ids.begin(), ids.end(), locked->GetId()) != ids.end()) {
                snapshot.push_back(locked);
            }
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

template <typename C, typename StoredPtr>
template <typename P>
typename std::enable_if<std::is_same<P, std::shared_ptr<C>>::value, std::vector<P>&>::type
PartList<C, StoredPtr>::GetList() {
    return mList;
}

template <typename C, typename StoredPtr>
template <typename P>
typename std::enable_if<std::is_same<P, std::shared_ptr<C>>::value, const std::vector<P>&>::type
PartList<C, StoredPtr>::GetList() const {
    return mList;
}

#ifdef COMPONENT_THREAD_SAFE
template <typename C, typename StoredPtr> std::recursive_mutex& PartList<C, StoredPtr>::GetMutex() const {
    return mMutex;
}
#endif

template <typename C, typename StoredPtr> bool PartList<C, StoredPtr>::CanAdd(std::shared_ptr<C> part) {
    return true;
}

template <typename C, typename StoredPtr> bool PartList<C, StoredPtr>::CanRemove(std::shared_ptr<C> part) {
    return true;
}

template <typename C, typename StoredPtr>
void PartList<C, StoredPtr>::Added(std::shared_ptr<C> part, const bool forced) {
}

template <typename C, typename StoredPtr>
void PartList<C, StoredPtr>::Removed(std::shared_ptr<C> part, const bool forced) {
}

template <typename C, typename StoredPtr> bool PartList<C, StoredPtr>::Has(std::shared_ptr<C> part) const {
    if (!part) {
        return false;
    }
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    const_cast<PartList<C, StoredPtr>*>(this)->PruneExpired();
    return ContainsIdUnlocked(part->GetId());
}

template <typename C, typename StoredPtr> template <typename T> bool PartList<C, StoredPtr>::Has() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    const_cast<PartList<C, StoredPtr>*>(this)->PruneExpired();
    return std::find_if(mList.begin(), mList.end(), [this](const StoredPtr& item) {
               auto locked = LockPtr(item);
               return locked && std::dynamic_pointer_cast<T>(locked) != nullptr;
           }) != mList.end();
}

template <typename C, typename StoredPtr> bool PartList<C, StoredPtr>::Has(const uint64_t id) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    const_cast<PartList<C, StoredPtr>*>(this)->PruneExpired();
    return ContainsIdUnlocked(id);
}

template <typename C, typename StoredPtr> bool PartList<C, StoredPtr>::Has() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    const_cast<PartList<C, StoredPtr>*>(this)->PruneExpired();
    return !mList.empty();
}

template <typename C, typename StoredPtr> size_t PartList<C, StoredPtr>::GetCount() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    const_cast<PartList<C, StoredPtr>*>(this)->PruneExpired();
    return mList.size();
}

template <typename C, typename StoredPtr> std::shared_ptr<C> PartList<C, StoredPtr>::Get(const uint64_t id) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    const_cast<PartList<C, StoredPtr>*>(this)->PruneExpired();
    auto it = std::find_if(mList.begin(), mList.end(), [this, id](const StoredPtr& item) {
        auto locked = LockPtr(item);
        return locked && locked->GetId() == id;
    });
    return it != mList.end() ? LockPtr(*it) : nullptr;
}

template <typename C, typename StoredPtr>
std::shared_ptr<std::vector<std::shared_ptr<C>>> PartList<C, StoredPtr>::Get() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    const_cast<PartList<C, StoredPtr>*>(this)->PruneExpired();
    auto result = std::make_shared<std::vector<std::shared_ptr<C>>>();
    result->reserve(mList.size());
    for (const auto& item : mList) {
        auto locked = LockPtr(item);
        if (locked) {
            result->push_back(locked);
        }
    }
    return result;
}

template <typename C, typename StoredPtr>
template <typename T>
std::shared_ptr<std::vector<std::shared_ptr<T>>> PartList<C, StoredPtr>::Get() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    const_cast<PartList<C, StoredPtr>*>(this)->PruneExpired();
    auto result = std::make_shared<std::vector<std::shared_ptr<T>>>();
    for (const auto& item : mList) {
        auto locked = LockPtr(item);
        if (!locked) {
            continue;
        }
        auto typed = std::dynamic_pointer_cast<T>(locked);
        if (typed) {
            result->push_back(typed);
        }
    }
    return result;
}

template <typename C, typename StoredPtr>
template <typename T>
std::shared_ptr<T> PartList<C, StoredPtr>::GetFirst() const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    const_cast<PartList<C, StoredPtr>*>(this)->PruneExpired();
    for (const auto& item : mList) {
        auto locked = LockPtr(item);
        if (!locked) {
            continue;
        }
        auto typed = std::dynamic_pointer_cast<T>(locked);
        if (typed) {
            return typed;
        }
    }
    return nullptr;
}

template <typename C, typename StoredPtr>
std::shared_ptr<std::vector<std::shared_ptr<C>>> PartList<C, StoredPtr>::Get(const std::vector<uint64_t>& ids) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    const_cast<PartList<C, StoredPtr>*>(this)->PruneExpired();
    auto result = std::make_shared<std::vector<std::shared_ptr<C>>>();
    for (const auto& item : mList) {
        auto locked = LockPtr(item);
        if (!locked) {
            continue;
        }
        if (std::find(ids.begin(), ids.end(), locked->GetId()) != ids.end()) {
            result->push_back(locked);
        }
    }
    return result;
}

} // namespace Ship
