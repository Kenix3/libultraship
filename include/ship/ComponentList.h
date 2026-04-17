#pragma once

#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <mutex>

#include "ship/PartList.h"

namespace Ship {

class Component;

/**
 * @brief Describes the role of a ComponentList within a parent/child relationship.
 *
 * When a ComponentList has a role, its Added()/Removed() hooks automatically
 * maintain the bidirectional relationship on the linked Component.
 */
enum class ComponentListRole {
    None,     /**< @brief No automatic relationship management. */
    Children, /**< @brief This list holds children; adding updates the child's parent list. */
    Parents   /**< @brief This list holds parents; adding updates the parent's child list. */
};

/**
 * @brief Extends PartList<Component> with name- and type-based search helpers.
 *
 * Provides overloaded Has() and Get() methods that look up Components by their
 * human-readable name, optionally filtered by derived type via dynamic_cast.
 *
 * When constructed with a ComponentListRole and an owner pointer, the list
 * automatically maintains bidirectional parent/child relationships and
 * auto-registers/unregisters TickableComponents with the Context's TickableList.
 */
class ComponentList : public PartList<Component> {
  public:
    using PartList<Component>::PartList;

    /**
     * @brief Constructs a ComponentList with a role and owner for bidirectional relationships.
     * @param owner The Component that owns this list (raw pointer, must outlive the list).
     * @param role  The relationship role (Children or Parents).
     */
    ComponentList(Component* owner, ComponentListRole role);

    /**
     * @brief Checks whether a Component with the given name exists.
     * @param name The Component name to search for.
     * @return True if at least one Component with that name is present.
     */
    bool Has(const std::string& name) const;

    // Pull in base-class Has overloads so they're not hidden
    using PartList<Component>::Has;

    /**
     * @brief Checks whether a Component of type T with the given name exists.
     * @tparam T The derived type to match via dynamic_cast.
     * @param name The Component name to search for.
     * @return True if a matching Component is found.
     */
    template <typename T> bool Has(const std::string& name) const;

    /**
     * @brief Returns all Components with the given name.
     * @param name The Component name to search for.
     * @return A vector of matching Components.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> Get(const std::string& name) const;

    // Pull in base-class Get overloads so they're not hidden
    using PartList<Component>::Get;

    /**
     * @brief Returns all Components of type T with the given name.
     * @tparam T The derived type to match via dynamic_cast.
     * @param name The Component name to search for.
     * @return A vector of matching Components cast to T.
     */
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> Get(const std::string& name) const;

    /**
     * @brief Returns all Components matching any of the given names.
     * @param names A vector of names to search for.
     * @return A vector of matching Components.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> Get(const std::vector<std::string>& names) const;

  protected:
    void Added(std::shared_ptr<Component> part, const bool forced) override;
    void Removed(std::shared_ptr<Component> part, const bool forced) override;

  private:
    Component* mOwner = nullptr;
    ComponentListRole mRole = ComponentListRole::None;
};

} // namespace Ship
