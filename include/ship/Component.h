#pragma once

#include <string>
#include <memory>
#include <vector>
#include <algorithm>

#ifdef COMPONENT_THREAD_SAFE
#include <shared_mutex>
#endif

#include "ship/Part.h"
#include "ship/PartList.h"

namespace Ship {

/**
 * @brief A named Part with a parent/child hierarchy and optional thread safety.
 *
 * Component extends Part with a human-readable name, a string representation,
 * and bidirectional parent/child relationships managed via PartList. When the
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
    std::string GetName() const;

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
    std::shared_mutex& GetMutex();
#endif

    // ---- Parent/child relationship management ----

    /** @brief Returns a mutable reference to the parent list. */
    PartList<Component>& GetParents();
    /** @brief Returns a const reference to the parent list. */
    const PartList<Component>& GetParents() const;

    /** @brief Returns a mutable reference to the child list. */
    PartList<Component>& GetChildren();
    /** @brief Returns a const reference to the child list. */
    const PartList<Component>& GetChildren() const;

    /**
     * @brief Adds a parent Component.
     * @param parent The Component to add as a parent.
     * @param force If true, bypass the CanAddParent check.
     * @return True if the parent was successfully added.
     */
    bool AddParent(std::shared_ptr<Component> parent, const bool force = false);

    /**
     * @brief Adds multiple parent Components.
     * @param parents The Components to add as parents.
     * @param force If true, bypass permission checks.
     * @return True if all parents were successfully added.
     */
    bool AddParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force = false);

    /**
     * @brief Adds a child Component.
     * @param child The Component to add as a child.
     * @param force If true, bypass the CanAddChild check.
     * @return True if the child was successfully added.
     */
    bool AddChild(std::shared_ptr<Component> child, const bool force = false);

    /**
     * @brief Adds multiple child Components.
     * @param children The Components to add as children.
     * @param force If true, bypass permission checks.
     * @return True if all children were successfully added.
     */
    bool AddChildren(const std::vector<std::shared_ptr<Component>>& children, const bool force = false);

    /**
     * @brief Removes a parent Component by pointer.
     * @param parent The parent to remove.
     * @param force If true, bypass the CanRemoveParent check.
     * @return True if the parent was successfully removed.
     */
    bool RemoveParent(std::shared_ptr<Component> parent, const bool force = false);

    /**
     * @brief Removes a parent Component by its unique ID.
     * @param parentId The ID of the parent to remove.
     * @param force If true, bypass permission checks.
     * @return True if the parent was found and removed.
     */
    bool RemoveParent(const uint64_t parentId, const bool force = false);

    /**
     * @brief Removes all parents.
     * @param force If true, bypass permission checks.
     * @return True if all parents were successfully removed.
     */
    bool RemoveParents(const bool force = false);

    /**
     * @brief Removes the specified parent Components.
     * @param parents The parents to remove.
     * @param force If true, bypass permission checks.
     * @return True if all specified parents were successfully removed.
     */
    bool RemoveParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force = false);

    /**
     * @brief Removes parent Components by their IDs.
     * @param parentIds The IDs of the parents to remove.
     * @param force If true, bypass permission checks.
     * @return True if all specified parents were successfully removed.
     */
    bool RemoveParents(const std::vector<uint64_t>& parentIds, const bool force = false);

    /**
     * @brief Removes all parents matching the given name.
     * @param name The name to match.
     * @param force If true, bypass permission checks.
     * @return True if all matching parents were successfully removed.
     */
    bool RemoveParents(const std::string& name, const bool force = false);

    /**
     * @brief Removes all parents matching any of the given names.
     * @param names The names to match.
     * @param force If true, bypass permission checks.
     * @return True if all matching parents were successfully removed.
     */
    bool RemoveParents(const std::vector<std::string>& names, const bool force = false);

    /**
     * @brief Removes all parents of type T.
     * @tparam T The derived Component type to match via dynamic_cast.
     * @param force If true, bypass permission checks.
     * @return True if all matching parents were successfully removed.
     */
    template <typename T> bool RemoveParents(const bool force = false);

    /**
     * @brief Removes all parents of type T with the given name.
     * @tparam T The derived Component type to match via dynamic_cast.
     * @param name The name to match.
     * @param force If true, bypass permission checks.
     * @return True if all matching parents were successfully removed.
     */
    template <typename T> bool RemoveParents(const std::string& name, const bool force = false);

    /**
     * @brief Removes a child Component by pointer.
     * @param child The child to remove.
     * @param force If true, bypass the CanRemoveChild check.
     * @return True if the child was successfully removed.
     */
    bool RemoveChild(std::shared_ptr<Component> child, const bool force = false);

    /**
     * @brief Removes a child Component by its unique ID.
     * @param childId The ID of the child to remove.
     * @param force If true, bypass permission checks.
     * @return True if the child was found and removed.
     */
    bool RemoveChild(const uint64_t childId, const bool force = false);

    /**
     * @brief Removes all children.
     * @param force If true, bypass permission checks.
     * @return True if all children were successfully removed.
     */
    bool RemoveChildren(const bool force = false);

    /**
     * @brief Removes the specified child Components.
     * @param children The children to remove.
     * @param force If true, bypass permission checks.
     * @return True if all specified children were successfully removed.
     */
    bool RemoveChildren(const std::vector<std::shared_ptr<Component>>& children, const bool force = false);

    /**
     * @brief Removes child Components by their IDs.
     * @param childIds The IDs of the children to remove.
     * @param force If true, bypass permission checks.
     * @return True if all specified children were successfully removed.
     */
    bool RemoveChildren(const std::vector<uint64_t>& childIds, const bool force = false);

    /**
     * @brief Removes all children matching the given name.
     * @param name The name to match.
     * @param force If true, bypass permission checks.
     * @return True if all matching children were successfully removed.
     */
    bool RemoveChildren(const std::string& name, const bool force = false);

    /**
     * @brief Removes all children matching any of the given names.
     * @param names The names to match.
     * @param force If true, bypass permission checks.
     * @return True if all matching children were successfully removed.
     */
    bool RemoveChildren(const std::vector<std::string>& names, const bool force = false);

    /**
     * @brief Removes all children of type T.
     * @tparam T The derived Component type to match via dynamic_cast.
     * @param force If true, bypass permission checks.
     * @return True if all matching children were successfully removed.
     */
    template <typename T> bool RemoveChildren(const bool force = false);

    /**
     * @brief Removes all children of type T with the given name.
     * @tparam T The derived Component type to match via dynamic_cast.
     * @param name The name to match.
     * @param force If true, bypass permission checks.
     * @return True if all matching children were successfully removed.
     */
    template <typename T> bool RemoveChildren(const std::string& name, const bool force = false);

  protected:
    /**
     * @brief Permission hook called before adding a parent. Override to deny.
     * @param parent The parent about to be added.
     * @return True if the addition is permitted.
     */
    virtual bool CanAddParent(std::shared_ptr<Component> parent);

    /**
     * @brief Permission hook called before adding a child. Override to deny.
     * @param child The child about to be added.
     * @return True if the addition is permitted.
     */
    virtual bool CanAddChild(std::shared_ptr<Component> child);

    /**
     * @brief Permission hook called before removing a parent. Override to deny.
     * @param parent The parent about to be removed.
     * @return True if the removal is permitted.
     */
    virtual bool CanRemoveParent(std::shared_ptr<Component> parent);

    /**
     * @brief Permission hook called before removing a child. Override to deny.
     * @param child The child about to be removed.
     * @return True if the removal is permitted.
     */
    virtual bool CanRemoveChild(std::shared_ptr<Component> child);

    /**
     * @brief Notification hook called after a parent has been added.
     * @param parent The parent that was added.
     * @param forced Whether the addition bypassed permission checks.
     */
    virtual void AddedParent(std::shared_ptr<Component> parent, const bool forced);

    /**
     * @brief Notification hook called after a child has been added.
     * @param child The child that was added.
     * @param forced Whether the addition bypassed permission checks.
     */
    virtual void AddedChild(std::shared_ptr<Component> child, const bool forced);

    /**
     * @brief Notification hook called after a parent has been removed.
     * @param parent The parent that was removed.
     * @param forced Whether the removal bypassed permission checks.
     */
    virtual void RemovedParent(std::shared_ptr<Component> parent, const bool forced);

    /**
     * @brief Notification hook called after a child has been removed.
     * @param child The child that was removed.
     * @param forced Whether the removal bypassed permission checks.
     */
    virtual void RemovedChild(std::shared_ptr<Component> child, const bool forced);

  private:
    std::string mName;
    PartList<Component> mParents;
    PartList<Component> mChildren;
#ifdef COMPONENT_THREAD_SAFE
    mutable std::shared_mutex mMutex;
#endif
};

// ---- Template method implementations (Component is complete here) ----

template <typename T> bool Component::RemoveParents(const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mParents.Get();
    }
    bool ok = true;
    for (const auto& p : *snapshot) {
        if (std::dynamic_pointer_cast<T>(p)) {
            ok &= RemoveParent(p, force);
        }
    }
    return ok;
}

template <typename T> bool Component::RemoveParents(const std::string& name, const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mParents.Get();
    }
    bool ok = true;
    for (const auto& p : *snapshot) {
        if (p->GetName() == name && std::dynamic_pointer_cast<T>(p)) {
            ok &= RemoveParent(p, force);
        }
    }
    return ok;
}

template <typename T> bool Component::RemoveChildren(const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mChildren.Get();
    }
    bool ok = true;
    for (const auto& c : *snapshot) {
        if (std::dynamic_pointer_cast<T>(c)) {
            ok &= RemoveChild(c, force);
        }
    }
    return ok;
}

template <typename T> bool Component::RemoveChildren(const std::string& name, const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef COMPONENT_THREAD_SAFE
        const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
        snapshot = mChildren.Get();
    }
    bool ok = true;
    for (const auto& c : *snapshot) {
        if (c->GetName() == name && std::dynamic_pointer_cast<T>(c)) {
            ok &= RemoveChild(c, force);
        }
    }
    return ok;
}

} // namespace Ship
