#pragma once

#include <string>
#include <memory>
#include <vector>
#include <algorithm>

#ifdef INCLUDE_PROFILING
#include <shared_mutex>
#endif

#include "ship/Part.h"
#include "ship/PartList.h"

namespace Ship {

class Component : public Part, public std::enable_shared_from_this<Component> {
  public:
    explicit Component(const std::string& name);
    virtual ~Component();

    std::string GetName() const;
    std::string ToString() const;
    explicit operator std::string() const;

#ifdef INCLUDE_PROFILING
    std::shared_mutex& GetMutex();
#endif

    // ---- Parent/child relationship management ----
    bool HasParent(std::shared_ptr<Component> parent) const;
    bool HasParent(const std::string& name) const;
    template <typename T> bool HasParent(const std::string& name) const;

    bool HasChild(std::shared_ptr<Component> child) const;
    bool HasChild(const std::string& name) const;
    template <typename T> bool HasChild(const std::string& name) const;

    PartList<Component>& GetParents();
    const PartList<Component>& GetParents() const;

    PartList<Component>& GetChildren();
    const PartList<Component>& GetChildren() const;

    bool AddParent(std::shared_ptr<Component> parent, const bool force = false);
    bool AddParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force = false);

    bool AddChild(std::shared_ptr<Component> child, const bool force = false);
    bool AddChildren(const std::vector<std::shared_ptr<Component>>& children, const bool force = false);

    bool RemoveParent(std::shared_ptr<Component> parent, const bool force = false);
    bool RemoveParent(const uint64_t parentId, const bool force = false);
    bool RemoveParents(const bool force = false);
    bool RemoveParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force = false);
    bool RemoveParents(const std::vector<uint64_t>& parentIds, const bool force = false);
    bool RemoveParents(const std::string& name, const bool force = false);
    bool RemoveParents(const std::vector<std::string>& names, const bool force = false);
    template <typename T> bool RemoveParents(const bool force = false);
    template <typename T> bool RemoveParents(const std::string& name, const bool force = false);

    bool RemoveChild(std::shared_ptr<Component> child, const bool force = false);
    bool RemoveChild(const uint64_t childId, const bool force = false);
    bool RemoveChildren(const bool force = false);
    bool RemoveChildren(const std::vector<std::shared_ptr<Component>>& children, const bool force = false);
    bool RemoveChildren(const std::vector<uint64_t>& childIds, const bool force = false);
    bool RemoveChildren(const std::string& name, const bool force = false);
    bool RemoveChildren(const std::vector<std::string>& names, const bool force = false);
    template <typename T> bool RemoveChildren(const bool force = false);
    template <typename T> bool RemoveChildren(const std::string& name, const bool force = false);

  protected:
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
    PartList<Component> mParents;
    PartList<Component> mChildren;
#ifdef INCLUDE_PROFILING
    mutable std::shared_mutex mMutex;
#endif
};

// ---- Template method implementations (Component is complete here) ----

template <typename T>
bool Component::HasParent(const std::string& name) const {
#ifdef INCLUDE_PROFILING
    const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
    const auto& list = mParents.GetList();
    return std::find_if(list.begin(), list.end(),
               [&name](const std::shared_ptr<Component>& c) {
                   return c->GetName() == name && std::dynamic_pointer_cast<T>(c) != nullptr;
               }) != list.end();
}

template <typename T>
bool Component::HasChild(const std::string& name) const {
#ifdef INCLUDE_PROFILING
    const std::shared_lock<std::shared_mutex> lock(mMutex);
#endif
    const auto& list = mChildren.GetList();
    return std::find_if(list.begin(), list.end(),
               [&name](const std::shared_ptr<Component>& c) {
                   return c->GetName() == name && std::dynamic_pointer_cast<T>(c) != nullptr;
               }) != list.end();
}

template <typename T>
bool Component::RemoveParents(const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef INCLUDE_PROFILING
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

template <typename T>
bool Component::RemoveParents(const std::string& name, const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef INCLUDE_PROFILING
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

template <typename T>
bool Component::RemoveChildren(const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef INCLUDE_PROFILING
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

template <typename T>
bool Component::RemoveChildren(const std::string& name, const bool force) {
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> snapshot;
    {
#ifdef INCLUDE_PROFILING
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

