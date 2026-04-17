#include "ship/ComponentList.h"
#include "ship/Component.h"
#include "ship/Context.h"
#include "ship/TickableComponent.h"

namespace Ship {

ComponentList::ComponentList(Component* owner, ComponentListRole role)
    : PartList<Component>(), mOwner(owner), mRole(role) {
}

void ComponentList::Added(std::shared_ptr<Component> part, const bool forced) {
    if (!part || !mOwner) {
        return;
    }

    auto ownerShared = mOwner->shared_from_this();

    if (mRole == ComponentListRole::Children) {
        // Add the owner as a parent of the child (if not already present)
        if (!part->GetParents().Has(ownerShared)) {
            part->GetParents().Add(ownerShared, forced);
        }

        // Auto-register TickableComponents with the Context's global TickableList
        auto tickable = std::dynamic_pointer_cast<TickableComponent>(part);
        if (tickable) {
            auto context = Context::GetInstance();
            if (context && !context->GetTickableComponents().Has(tickable)) {
                context->GetTickableComponents().Add(tickable);
            }
        }
    } else if (mRole == ComponentListRole::Parents) {
        // Add the owner as a child of the parent (if not already present)
        if (!part->GetChildren().Has(ownerShared)) {
            part->GetChildren().Add(ownerShared, forced);
        }
    }
}

void ComponentList::Removed(std::shared_ptr<Component> part, const bool forced) {
    if (!part || !mOwner) {
        return;
    }

    auto ownerShared = mOwner->shared_from_this();

    if (mRole == ComponentListRole::Children) {
        // Remove the owner from the child's parent list
        if (part->GetParents().Has(ownerShared)) {
            part->GetParents().Remove(ownerShared, forced);
        }

        // Auto-unregister TickableComponents from the Context's global TickableList
        auto tickable = std::dynamic_pointer_cast<TickableComponent>(part);
        if (tickable) {
            auto context = Context::GetInstance();
            if (context && context->GetTickableComponents().Has(tickable)) {
                context->GetTickableComponents().Remove(tickable);
            }
        }
    } else if (mRole == ComponentListRole::Parents) {
        // Remove the owner from the parent's child list
        if (part->GetChildren().Has(ownerShared)) {
            part->GetChildren().Remove(ownerShared, forced);
        }
    }
}

bool ComponentList::Has(const std::string& name) const {
    const std::lock_guard<std::recursive_mutex> lock(GetMutex());
    const auto& list = this->GetList();
    return std::find_if(list.begin(), list.end(),
                        [&name](const std::shared_ptr<Component>& c) { return c->GetName() == name; }) != list.end();
}

std::shared_ptr<std::vector<std::shared_ptr<Component>>> ComponentList::Get(const std::string& name) const {
    const std::lock_guard<std::recursive_mutex> lock(GetMutex());
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
    const std::lock_guard<std::recursive_mutex> lock(GetMutex());
    auto result = std::make_shared<std::vector<std::shared_ptr<Component>>>();
    for (const auto& c : this->GetList()) {
        if (std::find(names.begin(), names.end(), c->GetName()) != names.end()) {
            result->push_back(c);
        }
    }
    return result;
}

} // namespace Ship
