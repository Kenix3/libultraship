#pragma once

#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include <algorithm>
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
    ForcedSuccess = 2,   /**< @brief Operation succeeded via force override. */
    Success = 1,         /**< @brief Operation succeeded normally. */
    Duplicate = 0,       /**< @brief Part already present; list unchanged. */
    NoItemsProvided = -1,/**< @brief The input collection was empty. */
    NotPermitted = -2,   /**< @brief Operation blocked by a permission check. */
    NotFound = -3,       /**< @brief The specified Part was not found. */
    Failed = -4          /**< @brief General failure (e.g. null pointer). */
};

/**
 * @brief A non-thread-safe ordered list of Parts.
 *
 * Provides add, remove, and lookup operations for a collection of Part-derived
 * objects stored as shared_ptr. Thread safety is the caller's responsibility.
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

    /**
     * @brief Checks whether a Part with the given name is in the list.
     * @param name The name to search for.
     * @return True if found.
     */
    bool Has(const std::string& name) const;

    /**
     * @brief Checks whether a Part of type T with the given name is in the list.
     * @tparam T The derived type to match via dynamic_cast.
     * @param name The name to search for.
     * @return True if a matching Part is found.
     */
    template <typename T> bool Has(const std::string& name) const;

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
     * @return ListReturnCode indicating the result.
     */
    ListReturnCode Add(std::shared_ptr<C> part);

    /**
     * @brief Adds multiple Parts to the list.
     * @param parts The Parts to add.
     * @return The aggregate ListReturnCode for the batch operation.
     */
    ListReturnCode Add(const std::vector<std::shared_ptr<C>>& parts);

    /**
     * @brief Removes a specific Part from the list.
     * @param part The Part to remove.
     * @return ListReturnCode indicating the result.
     */
    ListReturnCode Remove(std::shared_ptr<C> part);

    /**
     * @brief Removes a Part by its unique ID.
     * @param id The Part ID to remove.
     * @return ListReturnCode indicating the result.
     */
    ListReturnCode Remove(const uint64_t id);

    /** @brief Removes all Parts from the list. */
    ListReturnCode Remove();

    /**
     * @brief Removes multiple Parts from the list.
     * @param parts The Parts to remove.
     * @return The aggregate ListReturnCode for the batch operation.
     */
    ListReturnCode Remove(const std::vector<std::shared_ptr<C>>& parts);

    /**
     * @brief Removes all Parts that can be dynamic_cast to type T.
     * @tparam T The derived type to match for removal.
     * @return ListReturnCode indicating the result.
     */
    template <typename T> ListReturnCode Remove();

    /**
     * @brief Removes all Parts whose IDs appear in the given vector.
     * @param ids The IDs to remove.
     * @return The aggregate ListReturnCode for the batch operation.
     */
    ListReturnCode Remove(const std::vector<uint64_t>& ids);

    /**
     * @brief Direct access to the underlying vector for efficient iteration.
     *
     * Thread safety is the caller's responsibility.
     * @return A mutable reference to the internal vector.
     */
    std::vector<std::shared_ptr<C>>& GetList();

    /** @brief Direct const access to the underlying vector. */
    const std::vector<std::shared_ptr<C>>& GetList() const;

  private:
    std::vector<std::shared_ptr<C>> mList;
};

// ---- Inline implementations ----

template <typename C> PartList<C>::PartList(const size_t initialAllocation) : Part(), mList() {
    mList.reserve(initialAllocation);
}

template <typename C> bool PartList<C>::Has(std::shared_ptr<C> part) const {
    if (!part) {
        return false;
    }
    const auto& list = GetList();
    return std::find_if(list.begin(), list.end(), [&part](const std::shared_ptr<C>& item) {
               return item->GetId() == part->GetId();
           }) != list.end();
}

template <typename C> template <typename T> bool PartList<C>::Has() const {
    const auto& list = GetList();
    return std::find_if(list.begin(), list.end(), [](const std::shared_ptr<C>& item) {
               return std::dynamic_pointer_cast<T>(item) != nullptr;
           }) != list.end();
}

template <typename C> bool PartList<C>::Has(const uint64_t id) const {
    const auto& list = GetList();
    return std::find_if(list.begin(), list.end(),
                        [id](const std::shared_ptr<C>& item) { return item->GetId() == id; }) != list.end();
}

template <typename C> bool PartList<C>::Has(const std::string& name) const {
    const auto& list = GetList();
    return std::find_if(list.begin(), list.end(),
                        [&name](const std::shared_ptr<C>& item) { return item->GetName() == name; }) != list.end();
}

template <typename C> template <typename T> bool PartList<C>::Has(const std::string& name) const {
    const auto& list = GetList();
    return std::find_if(list.begin(), list.end(), [&name](const std::shared_ptr<C>& item) {
               return item->GetName() == name && std::dynamic_pointer_cast<T>(item) != nullptr;
           }) != list.end();
}

template <typename C> bool PartList<C>::Has() const {
    return !mList.empty();
}

template <typename C> size_t PartList<C>::GetCount() const {
    return mList.size();
}

template <typename C> std::shared_ptr<C> PartList<C>::Get(const uint64_t id) const {
    const auto& list = GetList();
    auto it =
        std::find_if(list.begin(), list.end(), [id](const std::shared_ptr<C>& item) { return item->GetId() == id; });
    return it != list.end() ? *it : nullptr;
}

template <typename C> std::shared_ptr<std::vector<std::shared_ptr<C>>> PartList<C>::Get() const {
    return std::make_shared<std::vector<std::shared_ptr<C>>>(mList);
}

template <typename C> template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> PartList<C>::Get() const {
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
    auto result = std::make_shared<std::vector<std::shared_ptr<C>>>();
    for (const auto& item : mList) {
        const uint64_t id = item->GetId();
        if (std::find(ids.begin(), ids.end(), id) != ids.end()) {
            result->push_back(item);
        }
    }
    return result;
}

template <typename C> ListReturnCode PartList<C>::Add(std::shared_ptr<C> part) {
    if (!part) {
        return ListReturnCode::Failed;
    }
    if (Has(part)) {
        return ListReturnCode::Duplicate;
    }
    mList.push_back(part);
    return ListReturnCode::Success;
}

template <typename C> ListReturnCode PartList<C>::Add(const std::vector<std::shared_ptr<C>>& parts) {
    if (parts.empty()) {
        return ListReturnCode::NoItemsProvided;
    }
    ListReturnCode result = ListReturnCode::Duplicate;
    for (const auto& part : parts) {
        const ListReturnCode r = Add(part);
        if (static_cast<int32_t>(r) > static_cast<int32_t>(result)) {
            result = r;
        } else if (static_cast<int32_t>(r) < static_cast<int32_t>(ListReturnCode::Duplicate) &&
                   static_cast<int32_t>(r) < static_cast<int32_t>(result)) {
            result = r;
        }
    }
    return result;
}

template <typename C> ListReturnCode PartList<C>::Remove(std::shared_ptr<C> part) {
    if (!part) {
        return ListReturnCode::Failed;
    }
    auto& list = GetList();
    auto it = std::find_if(list.begin(), list.end(),
                           [&part](const std::shared_ptr<C>& item) { return item->GetId() == part->GetId(); });
    if (it == list.end()) {
        return ListReturnCode::NotFound;
    }
    list.erase(it);
    return ListReturnCode::Success;
}

template <typename C> ListReturnCode PartList<C>::Remove(const uint64_t id) {
    auto& list = GetList();
    auto it =
        std::find_if(list.begin(), list.end(), [id](const std::shared_ptr<C>& item) { return item->GetId() == id; });
    if (it == list.end()) {
        return ListReturnCode::NotFound;
    }
    list.erase(it);
    return ListReturnCode::Success;
}

template <typename C> ListReturnCode PartList<C>::Remove() {
    if (mList.empty()) {
        return ListReturnCode::NotFound;
    }
    mList.clear();
    return ListReturnCode::Success;
}

template <typename C> ListReturnCode PartList<C>::Remove(const std::vector<std::shared_ptr<C>>& parts) {
    if (parts.empty()) {
        return ListReturnCode::NoItemsProvided;
    }
    ListReturnCode result = ListReturnCode::NotFound;
    for (const auto& part : parts) {
        const ListReturnCode r = Remove(part);
        if (static_cast<int32_t>(r) > static_cast<int32_t>(result)) {
            result = r;
        } else if (static_cast<int32_t>(r) < static_cast<int32_t>(ListReturnCode::Duplicate) &&
                   static_cast<int32_t>(r) < static_cast<int32_t>(result)) {
            result = r;
        }
    }
    return result;
}

template <typename C> template <typename T> ListReturnCode PartList<C>::Remove() {
    auto& list = GetList();
    const auto it = std::remove_if(list.begin(), list.end(), [](const std::shared_ptr<C>& item) {
        return std::dynamic_pointer_cast<T>(item) != nullptr;
    });
    if (it == list.end()) {
        return ListReturnCode::NotFound;
    }
    list.erase(it, list.end());
    return ListReturnCode::Success;
}

template <typename C> ListReturnCode PartList<C>::Remove(const std::vector<uint64_t>& ids) {
    if (ids.empty()) {
        return ListReturnCode::NoItemsProvided;
    }
    auto& list = GetList();
    const auto it = std::remove_if(list.begin(), list.end(), [&ids](const std::shared_ptr<C>& item) {
        return std::find(ids.begin(), ids.end(), item->GetId()) != ids.end();
    });
    if (it == list.end()) {
        return ListReturnCode::NotFound;
    }
    list.erase(it, list.end());
    return ListReturnCode::Success;
}

template <typename C> std::vector<std::shared_ptr<C>>& PartList<C>::GetList() {
    return mList;
}

template <typename C> const std::vector<std::shared_ptr<C>>& PartList<C>::GetList() const {
    return mList;
}

} // namespace Ship
