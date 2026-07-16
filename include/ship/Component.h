#pragma once

#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <type_traits>

#include "ship/Part.h"
#include "ship/PartList.h"
#include "ship/ComponentList.h"

namespace Ship {

/**
 * @brief A named Part with a parent/child hierarchy and optional thread safety.
 *
 * Component extends Part with a human-readable name, a string representation,
 * and bidirectional parent/child relationships managed via ComponentList. Adding
 * a child automatically adds the corresponding parent, and vice versa. When the
 * COMPONENT_THREAD_SAFE preprocessor flag is defined, all relationship
 * mutations are guarded by a recursive_mutex held by the ComponentList (PartList).
 */
class Component : public Part, public std::enable_shared_from_this<Component> {
  public:
    /**
     * @brief Constructs a Component with the given name.
     * @param name A human-readable name for this Component.
     */
    explicit Component(const std::string& name, std::shared_ptr<Context> context = nullptr);
    virtual ~Component();

    /**
     * @brief Performs one-time initialization of this component.
     *
     * This method is non-virtual and manages the initialized state flag. It
     * calls the protected virtual OnInit() hook exactly once; subsequent calls
     * are no-ops.
     *
     * Concrete subclasses that need parameter-free initialization should
     * override OnInit() rather than Init().
     *
     * Subclasses that require initialization parameters (e.g. ResourceManager)
     * provide their own Init(…) overload and must call MarkInitialized() when
     * initialization succeeds.
     *
     * @param initArgs Optional JSON object containing initialization arguments
     *                 for this component. Passed through to OnInit().
     */
    void Init(const nlohmann::json& initArgs = nlohmann::json::object());

    /**
     * @brief Returns true once Init() (or MarkInitialized()) has completed
     *        successfully.
     *
     * Components that self-initialize in their constructor (e.g. Config,
     * ThreadPool) call MarkInitialized() there and return true
     * immediately. Components initialized via Init() / OnInit() return true
     * only after that call succeeds.
     */
    bool IsInitialized() const;

    /** @brief Returns the name of this Component. */
    const std::string& GetName() const;

    /** @brief Returns a human-readable string representation (e.g. "Name (id)"). */
    std::string ToString() const;

    /**
     * @brief Returns a human-readable tree representation of this component and its children.
     * @param depth Indentation depth (0 = root).
     */
    std::string ToTreeString(int depth = 0) const;

    /** @brief Conversion operator to std::string; equivalent to ToString(). */
    explicit operator std::string() const;

    // ---- Parent/child relationship accessors ----

    /** @brief Returns a mutable reference to the parent list. */
    ComponentList& GetParents();
    /** @brief Returns a const reference to the parent list. */
    const ComponentList& GetParents() const;

    /** @brief Returns a mutable reference to the child list. */
    ComponentList& GetChildren();
    /** @brief Returns a const reference to the child list. */
    const ComponentList& GetChildren() const;

    /**
     * @brief Returns a shared_ptr to this Component via the correct enable_shared_from_this base.
     *
     * Subclasses that inherit enable_shared_from_this via multiple paths (e.g. TickableComponent
     * inherits from both Component and Tickable, each with their own enable_shared_from_this) must
     * override this to use the base whose weak_ptr is properly initialized by make_shared.
     */
    virtual std::shared_ptr<Component> GetSharedComponent();

    // ---- Breadth-first hierarchy search ----

    /**
     * @brief Checks whether any descendant Component matches type T via BFS.
     * @tparam T The derived type to search for via dynamic_cast.
     * @return True if at least one matching descendant is found.
     */
    template <typename T> bool HasInChildren() const;

    /**
     * @brief Returns the first descendant that matches type T via BFS.
     * @tparam T The derived type to search for via dynamic_cast.
     * @return The first matching descendant, or nullptr if none found.
     */
    template <typename T> std::shared_ptr<T> GetFirstInChildren() const;

    /**
     * @brief Returns all descendants that match type T via BFS.
     * @tparam T The derived type to filter by via dynamic_cast.
     * @return A vector of all matching descendants.
     */
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> GetInChildren() const;

    // ---- Breadth-first ancestor search ----

    /**
     * @brief Checks whether any ancestor Component matches type T via BFS.
     * @tparam T The derived type to search for via dynamic_cast.
     * @return True if at least one matching ancestor is found.
     */
    template <typename T> bool HasInParents() const;

    /**
     * @brief Returns the first ancestor that matches type T via BFS.
     * @tparam T The derived type to search for via dynamic_cast.
     * @return The first matching ancestor, or nullptr if none found.
     */
    template <typename T> std::shared_ptr<T> GetFirstInParents() const;

    /**
     * @brief Returns all ancestors that match type T via BFS.
     * @tparam T The derived type to filter by via dynamic_cast.
     * @return A vector of all matching ancestors.
     */
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> GetInParents() const;

  protected:
    /**
     * @brief Override this to implement component-specific initialization logic.
     *
     * Called by Init() the first time it is invoked.
     * After OnInit() returns without throwing, the component is marked as initialized.
     * If OnInit() throws, the component remains uninitialized so Init() may be retried.
     *
     * @param initArgs JSON object with component-specific initialization arguments
     *                 from the build hierarchy. Default implementation ignores them.
     *
     * Components with initialization parameters (ResourceManager, Window, …)
     * provide their own Init(…) overload instead of overriding this method.
     *
     * The default implementation is a no-op.
     */
    virtual void OnInit(const nlohmann::json& initArgs = nlohmann::json::object());

    /**
     * @brief Marks this component as initialized without going through Init().
     *
     * Call this in the constructor of components that self-initialize (i.e. are
     * fully usable as soon as they are constructed and need no separate Init()
     * call), or at the end of a custom parameterized Init(…) override, so that
     * IsInitialized() returns true and ordering-dependency checks pass.
     */
    void MarkInitialized();

    /**
     * @brief Returns a cached dependency after validating it exists and is initialized.
     *
     * Use this from subclasses when accessing constructor- or init-cached component
     * dependencies. The dependency must both exist and already be initialized.
     *
     * @throws std::runtime_error if the dependency pointer is empty.
     * @throws std::runtime_error if the dependency exists but is not initialized.
     */
    template <typename T>
    std::shared_ptr<T> RequireDependency(const std::shared_ptr<T>& dependency, const std::string& dependencyName) const;

  private:
    std::string mName;
    bool mIsInitialized = false;
    ComponentList mParents;
    ComponentList mChildren;
};

// ---- Template BFS implementations (children) ----
// All three methods perform a full breadth-first traversal through the entire
// descendant tree. Every node's children are enqueued for further exploration,
// so the search is not limited to any particular depth.

template <typename T> bool Component::HasInChildren() const {
    std::queue<const Component*> queue;
    std::unordered_set<uint64_t> visited;
    visited.insert(GetId());
    queue.push(this);

    // BFS: process every node, enqueue its children so we reach all depths.
    while (!queue.empty()) {
        const Component* current = queue.front();
        queue.pop();

        auto currentChildren = current->GetChildren().Get();
        for (const auto& child : *currentChildren) {
            if (visited.insert(child->GetId()).second) {
                if (std::dynamic_pointer_cast<T>(child)) {
                    return true;
                }
                queue.push(child.get());
            }
        }
    }
    return false;
}

template <typename T>
std::shared_ptr<T> Component::RequireDependency(const std::shared_ptr<T>& dependency,
                                                const std::string& dependencyName) const {
    static_assert(std::is_base_of_v<Component, T>, "RequireDependency only supports Component dependencies");

    if (!dependency) {
        throw std::runtime_error("Component '" + GetName() + "' requires dependency '" + dependencyName +
                                 "' to exist before use");
    }

    if (!dependency->IsInitialized()) {
        throw std::runtime_error("Component '" + GetName() + "' requires dependency '" + dependencyName +
                                 "' to be initialized before use");
    }

    return dependency;
}

template <typename T> std::shared_ptr<T> Component::GetFirstInChildren() const {
    std::queue<const Component*> queue;
    std::unordered_set<uint64_t> visited;
    visited.insert(GetId());
    queue.push(this);

    while (!queue.empty()) {
        const Component* current = queue.front();
        queue.pop();

        auto currentChildren = current->GetChildren().Get();
        for (const auto& child : *currentChildren) {
            if (visited.insert(child->GetId()).second) {
                auto typed = std::dynamic_pointer_cast<T>(child);
                if (typed) {
                    return typed;
                }
                queue.push(child.get());
            }
        }
    }
    return nullptr;
}

template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> Component::GetInChildren() const {
    auto result = std::make_shared<std::vector<std::shared_ptr<T>>>();
    std::queue<const Component*> queue;
    std::unordered_set<uint64_t> visited;
    visited.insert(GetId());
    queue.push(this);

    while (!queue.empty()) {
        const Component* current = queue.front();
        queue.pop();

        auto currentChildren = current->GetChildren().Get();
        for (const auto& child : *currentChildren) {
            if (visited.insert(child->GetId()).second) {
                auto typed = std::dynamic_pointer_cast<T>(child);
                if (typed) {
                    result->push_back(typed);
                }
                queue.push(child.get());
            }
        }
    }
    return result;
}

// ---- Template BFS implementations (parents) ----
// Mirror of the InChildren methods, but traversing upward through parents.

template <typename T> bool Component::HasInParents() const {
    std::queue<const Component*> queue;
    std::unordered_set<uint64_t> visited;
    visited.insert(GetId());
    queue.push(this);

    while (!queue.empty()) {
        const Component* current = queue.front();
        queue.pop();

        auto currentParents = current->GetParents().Get();
        for (const auto& parent : *currentParents) {
            if (visited.insert(parent->GetId()).second) {
                if (std::dynamic_pointer_cast<T>(parent)) {
                    return true;
                }
                queue.push(parent.get());
            }
        }
    }
    return false;
}

template <typename T> std::shared_ptr<T> Component::GetFirstInParents() const {
    std::queue<const Component*> queue;
    std::unordered_set<uint64_t> visited;
    visited.insert(GetId());
    queue.push(this);

    while (!queue.empty()) {
        const Component* current = queue.front();
        queue.pop();

        auto currentParents = current->GetParents().Get();
        for (const auto& parent : *currentParents) {
            if (visited.insert(parent->GetId()).second) {
                auto typed = std::dynamic_pointer_cast<T>(parent);
                if (typed) {
                    return typed;
                }
                queue.push(parent.get());
            }
        }
    }
    return nullptr;
}

template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> Component::GetInParents() const {
    auto result = std::make_shared<std::vector<std::shared_ptr<T>>>();
    std::queue<const Component*> queue;
    std::unordered_set<uint64_t> visited;
    visited.insert(GetId());
    queue.push(this);

    while (!queue.empty()) {
        const Component* current = queue.front();
        queue.pop();

        auto currentParents = current->GetParents().Get();
        for (const auto& parent : *currentParents) {
            if (visited.insert(parent->GetId()).second) {
                auto typed = std::dynamic_pointer_cast<T>(parent);
                if (typed) {
                    result->push_back(typed);
                }
                queue.push(parent.get());
            }
        }
    }
    return result;
}

// ---- ComponentList template implementations ----
// These live here because they need the full Component definition,
// while ComponentList.h only forward-declares Component to break the
// circular dependency.

template <typename T> bool ComponentList::Has(const std::string& name) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(GetMutex());
#endif
    const auto& list = this->GetList();
    return std::find_if(list.begin(), list.end(), [&name](const std::shared_ptr<Component>& c) {
               return c->GetName() == name && std::dynamic_pointer_cast<T>(c) != nullptr;
           }) != list.end();
}

template <typename T>
std::shared_ptr<std::vector<std::shared_ptr<T>>> ComponentList::Get(const std::string& name) const {
#ifdef COMPONENT_THREAD_SAFE
    const std::lock_guard<std::recursive_mutex> lock(GetMutex());
#endif
    auto result = std::make_shared<std::vector<std::shared_ptr<T>>>();
    for (const auto& c : this->GetList()) {
        auto typed = std::dynamic_pointer_cast<T>(c);
        if (typed && c->GetName() == name) {
            result->push_back(typed);
        }
    }
    return result;
}

} // namespace Ship
