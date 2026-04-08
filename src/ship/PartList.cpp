#include "ship/PartList.h"

#include <shared_mutex>

#include <spdlog/spdlog.h>

#include "Ship/Component.h"

namespace Ship {

template <DerivedFromPart C>
PartList<C>::PartList(std::shared_ptr<Component> parent, const size_t initialAllocation)
    : Part(), mParent(parent), mList(initialAllocation) {
    if (GetParent() == nullptr) {
        SPDLOG_CRITICAL("Instantiated a PartList ({}) with a null parent. This will cause crashes later.", GetId());
    }
}

template <DerivedFromPart C> std::shared_ptr<Component> PartList<C>::GetParent() const {
    return mParent;
}

template <DerivedFromPart C> bool PartList<C>::Has(std::shared_ptr<C> part) MUTEX_CONST {
#ifdef INCLUDE_MUTEX
    std::shared_lock lock(GetParent()->GetMutex());
#endif
    auto iterator = std::find_if(GetList()->begin(), GetList()->end(),
                                 [&part](const std::shared_ptr<Part>& comparison) { return comparison == part; });
    return iterator != GetList()->end();
}

template <DerivedFromPart C> template <typename T> bool PartList<C>::Has() MUTEX_CONST {
#ifdef INCLUDE_MUTEX
    std::shared_lock lock(GetParent()->GetMutex());
#endif
    auto iterator = std::find_if(GetList()->begin(), GetList()->end(), [](const std::shared_ptr<Part>& comparison) {
        return std::dynamic_pointer_cast<T>(comparison) != nullptr;
    });
    return iterator != GetList()->end();
}

template <DerivedFromPart C> bool PartList<C>::Has(const uint64_t id) MUTEX_CONST {
#ifdef INCLUDE_MUTEX
    std::shared_lock lock(GetParent()->GetMutex());
#endif
    auto iterator =
        std::find_if(GetList()->begin(), GetList()->end(),
                     [&id](const std::shared_ptr<Part>& comparison) {
        return comparison->GetId() == id;
    });
    return iterator != GetList()->end();
}

template <DerivedFromPart C> bool PartList<C>::Has() MUTEX_CONST {
#ifdef INCLUDE_MUTEX
    std::shared_lock lock(GetParent()->GetMutex());
#endif
    return !mList.empty();
}

template <DerivedFromPart C> size_t PartList<C>::GetCount() MUTEX_CONST {
#ifdef INCLUDE_MUTEX
    std::shared_lock lock(GetParent()->GetMutex());
#endif #endif
    return mList.size();
}

template <DerivedFromPart C> std::shared_ptr<C> PartList<C>::Get(const uint64_t id) MUTEX_CONST {
#ifdef INCLUDE_MUTEX
    std::shared_lock lock(GetParent()->GetMutex());
#endif

    auto iterator =
        std::find_if(GetList()->begin(), GetList()->end(),
                     [&id](const std::shared_ptr<Part>& comparison) { return comparison->GetId() == id;
    });

    if (iterator == GetList()->end()) {
        return nullptr;
    }

    return *iterator;
}

template <DerivedFromPart C> std::shared_ptr<std::vector<std::shared_ptr<C>>> PartList<C>::Get() MUTEX_CONST {
#ifdef INCLUDE_MUTEX
    std::shared_lock lock(GetParent()->GetMutex());
#endif
    return std::make_shared<std::vector<std::shared_ptr<Part>>>(mList);
}

template <DerivedFromPart C>
template <typename T>
std::shared_ptr<std::vector<std::shared_ptr<T>>> PartList<C>::Get() MUTEX_CONST {
#ifdef INCLUDE_MUTEX
    std::shared_lock lock(GetParent()->GetMutex());
#endif
    auto val = std::make_shared<std::vector<T>>(GetCount());
    for (const auto& listObject : *GetList()) {
        if (std::dynamic_pointer_cast<T>(listObject) != nullptr) {
            val.push_back(listObject);
        }
    }
    return val;
}

template <DerivedFromPart C>
std::shared_ptr<std::vector<std::shared_ptr<C>>> PartList<C>::Get(const std::vector<uint64_t>& ids) MUTEX_CONST {
#ifdef INCLUDE_MUTEX
    std::shared_lock lock(GetParent()->GetMutex());
#endif
    auto val = std::make_shared<std::vector<std::shared_ptr<Component>>>(GetCount());
    for (const auto& listObject : *GetList()) {
        const auto search = listObject->GetId();
        auto iterator = std::find_if(ids.begin(), ids.end(), [&search](const uint64_t& id) { return id == search; });

        if (iterator != ids.end()) {
            val->push_back(listObject);
        }
    }
    return val;
}

template <DerivedFromPart C> ListReturnCode PartList<C>::Add(std::shared_ptr<C> part) {
#ifdef INCLUDE_MUTEX
    std::unique_lock lock(GetParent()->GetMutex());
#endif

    // Can not reuse HasPart because a shared_mutex is not a recursive mutex.
    auto iterator = std::find_if(GetList()->begin(), GetList()->end(),
                                 [&part](const std::shared_ptr<Part>& comparison) { return comparison == part; });

    if (iterator != GetList()->end()) {
        return ListReturnCode::Duplicate;
    }

    GetList()->push_back(part);
    return ListReturnCode::Success;
}

template <DerivedFromPart C> ListReturnCode PartList<C>::Add(const std::vector<std::shared_ptr<C>>& parts) {
    auto returnValue = ListReturnCode::Unknown;
    for (const auto& part : parts) {
        const auto addReturned = Add(part);
        SPDLOG_DEBUG("Add returned {} in list {}", addReturned, GetId());

        // If we are in a success scenario, the return value should always be the highest error code.
        // Additionally, never turn a failure scenario back to a success.
        if (returnValue >= ListReturnCode::Duplicate && addReturned >= ListReturnCode::Duplicate &&
            addReturned > returnValue) {
            returnValue = addReturned;
        }

        // If we are in a failure scenario, the return value should always be the lowest error code.
        if (addReturned < ListReturnCode::Duplicate && addReturned < returnValue) {
            returnValue = addReturned;
        }
    }

    return returnValue;
}

template <DerivedFromPart C> ListReturnCode PartList<C>::Remove(std::shared_ptr<C> part) {
#ifdef INCLUDE_MUTEX
    std::unique_lock lock(GetParent()->GetMutex());
#endif

    // Can not reuse HasPart because a shared_mutex is not a recursive mutex.
    auto iterator = std::find_if(GetList()->begin(), GetList()->end(),
                                 [&part](const std::shared_ptr<Part>& comparison) { return comparison == part; });
    const bool hasPart = iterator != GetList()->end();

    if (!hasPart) {
        return ListReturnCode::NotFound;
    }

    auto removed =
        std::remove_if(GetList()->begin(), GetList()->end(),
                       [&part](const std::shared_ptr<Component>& component) { return part == component; });
    GetList()->erase(removed, GetList()->end());

    return ListReturnCode::Success;
}

template <DerivedFromPart C> ListReturnCode PartList<C>::Remove(const uint64_t id) {
    
}

template <DerivedFromPart C> ListReturnCode PartList<C>::Remove() {

}

template <DerivedFromPart C> ListReturnCode PartList<C>::Remove(const std::vector<std::shared_ptr<C>>& parts) {

}

template <DerivedFromPart C> template <typename T> ListReturnCode PartList<C>::Remove() {
    
}

template <DerivedFromPart C> ListReturnCode PartList<C>::Remove(const std::vector<uint64_t>& ids) {
    
}

template <DerivedFromPart C> std::vector<std::shared_ptr<C>>& PartList<C>::GetList() const {
    return mList;
}
} // namespace Ship