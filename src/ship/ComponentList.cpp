#include "ship/ComponentList.h"
#include "ship/Component.h"
#include "ship/Context.h"
#include "ship/TickableComponent.h"

namespace Ship {

// Helper: recursively set the context on a component and all of its descendants.
static void PropagateContextDown(Component* comp, std::shared_ptr<Context> ctx) {
    comp->SetContext(ctx);
    auto children = comp->GetChildren().Get();
    for (const auto& child : *children) {
        PropagateContextDown(child.get(), ctx);
    }
}

ComponentList::ComponentList(Component* owner, ComponentListRole role)
    : PartList<Component>(), mOwner(owner), mRole(role) {
}

void ComponentList::Added(std::shared_ptr<Component> part, const bool forced) {
    if (!part || !mOwner) {
        return;
    }

    auto ownerShared = mOwner->GetSharedComponent();

    if (mRole == ComponentListRole::Children) {
        // Ensure the TickableComponent part has its mWeakSelf initialized before
        // the bidirectional sync fires (which calls part->GetSharedComponent()).
        if (auto tickable = std::dynamic_pointer_cast<TickableComponent>(part)) {
            tickable->InitWeakSelf(tickable);
        }
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
        if (tickable && GetCount() == 1) {
            // Use the TickableComponent's own stored context (set at construction time) rather
            // than the owner's Part context, which may not yet be propagated.
            auto context = tickable->GetContext();
            if (context && !context->GetTickableComponents().Has(tickable)) {
                context->GetTickableComponents().Add(tickable);
            }
        }
    }
}

void ComponentList::Removed(std::shared_ptr<Component> part, const bool forced) {
    if (!part || !mOwner) {
        return;
    }

    auto ownerShared = mOwner->GetSharedComponent();

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
        if (tickable && GetCount() == 0) {
            // Use the TickableComponent's own stored context (set at construction time).
            auto context = tickable->GetContext();
            if (context && context->GetTickableComponents().Has(tickable)) {
                context->GetTickableComponents().Remove(tickable);
            }
        }
    }
}

bool ComponentList::Has(const std::string& name) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(GetMutex());
#endif
    const auto& list = this->GetList();
    return std::find_if(list.begin(), list.end(),
                        [&name](const std::shared_ptr<Component>& c) { return c->GetName() == name; }) != list.end();
}

std::shared_ptr<std::vector<std::shared_ptr<Component>>> ComponentList::Get(const std::string& name) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(GetMutex());
#endif
    auto result = std::make_shared<std::vector<std::shared_ptr<Component>>>();
    for (const auto& c : this->GetList()) {
        if (c->GetName() == name) {
            result->push_back(c);
        }
    }
    return result;
}

std::shared_ptr<std::vector<std::shared_ptr<Component>>>
ComponentList::Get(const std::vector<std::string>& names) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(GetMutex());
#endif
    auto result = std::make_shared<std::vector<std::shared_ptr<Component>>>();
    for (const auto& c : this->GetList()) {
        if (std::find(names.begin(), names.end(), c->GetName()) != names.end()) {
            result->push_back(c);
        }
    }
    return result;
}

} // namespace Ship
