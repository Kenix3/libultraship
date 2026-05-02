#include "ship/Component.h"

#ifdef COMPONENT_THREAD_SAFE
#include <shared_mutex>
#include <mutex>
#endif

#include <spdlog/spdlog.h>
#include <algorithm>

namespace Ship {

// ---- Component ----

Component::Component(const std::string& name)
    : Part(), mName(name), mParents(this, ComponentListRole::Parents), mChildren(this, ComponentListRole::Children)
#ifdef COMPONENT_THREAD_SAFE
      ,
      mMutex()
#endif
{
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

#ifdef COMPONENT_THREAD_SAFE
std::shared_mutex& Component::GetMutex() const {
    return mMutex;
}
#endif

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

    // Check declared dependencies before calling OnInit.
    auto deps = GetDependencies();
    if (deps.is_array()) {
        for (const auto& dep : deps) {
            if (!dep.is_string()) {
                continue;
            }
            std::string depName = dep.get<std::string>();
            // Search parents for the dependency (siblings are children of our parent).
            bool found = false;
            auto parents = GetParents().Get();
            for (const auto& parent : *parents) {
                auto siblings = parent->GetChildren().Get();
                for (const auto& sibling : *siblings) {
                    if (sibling.get() == this) {
                        continue;
                    }
                    if (sibling->GetName() == depName) {
                        if (!sibling->IsInitialized()) {
                            throw std::runtime_error(GetName() + " requires " + depName +
                                                     " to be initialized before it");
                        }
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
            if (!found) {
                throw std::runtime_error(GetName() + " requires " + depName +
                                         " in the component hierarchy but it was not found");
            }
        }
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

nlohmann::json Component::GetDependencies() const {
    return nlohmann::json::array();
}

void Component::MarkInitialized() {
    mIsInitialized = true;
}

} // namespace Ship
