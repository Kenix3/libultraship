#include "ship/ComponentList.h"
#include "ship/Component.h"

namespace Ship {

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
