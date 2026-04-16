#pragma once

#include <string>
#include <memory>
#include <vector>
#include <algorithm>

#include "ship/Component.h"

namespace Ship {

/**
 * @brief Extends PartList<Component> with name- and type-based search helpers.
 *
 * Provides overloaded Has() and Get() methods that look up Components by their
 * human-readable name, optionally filtered by derived type via dynamic_cast.
 */
class ComponentList : public PartList<Component> {
  public:
    using PartList<Component>::PartList;

    /**
     * @brief Checks whether a Component with the given name exists.
     * @param name The Component name to search for.
     * @return True if at least one Component with that name is present.
     */
    bool Has(const std::string& name) const;

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
};

inline bool ComponentList::Has(const std::string& name) const {
    const auto& list = this->GetList();
    return std::find_if(list.begin(), list.end(),
                        [&name](const std::shared_ptr<Component>& c) { return c->GetName() == name; }) != list.end();
}

template <typename T> bool ComponentList::Has(const std::string& name) const {
    const auto& list = this->GetList();
    return std::find_if(list.begin(), list.end(), [&name](const std::shared_ptr<Component>& c) {
               return c->GetName() == name && std::dynamic_pointer_cast<T>(c) != nullptr;
           }) != list.end();
}

inline std::shared_ptr<std::vector<std::shared_ptr<Component>>> ComponentList::Get(const std::string& name) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<Component>>>();
    for (const auto& c : this->GetList()) {
        if (c->GetName() == name) {
            result->push_back(c);
        }
    }
    return result;
}

template <typename T>
std::shared_ptr<std::vector<std::shared_ptr<T>>> ComponentList::Get(const std::string& name) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<T>>>();
    for (const auto& c : this->GetList()) {
        auto typed = std::dynamic_pointer_cast<T>(c);
        if (typed && c->GetName() == name) {
            result->push_back(typed);
        }
    }
    return result;
}

inline std::shared_ptr<std::vector<std::shared_ptr<Component>>>
ComponentList::Get(const std::vector<std::string>& names) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<Component>>>();
    for (const auto& c : this->GetList()) {
        if (std::find(names.begin(), names.end(), c->GetName()) != names.end()) {
            result->push_back(c);
        }
    }
    return result;
}

} // namespace Ship
