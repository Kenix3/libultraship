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
    // Dependencies are resolved by searching the full hierarchy: siblings,
    // ancestors, and their subtrees (BFS from each parent upward).
    const auto& deps = GetDependencies();
    if (deps.is_array()) {
        for (const auto& dep : deps) {
            if (!dep.is_string()) {
                continue;
            }
            std::string depName = dep.get<std::string>();
            bool found = false;

            // BFS upward through parent chain, checking siblings and ancestors.
            std::queue<Component*> searchQueue;
            std::unordered_set<uint64_t> visited;
            visited.insert(GetId());

            auto parents = GetParents().Get();
            for (const auto& parent : *parents) {
                if (visited.insert(parent->GetId()).second) {
                    searchQueue.push(parent.get());
                }
            }

            while (!searchQueue.empty() && !found) {
                Component* current = searchQueue.front();
                searchQueue.pop();

                // Check the current node itself.
                if (current->GetName() == depName) {
                    if (!current->IsInitialized()) {
                        throw std::runtime_error("Component '" + GetName() + "' requires dependency '" + depName +
                                                 "' to be initialized before calling Init()");
                    }
                    found = true;
                    break;
                }

                // Check this node's children (our siblings, cousins, etc.).
                auto children = current->GetChildren().Get();
                for (const auto& child : *children) {
                    if (visited.count(child->GetId())) {
                        continue;
                    }
                    visited.insert(child->GetId());
                    if (child->GetName() == depName) {
                        if (!child->IsInitialized()) {
                            throw std::runtime_error("Component '" + GetName() + "' requires dependency '" + depName +
                                                     "' to be initialized before calling Init()");
                        }
                        found = true;
                        break;
                    }
                }

                // Continue searching upward through parents.
                if (!found) {
                    auto grandParents = current->GetParents().Get();
                    for (const auto& gp : *grandParents) {
                        if (visited.insert(gp->GetId()).second) {
                            searchQueue.push(gp.get());
                        }
                    }
                }
            }

            if (!found) {
                throw std::runtime_error("Component '" + GetName() + "' requires dependency '" + depName +
                                         "' to be present in the component hierarchy. "
                                         "Ensure it is added before initializing '" +
                                         GetName() + "'");
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

const nlohmann::json& Component::GetDependencies() const {
    static const nlohmann::json empty = nlohmann::json::array();
    return empty;
}

void Component::MarkInitialized() {
    mIsInitialized = true;
}

} // namespace Ship
