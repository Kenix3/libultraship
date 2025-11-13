#include "ship/Component.h"

#include "spdlog/spdlog.h"


namespace Ship {
std::atomic_int Component::NextComponentId = 0;

Component::Component(const std::string& name)
    : mId(NextComponentId++), mName(name), mParents(), mChildren()
#ifdef INCLUDE_MUTEX
    , mMutex()
#endif
{
    SPDLOG_INFO("Constructing component {}", static_cast<std::string>(*this));
}

Component::~Component() {
    SPDLOG_INFO("Destructing component {}", static_cast<std::string>(*this));
}

bool Component::DrawDebugMenu() {
    // TODO: Not implemented.
    // The idea is that we display generic properties of the Component, then call the overridable method which will draw the specific stuff.
    // Then we create a collapsable box to show all children and recursively call DrawDebugMenu
    // There is probably going to be NO protection for cyclical references. If this happens, you will crash via recursion.

    return true;
}

int Component::GetId() const {
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

std::shared_ptr<Component> Component::GetParent(const std::string& parent) {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    if (HasParent(parent)) {
        return mParents[parent];
    }
    return nullptr;
}

std::shared_ptr<Component> Component::GetChild(const std::string& child) {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    if (HasChild(child)) {
        return mChildren[child];
    }
    return nullptr;
}

std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Component>>> Component::GetParents() {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    return std::make_shared < std::unordered_map < std::string, std::shared_ptr < Component>>>(mParents);
}

std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Component>>> Component::GetChildren() {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    return std::make_shared<std::unordered_map<std::string, std::shared_ptr<Component>>>(mChildren);
}

bool Component::HasParent(std::shared_ptr<Component> parent) {
    return HasParent(parent->GetName());
}

bool Component::HasParent(const std::string& parent) {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    return mParents.contains(parent);
}

bool Component::HasParent() {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    return !mParents.empty();
}

bool Component::HasChild(std::shared_ptr<Component> child) {
    return HasChild(child->GetName());
}
bool Component::HasChild(const std::string& child) {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    return mChildren.contains(child);
}

bool Component::HasChild() {
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    return !mChildren.empty();
}

bool Component::AddParent(std::shared_ptr<Component> parent, const bool force) {
    if (parent == nullptr) {
        return false;
    }

    if (HasParent(parent->GetName())) {
        return true;
    }

    auto ptr = shared_from_this();
    const bool canAddParent = CanAddParent(parent);
    const bool canAddChild = parent->CanAddChild(ptr);
    if ((!canAddParent || !canAddChild) && !force) {
        return false;
    }

    const bool forced = (!canAddParent || !canAddChild) && force;
    AddParentRaw(parent);
    AddedParent(parent, forced);
    parent->AddChild(ptr, force);

    return true;
}

bool Component::AddChild(std::shared_ptr<Component> child, const bool force) {
    if (child == nullptr) {
        return false;
    }

    if (HasChild(child->GetName())) {
        return true;
    }

    auto ptr = shared_from_this();
    const bool canAddChild = CanAddChild(child);
    const bool canAddParent = child->CanAddParent(ptr);
    if ((!canAddParent || !canAddChild) && !force) {
        return false;
    }

    const bool forced = (!canAddParent || !canAddChild) && force;
    AddChildRaw(child);
    AddedChild(child, forced);
    child->AddParent(ptr, force);

    return true;
}

bool Component::RemoveParent(std::shared_ptr<Component> parent, bool force) {
    if (parent == nullptr) {
        return false;
    }

    if (!HasParent(parent->GetName())) {
        return true;
    }

    auto ptr = shared_from_this();
    const bool canRemoveParent = CanRemoveParent(parent);
    const bool canRemoveChild = parent->CanRemoveChild(ptr);
    if ((!canRemoveParent || !canRemoveChild) && !force) {
        return false;
    }

    const bool forced = (!canRemoveParent || !canRemoveChild) && force;
    RemoveParentRaw(parent);
    RemovedParent(parent, forced);
    parent->RemoveChild(ptr, force);

    return true;
}

bool Component::RemoveChild(std::shared_ptr<Component> child, bool force) {
    if (child == nullptr) {
        return false;
    }

    if (!HasChild(child->GetName())) {
        return true;
    }

    auto ptr = shared_from_this();
    const bool canRemoveChild = CanRemoveChild(child);
    const bool canRemoveParent = child->CanRemoveParent(ptr);
    if ((!canRemoveParent || !canRemoveChild) && !force) {
        return false;
    }

    const bool forced = (!canRemoveParent || !canRemoveChild) && force;
    RemoveChildRaw(child);
    RemovedChild(child, forced);
    child->RemoveParent(ptr, force);

    return true;
}

bool Component::RemoveParent(const std::string& parent, bool force) {
    return RemoveParent(parent);
}

bool Component::RemoveChild(const std::string& child, bool force) {
    return RemoveChild(child);
}

bool Component::AddParents(const std::unordered_map<std::string, std::shared_ptr<Component>>& parents, bool force) {
    bool val = true;
    for (const auto& pair : parents) {
        val &= AddParent(pair.second, force);
    }

    return val;
}

bool Component::AddChildren(const std::unordered_map<std::string, std::shared_ptr<Component>>& children,
                                  bool force) {
    bool val = true;
    for (const auto& pair : children) {
        val &= AddChild(pair.second, force);
    }

    return val;
}

bool Component::AddParents(const std::vector<std::shared_ptr<Component>>& parents, bool force) {
    bool val = true;
    for (const auto& parent : parents) {
        val &= AddParent(parent, force);
    }

    return val;
}

bool Component::AddChildren(const std::vector<std::shared_ptr<Component>>& children, bool force) {
    bool val = true;
    for (const auto& child : children) {
        val &= AddChild(child, force);
    }

    return val;
}

bool Component::RemoveParents(const std::unordered_map<std::string, std::shared_ptr<Component>>& parents,
                                    bool force) {
    bool val = true;
    for (const auto& pair : parents) {
        val &= RemoveParent(pair.second, force);
    }

    return val;
}

bool Component::RemoveChildren(const std::unordered_map<std::string, std::shared_ptr<Component>>& children,
                                     bool force) {
    bool val = true;
    for (const auto& pair : children) {
        val &= RemoveChild(pair.second, force);
    }

    return val;
}

bool Component::RemoveParents(const std::vector<std::shared_ptr<Component>>& parents, bool force) {
    bool val = true;
    for (const auto& parent : parents) {
        val &= RemoveParent(parent, force);
    }

    return val;
}

bool Component::RemoveChildren(const std::vector<std::shared_ptr<Component>>& children, bool force) {
    bool val = true;
    for (const auto& child : children) {
        val &= RemoveChild(child, force);
    }

    return val;
}

bool Component::RemoveParents(const std::vector<std::string>& parents, bool force) {
    bool val = true;
    for (const auto& parent : parents) {
        val &= RemoveParent(parent, force);
    }

    return val;
}

bool Component::RemoveChildren(const std::vector<std::string>& children, bool force) {
    bool val = true;
    for (const auto& child : children) {
        val &= RemoveChild(child, force);
    }

    return val;
}

bool Component::RemoveParents(bool force) {
    auto parents = GetParents();
    bool val = true;
    for (const auto& pair : parents) {
        val &= RemoveParent(pair.second, force);
    }

    return val;
}

bool Component::RemoveChildren(bool force) {
    auto children = GetChildren();
    bool val = true;
    for (const auto& pair : children) {
        RemoveParent(pair.second, force);
    }

    return val;
}

Component& Component::AddParentRaw(std::shared_ptr<Component> parent) {
    SPDLOG_INFO("Adding parent {} to this component {}", static_cast<std::string>(*parent), static_cast<std::string>(*this));
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    mParents[parent->GetName()] = parent;
    return *this;
}

Component& Component::AddChildRaw(std::shared_ptr<Component> child) {
    SPDLOG_INFO("Adding parent {} to this component {}", static_cast<std::string>(*child),
                static_cast<std::string>(*this));
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    mChildren[child->GetName()] = child;
    return *this;
}

Component& Component::RemoveParentRaw(std::shared_ptr<Component> parent) {
    SPDLOG_INFO("Removing parent {} from this component {}", static_cast<std::string>(*parent),
                static_cast<std::string>(*this));
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    mParents.erase(parent->GetName());
    return *this;
}

Component& Component::RemoveChildRaw(std::shared_ptr<Component> child) {
    SPDLOG_INFO("Adding child {} from this component {}", static_cast<std::string>(*child),
                static_cast<std::string>(*this));
#ifdef INCLUDE_MUTEX
    const std::lock_guard<std::mutex> lock(mMutex);
#endif
    mChildren.erase(child->GetName());
    return *this;
}
} // namespace Ship
