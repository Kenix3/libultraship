#pragma once

#include <string>
#include <memory>
#include <vector>
#include <algorithm>

#include "ship/Component.h"

namespace Ship {

// ComponentList extends PartList<Component> with name- and type-based search helpers.
class ComponentList : public PartList<Component> {
  public:
    using PartList<Component>::PartList;

    bool Has(const std::string& name) const;
    template <typename T> bool Has(const std::string& name) const;

    std::shared_ptr<std::vector<std::shared_ptr<Component>>> Get(const std::string& name) const;
    template <typename T>
    std::shared_ptr<std::vector<std::shared_ptr<T>>> Get(const std::string& name) const;
    std::shared_ptr<std::vector<std::shared_ptr<Component>>>
    Get(const std::vector<std::string>& names) const;
};

inline bool ComponentList::Has(const std::string& name) const {
    const auto& list = this->GetList();
    return std::find_if(list.begin(), list.end(),
               [&name](const std::shared_ptr<Component>& c) {
                   return c->GetName() == name;
               }) != list.end();
}

template <typename T>
bool ComponentList::Has(const std::string& name) const {
    const auto& list = this->GetList();
    return std::find_if(list.begin(), list.end(),
               [&name](const std::shared_ptr<Component>& c) {
                   return c->GetName() == name && std::dynamic_pointer_cast<T>(c) != nullptr;
               }) != list.end();
}

inline std::shared_ptr<std::vector<std::shared_ptr<Component>>>
ComponentList::Get(const std::string& name) const {
    auto result = std::make_shared<std::vector<std::shared_ptr<Component>>>();
    for (const auto& c : this->GetList()) {
        if (c->GetName() == name) {
            result->push_back(c);
        }
    }
    return result;
}

template <typename T>
std::shared_ptr<std::vector<std::shared_ptr<T>>>
ComponentList::Get(const std::string& name) const {
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

