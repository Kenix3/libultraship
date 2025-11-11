#include "ship/Component.h"

#include "spdlog/spdlog.h"


namespace Ship {
std::atomic_int Component::NextComponentId = 0;

Component::Component(const std::string& name)
    : mId(NextComponentId++), mName(name), mMutex(),
      mParentsToAdd(), mChildrenToAdd(), mParentsToRemove(), mChildrenToRemove(), mParents(), mChildren() {
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
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (HasParent(parent)) {
        return mParents[parent];
    }
    return nullptr;
}

std::shared_ptr<Component> Component::GetChild(const std::string& child) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (HasChild(child)) {
        return mChildren[child];
    }
    return nullptr;
}

std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Component>>> Component::GetParents() {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return std::make_shared<std::unordered_map<std::string, std::shared_ptr<Component>>>(mParents);
}

std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Component>>> Component::GetChildren() {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return std::make_shared<std::unordered_map<std::string, std::shared_ptr<Component>>>(mChildren);
}

bool Component::HasParent(std::shared_ptr<Component> parent) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return HasParent(parent->GetName());
}

bool Component::HasParent(const std::string& parent) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mParents.contains(parent);
}

bool Component::HasParent() {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return !mParents.empty();
}

bool Component::HasChild(std::shared_ptr<Component> child) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return HasChild(child->GetName());
}
bool Component::HasChild(const std::string& child) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mChildren.contains(child);
}

bool Component::HasChild() {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return !mChildren.empty();
}

Component& Component::AddParent(std::shared_ptr<Component> parent, bool now) {
    if (parent == nullptr) {
        return *this;
    }

    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasParent(parent->GetName())) {
        return *this;
    }

    if (now) {
        AddParentRaw(parent);
    } else {
        if (mParentsToAdd.contains(parent->GetName())) {
            return *this;
        }
        mParentsToAdd[parent->GetName()] = parent;
    }

    parent->AddChild(shared_from_this(), now);

    return *this;
}

Component& Component::AddChild(std::shared_ptr<Component> child, bool now) {
    if (child == nullptr) {
        return *this;
    }

    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasChild(child->GetName())) {
        return *this;
    }

    if (now) {
        AddChildRaw(child);
    } else {
        if (mChildrenToAdd.contains(child->GetName())) {
            return *this;
        }
        mChildrenToAdd[child->GetName()] = child;
    }

    child->AddParent(shared_from_this(), now);

    return *this;
}

Component& Component::RemoveParent(std::shared_ptr<Component> parent, bool now) {
    if (parent == nullptr) {
        return *this;
    }

    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasChild(parent->GetName())) {
        return *this;
    }

    if (now) {
        RemoveParentRaw(parent);
    } else {
        if (mParentsToRemove.contains(parent->GetName())) {
            return *this;
        }
        mParentsToRemove[parent->GetName()] = parent;
    }

    parent->RemoveChild(shared_from_this(), now);

    return *this;
}
Component& Component::RemoveChild(std::shared_ptr<Component> child, bool now) {
    if (child == nullptr) {
        return *this;
    }

    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasParent(child->GetName())) {
        return *this;
    }

    if (now) {
        RemoveChildRaw(child);
    } else {
        if (mChildrenToRemove.contains(child->GetName())) {
            return *this;
        }
        mParentsToRemove[child->GetName()] = child;
    }

    child->RemoveParent(shared_from_this(), now);

    return *this;
}

Component& Component::RemoveParent(const std::string& parent, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasParent(parent)) {
        return *this;
    }
    return RemoveParent(mParents[parent]);
}

Component& Component::RemoveChild(const std::string& child, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasChild(child)) {
        return *this;
    }
    return RemoveChild(mChildren[child]);
}

Component& Component::AddParents(const std::unordered_map<std::string, std::shared_ptr<Component>>& parents, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : parents) {
        AddParent(pair.second, now);
    }

    return *this;
}

Component& Component::AddChildren(const std::unordered_map<std::string, std::shared_ptr<Component>>& children,
                                  bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : children) {
        AddChild(pair.second, now);
    }

    return *this;
}

Component& Component::AddParents(const std::vector<std::shared_ptr<Component>>& parents, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& parent : parents) {
        AddParent(parent, now);
    }

    return *this;
}

Component& Component::AddChildren(const std::vector<std::shared_ptr<Component>>& children, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& child : children) {
        AddChild(child, now);
    }

    return *this;
}

Component& Component::RemoveParents(const std::unordered_map<std::string, std::shared_ptr<Component>>& parents,
                                    bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : parents) {
        RemoveParent(pair.second, now);
    }

    return *this;
}

Component& Component::RemoveChildren(const std::unordered_map<std::string, std::shared_ptr<Component>>& children,
                                     bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : children) {
        RemoveChild(pair.second, now);
    }

    return *this;
}

Component& Component::RemoveParents(const std::vector<std::shared_ptr<Component>>& parents, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& parent : parents) {
        RemoveParent(parent, now);
    }

    return *this;
}

Component& Component::RemoveChildren(const std::vector<std::shared_ptr<Component>>& children, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& child : children) {
        RemoveChild(child, now);
    }

    return *this;
}

Component& Component::RemoveParents(const std::vector<std::string>& parents, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& parent : parents) {
        RemoveParent(parent, now);
    }

    return *this;
}

Component& Component::RemoveChildren(const std::vector<std::string>& children, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& child : children) {
        RemoveChild(child, now);
    }

    return *this;
}

Component& Component::RemoveParents(bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : mParents) {
        RemoveParent(pair.second, now);
    }

    return *this;
}

Component& Component::RemoveChildren(bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : mParents) {
        RemoveParent(pair.second, now);
    }

    return *this;
}

Component& Component::AddParentRaw(std::shared_ptr<Component> parent) {
    SPDLOG_INFO("Adding parent {} to this component {}", static_cast<std::string>(*parent), static_cast<std::string>(*this));
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    mParents[parent->GetName()] = parent;
    return *this;
}

Component& Component::AddChildRaw(std::shared_ptr<Component> child) {
    SPDLOG_INFO("Adding parent {} to this component {}", static_cast<std::string>(*child),
                static_cast<std::string>(*this));
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    mChildren[child->GetName()] = child;
    return *this;
}

Component& Component::RemoveParentRaw(std::shared_ptr<Component> parent) {
    SPDLOG_INFO("Removing parent {} from this component {}", static_cast<std::string>(*parent),
                static_cast<std::string>(*this));

    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    mParents.erase(parent->GetName());
    return *this;
}

Component& Component::RemoveChildRaw(std::shared_ptr<Component> child) {
    SPDLOG_INFO("Adding child {} from this component {}", static_cast<std::string>(*child),
                static_cast<std::string>(*this));
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    mChildren.erase(child->GetName());
    return *this;
}
} // namespace Ship
