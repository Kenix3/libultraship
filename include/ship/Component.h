#pragma once

#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <queue>
#include <unordered_set>

#ifdef COMPONENT_THREAD_SAFE
#include <shared_mutex>
#endif

#include "ship/Part.h"
#include "ship/PartList.h"

namespace Ship {

class Component;

/**
 * @brief PartList subclass that maintains bidirectional parent/child relationships.
 *
 * When a Component is added to a ChildList, it is automatically added to the
 * child's parent list (and vice versa for ParentList). Removal also maintains
 * the opposite relationship.
 */
class ChildList : public PartList<Component> {
  public:
    explicit ChildList(Component* owner);
    void Added(std::shared_ptr<Component> part, const bool forced) override;
    void Removed(std::shared_ptr<Component> part, const bool forced) override;

  private:
    Component* mOwner;
};

/**
 * @brief PartList subclass that maintains bidirectional child/parent relationships.
 *
 * When a Component is added to a ParentList, it is automatically added to the
 * parent's child list (and vice versa). Removal also maintains the opposite
 * relationship.
 */
class ParentList : public PartList<Component> {
  public:
    explicit ParentList(Component* owner);
    void Added(std::shared_ptr<Component> part, const bool forced) override;
    void Removed(std::shared_ptr<Component> part, const bool forced) override;

  private:
    Component* mOwner;
};

/**
 * @brief A named Part with a parent/child hierarchy and optional thread safety.
 *
 * Component extends Part with a human-readable name, a string representation,
 * and bidirectional parent/child relationships managed via PartList. Adding a
 * child automatically adds the corresponding parent, and vice versa. When the
 * COMPONENT_THREAD_SAFE preprocessor flag is defined, all relationship
 * mutations are guarded by a shared_mutex.
 */
class Component : public Part, public std::enable_shared_from_this<Component> {
  public:
    /**
     * @brief Constructs a Component with the given name.
     * @param name A human-readable name for this Component.
     */
    explicit Component(const std::string& name);
    virtual ~Component();

    /** @brief Returns the name of this Component. */
    const std::string& GetName() const;

    /** @brief Returns a human-readable string representation (e.g. "Name (id)"). */
    std::string ToString() const;

    /** @brief Conversion operator to std::string; equivalent to ToString(). */
    explicit operator std::string() const;

#ifdef COMPONENT_THREAD_SAFE
    /**
     * @brief Returns a reference to the Component's shared mutex.
     *
     * Only available when COMPONENT_THREAD_SAFE is defined. Callers can use
     * this to synchronize external access to the Component.
     */
    std::shared_mutex& GetMutex() const;
#endif

    // ---- Parent/child relationship accessors ----

    /** @brief Returns a mutable reference to the parent list. */
    PartList<Component>& GetParents();
    /** @brief Returns a const reference to the parent list. */
    const PartList<Component>& GetParents() const;

    /** @brief Returns a mutable reference to the child list. */
    PartList<Component>& GetChildren();
    /** @brief Returns a const reference to the child list. */
    const PartList<Component>& GetChildren() const;

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

  private:
    std::string mName;
    ParentList mParents;
    ChildList mChildren;
#ifdef COMPONENT_THREAD_SAFE
    mutable std::shared_mutex mMutex;
#endif
};

// ---- Template BFS implementations ----

template <typename T> bool Component::HasInChildren() const {
    std::queue<const Component*> queue;
    std::unordered_set<uint64_t> visited;
    visited.insert(GetId());

    auto directChildren = GetChildren().Get();
    for (const auto& child : *directChildren) {
        if (visited.insert(child->GetId()).second) {
            if (std::dynamic_pointer_cast<T>(child)) {
                return true;
            }
            queue.push(child.get());
        }
    }

    while (!queue.empty()) {
        const Component* current = queue.front();
        queue.pop();
        auto grandChildren = current->GetChildren().Get();
        for (const auto& child : *grandChildren) {
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

template <typename T> std::shared_ptr<T> Component::GetFirstInChildren() const {
    std::queue<const Component*> queue;
    std::unordered_set<uint64_t> visited;
    visited.insert(GetId());

    auto directChildren = GetChildren().Get();
    for (const auto& child : *directChildren) {
        if (visited.insert(child->GetId()).second) {
            auto typed = std::dynamic_pointer_cast<T>(child);
            if (typed) {
                return typed;
            }
            queue.push(child.get());
        }
    }

    while (!queue.empty()) {
        const Component* current = queue.front();
        queue.pop();
        auto grandChildren = current->GetChildren().Get();
        for (const auto& child : *grandChildren) {
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

    auto directChildren = GetChildren().Get();
    for (const auto& child : *directChildren) {
        if (visited.insert(child->GetId()).second) {
            auto typed = std::dynamic_pointer_cast<T>(child);
            if (typed) {
                result->push_back(typed);
            }
            queue.push(child.get());
        }
    }

    while (!queue.empty()) {
        const Component* current = queue.front();
        queue.pop();
        auto grandChildren = current->GetChildren().Get();
        for (const auto& child : *grandChildren) {
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

} // namespace Ship
