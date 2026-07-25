#include "ship/ComponentList.h"
#include "ship/Component.h"
#include "ship/Context.h"
#include "ship/TickableComponent.h"

#include <spdlog/spdlog.h>
#include <unordered_set>

namespace Ship {

// Helper: recursively set the context on a component and all of its descendants.
// A visited set guards against cycles in the child graph so a cyclic hierarchy
// cannot cause unbounded recursion.
static void PropagateContextDown(Component* comp, std::shared_ptr<Context> ctx, std::unordered_set<uint64_t>& visited) {
    if (!visited.insert(comp->GetId()).second) {
        return;
    }
    comp->SetContext(ctx);
    auto children = comp->GetChildren().Get();
    for (const auto& child : *children) {
        PropagateContextDown(child.get(), ctx, visited);
    }
}

static void PropagateContextDown(Component* comp, std::shared_ptr<Context> ctx) {
    std::unordered_set<uint64_t> visited;
    PropagateContextDown(comp, std::move(ctx), visited);
}

template <typename StoredPtr>
BasicComponentList<StoredPtr>::BasicComponentList(Component* owner, ComponentListRole role)
    : PartListBase(), mOwner(owner), mRole(role) {
}

template <typename StoredPtr>
void BasicComponentList<StoredPtr>::Added(std::shared_ptr<Component> part, const bool forced) {
    if (!part || !mOwner) {
        return;
    }

    std::shared_ptr<Component> ownerShared = nullptr;
    if (mRole == ComponentListRole::Parents) {
        ownerShared = part->GetChildren().Get(mOwner->GetId());
        if (!ownerShared) {
            ownerShared = mOwner->TryGetSharedComponent();
        }
        if (!ownerShared) {
            auto tickableOwner = dynamic_cast<TickableComponent*>(mOwner);
            if (tickableOwner) {
                auto context = tickableOwner->GetContext();
                if (context) {
                    ownerShared = std::static_pointer_cast<Component>(context->GetTickableComponents().Get(mOwner->GetId()));
                }
            }
        }
    } else {
        ownerShared = mOwner->TryGetSharedComponent();
    }

    if (!ownerShared) {
        SPDLOG_TRACE("Skipping Added() reciprocal sync for owner id {} during teardown", mOwner->GetId());
        return;
    }

    if (mRole == ComponentListRole::Children) {
        // Add the owner as a parent of the child (if not already present)
        if (!part->GetParents().Has(ownerShared)) {
            part->GetParents().Add(ownerShared, forced);
        }
        // Propagate the owner's Context (if any) down to the new child and all
        // of its existing descendants.
        if (auto ctx = mOwner->GetContext()) {
            PropagateContextDown(part.get(), ctx);
        }
    } else if (mRole == ComponentListRole::Parents) {
        // Add the owner as a child of the parent (if not already present)
        if (!part->GetChildren().Has(ownerShared)) {
            part->GetChildren().Add(ownerShared, forced);
        }

        // Register TickableComponent with the Context's global TickableList when it gets its first parent
        auto tickable = std::dynamic_pointer_cast<TickableComponent>(ownerShared);
        if (tickable && this->GetCount() == 1) {
            // Use the TickableComponent's own stored context (set at construction time) rather
            // than the owner's Part context, which may not yet be propagated.
            auto context = tickable->GetContext();
            if (context && !context->GetTickableComponents().Has(tickable)) {
                context->GetTickableComponents().Add(tickable);
            }
        }
    }
}

template <typename StoredPtr>
void BasicComponentList<StoredPtr>::Removed(std::shared_ptr<Component> part, const bool forced) {
    if (!part || !mOwner) {
        return;
    }

    std::shared_ptr<Component> ownerShared = nullptr;
    if (mRole == ComponentListRole::Parents) {
        ownerShared = part->GetChildren().Get(mOwner->GetId());
        if (!ownerShared) {
            ownerShared = mOwner->TryGetSharedComponent();
        }
        if (!ownerShared) {
            auto tickableOwner = dynamic_cast<TickableComponent*>(mOwner);
            if (tickableOwner) {
                auto context = tickableOwner->GetContext();
                if (context) {
                    ownerShared = std::static_pointer_cast<Component>(context->GetTickableComponents().Get(mOwner->GetId()));
                }
            }
        }
    } else {
        ownerShared = mOwner->TryGetSharedComponent();
    }

    if (!ownerShared) {
        SPDLOG_TRACE("Skipping Removed() reciprocal sync for owner id {} during teardown", mOwner->GetId());
        return;
    }

    if (mRole == ComponentListRole::Children) {
        // Remove the owner from the child's parent list
        if (part->GetParents().Has(ownerShared)) {
            part->GetParents().Remove(ownerShared, forced);
        }
    } else if (mRole == ComponentListRole::Parents) {
        // Remove the owner from the parent's child list
        if (part->GetChildren().Has(ownerShared)) {
            part->GetChildren().Remove(ownerShared, forced);
        }

        // Unregister TickableComponent from the Context's global TickableList when it loses its last parent
        auto tickable = std::dynamic_pointer_cast<TickableComponent>(ownerShared);
        if (tickable && this->GetCount() == 0) {
            // Use the TickableComponent's own stored context (set at construction time).
            auto context = tickable->GetContext();
            if (context && context->GetTickableComponents().Has(tickable)) {
                context->GetTickableComponents().Remove(tickable);
            }
        }
    }
}

template <typename StoredPtr> bool BasicComponentList<StoredPtr>::Has(const std::string& name) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(this->GetMutex());
#endif
    auto list = this->Get();
    return std::find_if(list->begin(), list->end(),
                        [&name](const std::shared_ptr<Component>& c) { return c->GetName() == name; }) != list->end();
}

template <typename StoredPtr>
std::shared_ptr<std::vector<std::shared_ptr<Component>>>
BasicComponentList<StoredPtr>::Get(const std::string& name) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(this->GetMutex());
#endif
    auto result = std::make_shared<std::vector<std::shared_ptr<Component>>>();
    auto list = this->Get();
    for (const auto& c : *list) {
        if (c->GetName() == name) {
            result->push_back(c);
        }
    }
    return result;
}

template <typename StoredPtr>
std::shared_ptr<std::vector<std::shared_ptr<Component>>>
BasicComponentList<StoredPtr>::Get(const std::vector<std::string>& names) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(this->GetMutex());
#endif
    auto result = std::make_shared<std::vector<std::shared_ptr<Component>>>();
    auto list = this->Get();
    for (const auto& c : *list) {
        if (std::find(names.begin(), names.end(), c->GetName()) != names.end()) {
            result->push_back(c);
        }
    }
    return result;
}

template class BasicComponentList<std::shared_ptr<Component>>;
template class BasicComponentList<std::weak_ptr<Component>>;

} // namespace Ship
