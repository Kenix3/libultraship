#include "ship/core/Component.h"
#include "ship/core/Tickable.h"
#include "ship/core/TickableComponent.h"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace Ship {

// ---- Component ----

Component::Component(const std::string& name, std::shared_ptr<Context> context)
    : Part(std::move(context)), mName(name), mParents(this, ComponentListRole::Parents),
      mChildren(this, ComponentListRole::Children) {
    if (spdlog::default_logger()) {
        SPDLOG_INFO("Constructing component {}", ToString());
    }
}

Component::~Component() {
    if (spdlog::default_logger()) {
        SPDLOG_INFO("Destructing component {}", ToString());
    }
}

const std::string& Component::GetName() const {
    return mName;
}

std::string Component::ToString() const {
    return std::to_string(GetId()) + "-" + GetName() + "-" + typeid(*this).name();
}

std::string Component::ToTreeString(int depth) const {
    std::unordered_set<uint64_t> visited;
    return ToTreeStringImpl(depth, visited);
}

std::string Component::ToTreeStringImpl(int depth, std::unordered_set<uint64_t>& visited) const {
    std::string indent(depth * 2, ' ');
    std::string result = indent + GetName() + "\n";
    // Guard against cycles in the child graph to avoid unbounded recursion.
    if (!visited.insert(GetId()).second) {
        return result;
    }
    auto children = mChildren.Get();
    for (const auto& child : *children) {
        result += child->ToTreeStringImpl(depth + 1, visited);
    }
    return result;
}

Component::operator std::string() const {
    return ToString();
}

// ---- Get ----

ParentComponentList& Component::GetParents() {
    return mParents;
}

const ParentComponentList& Component::GetParents() const {
    return mParents;
}

ComponentList& Component::GetChildren() {
    return mChildren;
}

const ComponentList& Component::GetChildren() const {
    return mChildren;
}

std::shared_ptr<Component> Component::TryGetSharedComponent() noexcept {
    if (auto self = mWeakSelf.lock()) {
        return self;
    }

    try {
        auto self = shared_from_this();
        mWeakSelf = self;
        return self;
    } catch (const std::bad_weak_ptr&) {
    }

    if (auto tickableComponent = dynamic_cast<TickableComponent*>(this)) {
        try {
            auto derivedSelf = tickableComponent->std::enable_shared_from_this<TickableComponent>::shared_from_this();
            auto componentSelf = std::static_pointer_cast<Component>(derivedSelf);
            mWeakSelf = componentSelf;
            return componentSelf;
        } catch (const std::bad_weak_ptr&) {
        }
    }

    if (auto tickable = dynamic_cast<Tickable*>(this)) {
        try {
            auto tickableSelf = tickable->shared_from_this();
            auto componentSelf = std::dynamic_pointer_cast<Component>(tickableSelf);
            if (componentSelf) {
                mWeakSelf = componentSelf;
                return componentSelf;
            }
        } catch (const std::bad_weak_ptr&) {
        }
    }

    auto parents = GetParents().Get();
    for (const auto& parent : *parents) {
        if (!parent) {
            continue;
        }
        auto selfFromParent = parent->GetChildren().Get(GetId());
        if (selfFromParent) {
            mWeakSelf = selfFromParent;
            return selfFromParent;
        }
    }

    return nullptr;
}

std::shared_ptr<Component> Component::GetSharedComponent() {
    auto self = TryGetSharedComponent();
    if (!self) {
        throw std::bad_weak_ptr();
    }
    return self;
}

void Component::Init(const nlohmann::json& initArgs) {
    if (mIsInitialized) {
        return;
    }

    TryGetSharedComponent();
    OnInit(initArgs);
    mIsInitialized = true;
}

bool Component::IsInitialized() const {
    return mIsInitialized;
}

void Component::OnInit(const nlohmann::json& /*initArgs*/) {
    // Default: no-op. Subclasses override to perform initialization.
}

void Component::MarkInitialized() {
    mIsInitialized = true;
}

} // namespace Ship
