#include "ship/Component.h"

#ifdef COMPONENT_THREAD_SAFE
#include <shared_mutex>
#include <mutex>
#endif

#include <spdlog/spdlog.h>
#include <algorithm>

namespace Ship {

// ---- ChildList ----

ChildList::ChildList(Component* owner) : PartList<Component>(), mOwner(owner) {
}

void ChildList::Added(std::shared_ptr<Component> part, const bool forced) {
    if (!part || !mOwner) {
        return;
    }
    // Add the owner as a parent of the child (if not already present)
    auto ownerShared = mOwner->shared_from_this();
    if (!part->GetParents().Has(ownerShared)) {
        part->GetParents().Add(ownerShared, forced);
    }
}

void ChildList::Removed(std::shared_ptr<Component> part, const bool forced) {
    if (!part || !mOwner) {
        return;
    }
    // Remove the owner from the child's parent list
    auto ownerShared = mOwner->shared_from_this();
    if (part->GetParents().Has(ownerShared)) {
        part->GetParents().Remove(ownerShared, forced);
    }
}

// ---- ParentList ----

ParentList::ParentList(Component* owner) : PartList<Component>(), mOwner(owner) {
}

void ParentList::Added(std::shared_ptr<Component> part, const bool forced) {
    if (!part || !mOwner) {
        return;
    }
    // Add the owner as a child of the parent (if not already present)
    auto ownerShared = mOwner->shared_from_this();
    if (!part->GetChildren().Has(ownerShared)) {
        part->GetChildren().Add(ownerShared, forced);
    }
}

void ParentList::Removed(std::shared_ptr<Component> part, const bool forced) {
    if (!part || !mOwner) {
        return;
    }
    // Remove the owner from the parent's child list
    auto ownerShared = mOwner->shared_from_this();
    if (part->GetChildren().Has(ownerShared)) {
        part->GetChildren().Remove(ownerShared, forced);
    }
}

// ---- Component ----

Component::Component(const std::string& name)
    : Part(), mName(name), mParents(this), mChildren(this)
#ifdef COMPONENT_THREAD_SAFE
      ,
      mMutex()
#endif
{
    SPDLOG_INFO("Constructing component {}", ToString());
}

Component::~Component() {
    SPDLOG_INFO("Destructing component {}", ToString());
}

const std::string& Component::GetName() const {
    return mName;
}

std::string Component::ToString() const {
    return std::to_string(GetId()) + "-" + GetName() + "-" + typeid(*this).name();
}

Component::operator std::string() const {
    return ToString();
}

#ifdef COMPONENT_THREAD_SAFE
std::shared_mutex& Component::GetMutex() const {
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

} // namespace Ship
