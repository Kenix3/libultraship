#include "ship/Component.h"

#ifdef COMPONENT_THREAD_SAFE
#include <shared_mutex>
#include <mutex>
#endif

#include <spdlog/spdlog.h>
#include <algorithm>

namespace Ship {

Component::Component(const std::string& name)
    : Part(), mName(name), mParents(), mChildren()
#ifdef COMPONENT_THREAD_SAFE
      , mMutex()
#endif
{
    SPDLOG_INFO("Constructing component {}", ToString());
}

Component::~Component() {
    SPDLOG_INFO("Destructing component {}", ToString());
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

#ifdef COMPONENT_THREAD_SAFE
std::shared_mutex& Component::GetMutex() {
    return mMutex;
}
#endif

// ---- Get ----

PartList<Component>& Component::GetParents() {
    return mParents;
}

const PartList<Component>& Component::GetParents() const {
    return mParents;
}

PartList<Component>& Component::GetChildren() {
    return mChildren;
}

const PartList<Component>& Component::GetChildren() const {
    return mChildren;
}

// ---- Add ----

bool Component::AddParent(std::shared_ptr<Component> parent, const bool force) {
    if (!parent) {
        return false;
    }
    const auto self = shared_from_this();
    const bool canAddParent = CanAddParent(parent);
    const bool canAddChild = parent->CanAddChild(self);
    const bool forced = (!canAddParent || !canAddChild) && force;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::unique_lock<std::shared_mutex> lock(mMutex);
#endif
        if (mParents.Has(parent)) {
            return true;
        }
        if ((!canAddParent || !canAddChild) && !force) {
            return false;
        }
        mParents.Add(parent);
    }
    AddedParent(parent, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be added as a parent to {}", parent->ToString(), ToString());
    }
    parent->AddChild(self, force);
    return true;
}

bool Component::AddParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force) {
    bool ok = true;
    for (const auto& p : parents) {
        ok &= AddParent(p, force);
    }
    return ok;
}

bool Component::AddChild(std::shared_ptr<Component> child, const bool force) {
    if (!child) {
        return false;
    }
    const auto self = shared_from_this();
    const bool canAddChild = CanAddChild(child);
    const bool canAddParent = child->CanAddParent(self);
    const bool forced = (!canAddChild || !canAddParent) && force;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::unique_lock<std::shared_mutex> lock(mMutex);
#endif
        if (mChildren.Has(child)) {
            return true;
        }
        if ((!canAddChild || !canAddParent) && !force) {
            return false;
        }
        mChildren.Add(child);
    }
    AddedChild(child, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be added as a child to {}", child->ToString(), ToString());
    }
    child->AddParent(self, force);
    return true;
}

bool Component::AddChildren(const std::vector<std::shared_ptr<Component>>& children, const bool force) {
    bool ok = true;
    for (const auto& c : children) {
        ok &= AddChild(c, force);
    }
    return ok;
}

// ---- Remove ----

bool Component::RemoveParent(std::shared_ptr<Component> parent, const bool force) {
    if (!parent) {
        return false;
    }
    const auto self = shared_from_this();
    const bool canRemoveParent = CanRemoveParent(parent);
    const bool canRemoveChild = parent->CanRemoveChild(self);
    const bool forced = (!canRemoveParent || !canRemoveChild) && force;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::unique_lock<std::shared_mutex> lock(mMutex);
#endif
        if (!mParents.Has(parent)) {
            return true;
        }
        if ((!canRemoveParent || !canRemoveChild) && !force) {
            return false;
        }
        mParents.Remove(parent);
    }
    RemovedParent(parent, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be removed as a parent from {}", parent->ToString(), ToString());
    }
    parent->RemoveChild(self, force);
    return true;
}

bool Component::RemoveParent(const uint64_t parentId, const bool force) {
    std::shared_ptr<Component> found;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        found = mParents.Get(parentId);
    }
    if (!found) {
        return false;
    }
    return RemoveParent(found, force);
}

bool Component::RemoveParents(const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mParents.Get();
    }
    bool ok = true;
    for (const auto& p : *snapshot) {
        ok &= RemoveParent(p, force);
    }
    return ok;
}

bool Component::RemoveParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force) {
    bool ok = true;
    for (const auto& p : parents) {
        ok &= RemoveParent(p, force);
    }
    return ok;
}

bool Component::RemoveParents(const std::vector<uint64_t>& parentIds, const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mParents.Get();
    }
    bool ok = true;
    for (const auto& p : *snapshot) {
        if (std::find(parentIds.begin(), parentIds.end(), p->GetId()) != parentIds.end()) {
            ok &= RemoveParent(p, force);
        }
    }
    return ok;
}

bool Component::RemoveParents(const std::string& name, const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mParents.Get();
    }
    bool ok = true;
    for (const auto& p : *snapshot) {
        if (p->GetName() == name) {
            ok &= RemoveParent(p, force);
        }
    }
    return ok;
}

bool Component::RemoveParents(const std::vector<std::string>& names, const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mParents.Get();
    }
    bool ok = true;
    for (const auto& p : *snapshot) {
        if (std::find(names.begin(), names.end(), p->GetName()) != names.end()) {
            ok &= RemoveParent(p, force);
        }
    }
    return ok;
}

bool Component::RemoveChild(std::shared_ptr<Component> child, const bool force) {
    if (!child) {
        return false;
    }
    const auto self = shared_from_this();
    const bool canRemoveChild = CanRemoveChild(child);
    const bool canRemoveParent = child->CanRemoveParent(self);
    const bool forced = (!canRemoveChild || !canRemoveParent) && force;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::unique_lock<std::shared_mutex> lock(mMutex);
#endif
        if (!mChildren.Has(child)) {
            return true;
        }
        if ((!canRemoveChild || !canRemoveParent) && !force) {
            return false;
        }
        mChildren.Remove(child);
    }
    RemovedChild(child, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be removed as a child from {}", child->ToString(), ToString());
    }
    child->RemoveParent(self, force);
    return true;
}

bool Component::RemoveChild(const uint64_t childId, const bool force) {
    std::shared_ptr<Component> found;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        found = mChildren.Get(childId);
    }
    if (!found) {
        return false;
    }
    return RemoveChild(found, force);
}

bool Component::RemoveChildren(const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mChildren.Get();
    }
    bool ok = true;
    for (const auto& c : *snapshot) {
        ok &= RemoveChild(c, force);
    }
    return ok;
}

bool Component::RemoveChildren(const std::vector<std::shared_ptr<Component>>& children,
                               const bool force) {
    bool ok = true;
    for (const auto& c : children) {
        ok &= RemoveChild(c, force);
    }
    return ok;
}

bool Component::RemoveChildren(const std::vector<uint64_t>& childIds, const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mChildren.Get();
    }
    bool ok = true;
    for (const auto& c : *snapshot) {
        if (std::find(childIds.begin(), childIds.end(), c->GetId()) != childIds.end()) {
            ok &= RemoveChild(c, force);
        }
    }
    return ok;
}

bool Component::RemoveChildren(const std::string& name, const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mChildren.Get();
    }
    bool ok = true;
    for (const auto& c : *snapshot) {
        if (c->GetName() == name) {
            ok &= RemoveChild(c, force);
        }
    }
    return ok;
}

bool Component::RemoveChildren(const std::vector<std::string>& names, const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mChildren.Get();
    }
    bool ok = true;
    for (const auto& c : *snapshot) {
        if (std::find(names.begin(), names.end(), c->GetName()) != names.end()) {
            ok &= RemoveChild(c, force);
        }
    }
    return ok;
}

// ---- Virtual hooks (default implementations) ----

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

void Component::AddedParent(std::shared_ptr<Component> parent, const bool forced) {}

void Component::AddedChild(std::shared_ptr<Component> child, const bool forced) {}

void Component::RemovedParent(std::shared_ptr<Component> parent, const bool forced) {}

void Component::RemovedChild(std::shared_ptr<Component> child, const bool forced) {}

} // namespace Ship

