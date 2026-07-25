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
 * @brief Extends PartList<Component, StoredPtr> with name- and type-based search helpers.
 *
 * @tparam StoredPtr Backing pointer storage type (`std::shared_ptr<Component>`
 *                   for owning child lists, `std::weak_ptr<Component>` for
 *                   non-owning parent lists).
 */
template <typename StoredPtr> class BasicComponentList : public PartList<Component, StoredPtr> {
  public:
    using PartListBase = PartList<Component, StoredPtr>;
    using PartListBase::Get;
    using PartListBase::Has;
    using PartListBase::PartList;

    BasicComponentList() = default;

    /**
     * @brief Constructs a list with relationship role metadata.
     * @param owner The owning component of this list.
     * @param role  Role of this list in relationship synchronization.
     */
    BasicComponentList(Component* owner, ComponentListRole role);

    /**
     * @brief Checks whether any component with the given name exists.
     * @param name Name to search for.
     * @return True if any live entry has this name.
     */
    bool Has(const std::string& name) const;

    /**
     * @brief Checks whether any component with the given name matches type T.
     * @tparam T Target type tested via dynamic_cast.
     * @param name Name to search for.
     * @return True if a typed component with matching name exists.
     */
    template <typename T> bool Has(const std::string& name) const;

    /**
     * @brief Returns all components with the given name.
     * @param name Name to search for.
     * @return Matching live components.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> Get(const std::string& name) const;

    /**
     * @brief Returns all components of type T with the given name.
     * @tparam T Target type tested via dynamic_cast.
     * @param name Name to search for.
     * @return Matching typed components.
     */
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> Get(const std::string& name) const;

    /**
     * @brief Returns all components whose names are in the provided set.
     * @param names Names to match.
     * @return Matching live components.
     */
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> Get(const std::vector<std::string>& names) const;

  protected:
    /**
     * @brief Synchronization hook called after a component is added.
     *
     * Maintains bidirectional parent/child links and tickable registration.
     */
    void Added(std::shared_ptr<Component> part, const bool forced) override;

    /**
     * @brief Synchronization hook called after a component is removed.
     *
     * Maintains bidirectional parent/child links and tickable unregistration.
     */
    void Removed(std::shared_ptr<Component> part, const bool forced) override;

  private:
    Component* mOwner = nullptr;
    ComponentListRole mRole = ComponentListRole::None;
};

/** @brief Owning child-component list storage type. */
using ComponentList = BasicComponentList<std::shared_ptr<Component>>;
/** @brief Non-owning parent-component list storage type. */
using ParentComponentList = BasicComponentList<std::weak_ptr<Component>>;

} // namespace Ship
