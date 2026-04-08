#pragma once

#include <string>
#include <memory>
#include <vector>
#include <shared_mutex>

#include "ship/Part.h"
#include "ship/PartList.h"

#define INCLUDE_MUTEX 1

namespace Ship {
class Component : public Part, public std::enable_shared_from_this<Component> {
  public:
    Component(const std::string& name);
    ~Component();

    std::string GetName() const;
    std::string ToString() const;
    explicit operator std::string() const;
#ifdef INCLUDE_MUTEX
    std::shared_mutex& GetMutex();
#endif

    // TODO: Move to ComponentList
    template <typename T> bool HasParent(const std::string name);
    template <typename T> bool HasChild(const std::string name);
    bool HasParent(const std::string& parent);
    bool HasChild(const std::string& child);
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> GetParents(const std::string& parent);
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> GetChildren(const std::string& child);
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetParents(const std::string& parent);
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetChildren(const std::string& child);
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetParents(const std::vector<std::string>& parents);
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetChildren(const std::vector<std::string>& children);
    template <typename T> bool RemoveParents(const std::string& name, const bool force = false);
    template <typename T> bool RemoveChildren(const std::string& name, const bool force = false);
    bool RemoveParents(const std::string& parent, const bool force = false);
    bool RemoveChildren(const std::string& child, const bool force = false);
    bool RemoveParents(const std::vector<std::string>& parents, const bool force = false);
    bool RemoveChildren(const std::vector<std::string>& children, const bool force = false);

protected:
    // TODO: Update to account for the new lists.
    virtual bool CanAddParent(std::shared_ptr<Component> parent);
    virtual bool CanAddChild(std::shared_ptr<Component> child);
    virtual bool CanRemoveParent(std::shared_ptr<Component> parent);
    virtual bool CanRemoveChild(std::shared_ptr<Component> child);
    virtual void AddedParent(std::shared_ptr<Component> parent, const bool forced);
    virtual void AddedChild(std::shared_ptr<Component> child, const bool forced);
    virtual void RemovedParent(std::shared_ptr<Component> parent, const bool forced);
    virtual void RemovedChild(std::shared_ptr<Component> child, const bool forced);

private:
    std::string mName;
    ComponentList mChildren;
#ifdef INCLUDE_MUTEX
    std::shared_mutex mMutex;
#endif
};

} // namespace Ship
