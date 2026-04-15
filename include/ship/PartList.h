#pragma once

#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include <algorithm>
#include <stdint.h>

#include "ship/Part.h"

namespace Ship {

// >= 0 means the Part is guaranteed to be in the list after running.
// < 0 is an error scenario.
// > 0 means a Part was actually added to the list.
// == 0 means the list is unchanged (duplicate).
enum class ListReturnCode : int32_t {
    ForcedSuccess = 2,
    Success = 1,
    Duplicate = 0,
    NoItemsProvided = -1,
    NotPermitted = -2,
    NotFound = -3,
    Failed = -4
};

// PartList is a non-thread-safe ordered list.
// Thread safety is the caller's responsibility.
// C must be derived from Part.
template <typename C = Part> class PartList : public Part {
  public:
    explicit PartList(const size_t initialAllocation = 0);
    virtual ~PartList() = default;

    bool Has(std::shared_ptr<C> part) const;
    template <typename T> bool Has() const;
    bool Has(const uint64_t id) const;
    bool Has(const std::string& name) const;
    template <typename T> bool Has(const std::string& name) const;
    bool Has() const;
    size_t GetCount() const;

    std::shared_ptr<C> Get(const uint64_t id) const;
    std::shared_ptr<std::vector<std::shared_ptr<C>>> Get() const;
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> Get() const;
    std::shared_ptr<std::vector<std::shared_ptr<C>>> Get(const std::vector<uint64_t>& ids) const;

    // Returns the first item in the list that can be dynamic_cast to T, or nullptr.
    template <typename T> std::shared_ptr<T> GetFirst() const;

    ListReturnCode Add(std::shared_ptr<C> part);
    ListReturnCode Add(const std::vector<std::shared_ptr<C>>& parts);
    ListReturnCode Remove(std::shared_ptr<C> part);
    ListReturnCode Remove(const uint64_t id);
    ListReturnCode Remove();
    ListReturnCode Remove(const std::vector<std::shared_ptr<C>>& parts);
    template <typename T> ListReturnCode Remove();
    ListReturnCode Remove(const std::vector<uint64_t>& ids);

    // Direct access to the underlying vector for efficient iteration.
    // Thread safety is the caller's responsibility.
    std::vector<std::shared_ptr<C>>& GetList();
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
