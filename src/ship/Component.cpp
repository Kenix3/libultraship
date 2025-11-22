#include "ship/Component.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace Ship {
std::atomic_int Component::NextComponentId = 0;

Component::Component(const std::string& name)
    : mId(NextComponentId++), mName(name), mParents(), mChildren()
#ifdef INCLUDE_MUTEX
    , mMutex()
#endif
{
    SPDLOG_INFO("Constructing component {}", ToString());
}

Component::~Component() {
    SPDLOG_INFO("Destructing component {}", ToString());
    RemoveChildren(true);
    RemoveParents(true);
}

int32_t Component::GetId() const {
    return mId;
}

std::string Component::GetName() const {
    return mName;
}

std::string Component::ToString() const {
    return std::to_string(GetId()) + "-" + GetName() + "-" + typeid(*this).name();
}

Component::operator std::string() const {
    return ToString();
}

bool Component::operator==(const Component& other) const {
    return GetId() == other.GetId();
}

bool Component::HasParent(std::shared_ptr<Component> parent) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    auto iterator = std::find_if(list->begin(), list->end(),
                     [&parent](const std::shared_ptr<Component>& component) { return component == parent; });
    return iterator != list->end();
}

bool Component::HasChild(std::shared_ptr<Component> child) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    auto iterator = std::find_if(list->begin(), list->end(),
                                 [&child](const std::shared_ptr<Component>& component) { return component == child; });
    return iterator != mChildren.end();
}

template <typename T> bool Component::HasParent(const std::string name) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    auto iterator = std::find_if(list->begin(), list->end(),
         [&name](const std::shared_ptr<Component>& component) {
            return component->GetName() == name && std::dynamic_pointer_cast<T>(component) != nullptr;
         });
    return iterator != list->end();
}

template <typename T> bool Component::HasChild(const std::string name) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    auto iterator = std::find_if(list->begin(), list->end(), [&name](const std::shared_ptr<Component>& component) {
            return component->GetName() == name &&
                   std::dynamic_pointer_cast<T>(component) != nullptr;
        });
    return iterator != list->end();
}
template <typename T> bool Component::HasParent() {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    auto iterator = std::find_if(list->begin(), list->end(),
                                 [](const std::shared_ptr<Component>& component) { return std::dynamic_pointer_cast<T>(component) != nullptr; });
    return iterator != list->end();
}

template <typename T> bool Component::HasChild() {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    auto iterator = std::find_if(list->begin(), list->end(), [](const std::shared_ptr<Component>& component) {
        return std::dynamic_pointer_cast<T>(component) != nullptr;
    });
    return iterator != list->end();
}

bool Component::HasParent(const int32_t parent) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    auto iterator = std::find_if(list->begin(), list->end(),
                     [&parent](const std::shared_ptr<Component>& component) { return component->GetId() == parent;
    });
    return iterator != list->end();
}

bool Component::HasChild(const int32_t child) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    auto iterator = std::find_if(list->begin(), list->end(),
                     [&child](const std::shared_ptr<Component>& component) { return component->GetId() == child;
    });
    return iterator != list->end();
}

bool Component::HasParent(const std::string& parent) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    auto iterator = std::find_if(list->begin(), list->end(),
                     [&parent](const std::shared_ptr<Component>& component) { return component->GetName() == parent; });
    return iterator != list->end();
}

bool Component::HasChild(const std::string& child) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    auto iterator = std::find_if(list->begin(), list->end(),
                     [&child](const std::shared_ptr<Component>& component) { return component->GetName() == child; });
    return iterator != list->end();
}

bool Component::HasParent() {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return !mParents.empty();
}

bool Component::HasChild() {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return !mChildren.empty();
}

std::shared_ptr<Component> Component::GetParent(const int32_t parent) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif

    auto iterator = std::find_if(list->begin(), list->end(),
                     [&parent](const std::shared_ptr<Component>& component) { return component->GetId() == parent; });

    if (iterator == list->end()) {
        return nullptr;
    }

    return *iterator;
}

std::shared_ptr<Component> Component::GetChild(const int32_t child) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif

    auto iterator = std::find_if(list->begin(), list->end(),
                     [&child](const std::shared_ptr<Component>& component) { return component->GetId() == child; });

    if (iterator == list->end()) {
        return nullptr;
    }

    return *iterator;
}

std::shared_ptr<std::vector<std::shared_ptr<Component>>> Component::GetParents() {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return std::make_shared<std::vector<std::shared_ptr<Component>>>(mParents);
}

std::shared_ptr<std::vector<std::shared_ptr<Component>>> Component::GetChildren() {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return std::make_shared<std::vector<std::shared_ptr<Component>>>(mChildren);
}

size_t Component::GetParentsCount() {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return mParents.size();
}

size_t Component::GetChildrenCount() {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    return mChildren.size();
}

template <typename T> std::shared_ptr<T> Component::GetParents(const std::string& parent) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    auto val = std::make_shared<std::vector<T>>(list->size());
    for (const auto& parentObject : *list) {
        if (std::dynamic_pointer_cast<T>(parentObject) != nullptr && parentObject->GetName() == parent) {
            val.push_back(parentObject);
        }
    }
    return val;
}

template <typename T> std::shared_ptr<T> Component::GetChildren(const std::string& child) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    auto val = std::make_shared<std::vector<T>>(list->size());
    for (const auto& childObject : *list) {
        if (std::dynamic_pointer_cast<T>(childObject) != nullptr && childObject->GetName() == child) {
            val->push_back(childObject);
        }
    }
    return val;
}

template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> Component::GetParents() {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    auto val = std::make_shared<std::vector<T>>(list->size());
    for (const auto& parent : *list) {
        if (std::dynamic_pointer_cast<T>(parent) != nullptr) {
            val.push_back(parent);
        }
    }
    return val;
}

template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> Component::GetChildren() {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    auto val = std::make_shared<std::vector<T>>(list->size());
    for (const auto& child : *list) {
        if (std::dynamic_pointer_cast<T>(child) != nullptr) {
            val->push_back(child);
        }
    }
    return val;
}

std::shared_ptr<std::vector<std::shared_ptr<Component>>> Component::GetParents(const std::vector<int32_t>& parents) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    auto val = std::make_shared<std::vector<std::shared_ptr<Component>>>(list->size());
    for (const auto& parent : *list) {
        const auto search = parent->GetId();
        auto iterator =
            std::find_if(parents.begin(), parents.end(), [&search](const int& id) {
                return id == search;
            });

        if (iterator != parents.end()) {
            val->push_back(parent);
        }

    }
    return val;
}

std::shared_ptr<std::vector<std::shared_ptr<Component>>> Component::GetChildren(const std::vector<int32_t>& children) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    auto val = std::make_shared<std::vector<std::shared_ptr<Component>>>(list->size());
    for (const auto& child : *list) {
        const auto search = child->GetId();
        auto iterator =
            std::find_if(children.begin(), children.end(), [&search](const int& id) { return id == search; });

        if (iterator != children.end()) {
            val->push_back(child);
        }
    }
    return val;
}

std::shared_ptr<std::vector<std::shared_ptr<Component>>> Component::GetParents(const std::string& parent) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    auto val = std::make_shared<std::vector<std::shared_ptr<Component>>>(list->size());
    for (const auto& parentObject : *list) {
        if (parentObject->GetName() == parent) {
            val->push_back(parentObject);
        }
    }
    return val;
}

std::shared_ptr<std::vector<std::shared_ptr<Component>>> Component::GetChildren(const std::string& child) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    auto val = std::make_shared<std::vector<std::shared_ptr<Component>>>(list->size());
    for (const auto& childObject : *list) {
        if (childObject->GetName() == child) {
            val->push_back(childObject);
        }
    }
    return val;
}

std::shared_ptr<std::vector<std::shared_ptr<Component>>>
Component::GetParents(const std::vector<std::string>& parents) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    auto val = std::make_shared<std::vector<std::shared_ptr<Component>>>(list->size());
    for (const auto& parent : *list) {
        const auto search = parent->GetName();
        auto iterator =
            std::find_if(parents.begin(), parents.end(), [&search](const std::string& name) {
                return name == search;
            });

        if (iterator != parents.end()) {
            val->push_back(parent);
        }
    }
    return val;
}

std::shared_ptr<std::vector<std::shared_ptr<Component>>>
Component::GetChildren(const std::vector<std::string>& children) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    auto val = std::make_shared<std::vector<std::shared_ptr<Component>>>(list->size());
    for (const auto& child : *list) {
        const auto search = child->GetName();
        auto iterator = std::find_if(children.begin(), children.end(),
                                     [&search](const std::string& name) { return name == search; });

        if (iterator != children.end()) {
            val->push_back(child);
        }
    }
    return val;
}

bool Component::AddParent(std::shared_ptr<Component> parent, const bool force) {
    if (parent == nullptr) {
        return false;
    }

    const auto ptr = shared_from_this();
    const bool canAddParent = CanAddParent(parent);
    const bool canAddChild = parent->CanAddChild(ptr);
    const bool forced = (!canAddParent || !canAddChild) && force;
    {
#ifdef INCLUDE_MUTEX
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif

        if (HasParent(parent)) {
            return true;
        }

        if ((!canAddParent || !canAddChild) && !force) {
            return false;
        }

        AddParentRaw(parent);
    }
    AddedParent(parent, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be added as a parent to {}", parent->ToString(), ptr->ToString());
    }
    parent->AddChild(ptr, force);

    return true;
}

bool Component::AddChild(std::shared_ptr<Component> child, const bool force) {
    if (child == nullptr) {
        return false;
    }

    const auto ptr = shared_from_this();
    const bool canAddChild = CanAddChild(child);
    const bool canAddParent = child->CanAddParent(ptr);
    const bool forced = (!canAddParent || !canAddChild) && force;
    {
#ifdef INCLUDE_MUTEX
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif

        if (HasChild(child)) {
            return true;
        }

        if ((!canAddParent || !canAddChild) && !force) {
            return false;
        }

        AddChildRaw(child);
    }
    AddedChild(child, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be added as a child to {}", child->ToString(), ptr->ToString());
    }
    child->AddParent(ptr, force);

    return true;
}

bool Component::AddParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force) {
    bool val = true;
    for (const auto& parent : parents) {
        val &= AddParent(parent, force);
    }

    return val;
}

bool Component::AddChildren(const std::vector<std::shared_ptr<Component>>& children, const bool force) {
    bool val = true;
    for (const auto& child : children) {
        val &= AddChild(child, force);
    }

    return val;
}

bool Component::RemoveParent(std::shared_ptr<Component> parent, const bool force) {
    if (parent == nullptr) {
        return false;
    }

    const auto ptr = shared_from_this();
    const bool canRemoveParent = CanRemoveParent(parent);
    const bool canRemoveChild = parent->CanRemoveChild(ptr);
    const bool forced = (!canRemoveParent || !canRemoveChild) && force;
    {
#ifdef INCLUDE_MUTEX
        // This use of recursive mutex allows us to make sure duplicates are never added to the parent list
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif

        if (!HasParent(parent)) {
            return true;
        }

        if ((!canRemoveParent || !canRemoveChild) && !force) {
            return false;
        }

        RemoveParentRaw(parent);
    }
    RemovedParent(parent, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be removed as a parent to {}", parent->ToString(), ptr->ToString());
    }
    parent->RemoveChild(ptr, force);

    return true;
}

bool Component::RemoveChild(std::shared_ptr<Component> child, const bool force) {
    if (child == nullptr) {
        return false;
    }

    const auto ptr = shared_from_this();
    const bool canRemoveChild = CanRemoveChild(child);
    const bool canRemoveParent = child->CanRemoveParent(ptr);
    const bool forced = (!canRemoveParent || !canRemoveChild) && force;
    {
#ifdef INCLUDE_MUTEX
        // This use of recursive mutex allows us to make sure duplicates are never added to the children list
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif

        if (!HasChild(child)) {
            return true;
        }

        if ((!canRemoveParent || !canRemoveChild) && !force) {
            return false;
        }

        RemoveChildRaw(child);
    }
    RemovedChild(child, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be removed as a child to {}", child->ToString(), ptr->ToString());
    }
    child->RemoveParent(ptr, force);

    return true;
}

bool Component::RemoveParent(const int32_t parent, const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    for (const auto& parentObject : *list) {
        if (parentObject->GetId() == parent) {
            return RemoveParent(parentObject, force);
        }
    }
    return false;
}

bool Component::RemoveChild(const int32_t child, const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    for (const auto& childObject : *list) {
        if (childObject->GetId() == child) {
            return RemoveChild(childObject, force);
        }
    }
    return false;
}

bool Component::RemoveParents(const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    bool val = true;
    for (const auto& parent : *list) {
        val &= RemoveParent(parent, force);
    }

    return val;
}

bool Component::RemoveChildren(const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    bool val = true;
    for (const auto& child : *list) {
        val &= RemoveChild(child, force);
    }

    return val;
}

bool Component::RemoveParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force) {
    bool val = true;
    for (const auto& parent : parents) {
        val &= RemoveParent(parent, force);
    }

    return val;
}

bool Component::RemoveChildren(const std::vector<std::shared_ptr<Component>>& children, const bool force) {
    bool val = true;
    for (const auto& child : children) {
        val &= RemoveChild(child, force);
    }

    return val;
}

template <typename T> bool Component::RemoveParents(const std::string& name, const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    bool val = true;
    for (const auto& parent : *list) {
        if (std::dynamic_pointer_cast<T>(parent) != nullptr && parent->GetName() == name) {
            val &= RemoveParent(parent, force);
        }
    }
    return val;
}

template <typename T> bool Component::RemoveChildren(const std::string& name, const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    bool val = true;
    for (const auto& child : *list) {
        if (std::dynamic_pointer_cast<T>(child) != nullptr && child->GetName() == name) {
            val &= RemoveChild(child, force);
        }
    }
    return val;
}

template <typename T> bool Component::RemoveParents(const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    bool val = true;
    for (const auto& parent : *list) {
        if (std::dynamic_pointer_cast<T>(parent) != nullptr) {
            val &= RemoveParent(parent, force);
        }
    }
    return val;
}

template <typename T> bool Component::RemoveChildren(const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    bool val = true;
    for (const auto& child : *list) {
        if (std::dynamic_pointer_cast<T>(child) != nullptr) {
            val &= RemoveChild(child, force);
        }
    }
    return val;
}

bool Component::RemoveParents(const std::vector<int32_t>& parents, const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    bool val = true;
    for (const auto& parent : *list) {
        const auto search = parent->GetId();
        auto iterator =
            std::find_if(parents.begin(), parents.end(), [&search](const int32_t& id) {
                return id == search;
            });

        if (iterator != parents.end()) {
            val &= RemoveParent(parent, force);
        }
    }
    return val;
}

bool Component::RemoveChildren(const std::vector<int32_t>& children, const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    bool val = true;
    for (const auto& child : *list) {
        const auto search = child->GetId();
        auto iterator =
            std::find_if(children.begin(), children.end(), [&search](const int32_t& id) { return id == search; });

        if (iterator != children.end()) {
            val &= RemoveChild(child, force);
        }
    }
    return val;
}

bool Component::RemoveParents(const std::string& parent, const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    bool val = true;
    for (const auto& parentObject : *list) {
        if (parentObject->GetName() == parent) {
            val &= RemoveParent(parentObject, force);
        }
    }
    return val;
}

bool Component::RemoveChildren(const std::string& child, const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    bool val = true;
    for (const auto& childObject : *list) {
        if (childObject->GetName() == child) {
            val &= RemoveChild(childObject, force);
        }
    }
    return val;
}

bool Component::RemoveParents(const std::vector<std::string>& parents, const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetParents();
    list = sharedList.get();
#else
    list = &mParents;
#endif
    bool val = true;
    for (const auto& parent : *list) {
        const auto search = parent->GetName();
        auto iterator =
            std::find_if(parents.begin(), parents.end(), [&search](const std::string& name) {
                return name == search;
            });

        if (iterator != parents.end()) {
            val &= RemoveParent(parent, force);
        }
    }
    return val;
}

bool Component::RemoveChildren(const std::vector<std::string>& children, const bool force) {
    std::vector<std::shared_ptr<Component>>* list;
#ifdef INCLUDE_MUTEX
    auto sharedList = GetChildren();
    list = sharedList.get();
#else
    list = &mChildren;
#endif
    bool val = true;
    for (const auto& child : *list) {
        const auto search = child->GetName();
        auto iterator = std::find_if(children.begin(), children.end(),
                                     [&search](const std::string& name) { return name == search; });

        if (iterator != children.end()) {
            val &= RemoveChild(child, force);
        }
    }
    return val;
}

bool Component::CanAddParent(std::shared_ptr<Component> parent) {
    return true;
}

bool Component::CanAddChild(std::shared_ptr<Component> child) {
    return true;
}

bool Component::CanRemoveParent(std::shared_ptr<Component> parent) {
    return true;
}

bool Component::CanRemoveChild(std::shared_ptr<Component> child) {
    return true;
}

void Component::AddedParent(std::shared_ptr<Component> parent, const bool forced) {

}

void Component::AddedChild(std::shared_ptr<Component> child, const bool forced) {

}

void Component::RemovedParent(std::shared_ptr<Component> parent, const bool forced) {

}

void Component::RemovedChild(std::shared_ptr<Component> child, const bool forced) {

}

Component& Component::AddParentRaw(std::shared_ptr<Component> parent) {
    SPDLOG_INFO("Adding parent {} to this component {}", static_cast<std::string>(*parent), static_cast<std::string>(*this));
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    mParents.push_back(parent);
    return *this;
}

Component& Component::AddChildRaw(std::shared_ptr<Component> child) {
    SPDLOG_INFO("Adding child {} to this component {}", static_cast<std::string>(*child),
                static_cast<std::string>(*this));
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    mChildren.push_back(child);
    return *this;
}

Component& Component::RemoveParentRaw(std::shared_ptr<Component> parent) {
    SPDLOG_INFO("Removing parent {} from this component {}", static_cast<std::string>(*parent),
                static_cast<std::string>(*this));
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    auto removed = std::remove_if(mParents.begin(), mParents.end(), [&parent](const std::shared_ptr<Component>& component) { return parent == component;
    });
    mParents.erase(removed, mParents.end());
    return *this;
}

Component& Component::RemoveChildRaw(std::shared_ptr<Component> child) {
    SPDLOG_INFO("Adding child {} from this component {}", static_cast<std::string>(*child),
                static_cast<std::string>(*this));
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif
    auto removed =
        std::remove_if(mChildren.begin(), mChildren.end(),
                       [&child](const std::shared_ptr<Component>& component) { return child == component; });
    mChildren.erase(removed, mChildren.end());
    return *this;
}
} // namespace Ship
