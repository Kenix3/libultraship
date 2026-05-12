#include "ship/Component.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace Ship {

// ---- Component ----

Component::Component(const std::string& name)
    : Part(), mName(name), mParents(this, ComponentListRole::Parents), mChildren(this, ComponentListRole::Children) {
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

Component::operator std::string() const {
    return ToString();
}

// ---- Get ----

ComponentList& Component::GetParents() {
    return mParents;
}

const ComponentList& Component::GetParents() const {
    return mParents;
}

ComponentList& Component::GetChildren() {
    return mChildren;
}

const ComponentList& Component::GetChildren() const {
    return mChildren;
}

std::shared_ptr<Component> Component::GetSharedComponent() {
    return shared_from_this();
}

void Component::Init(const nlohmann::json& initArgs) {
    if (mIsInitialized) {
        return;
    }

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
