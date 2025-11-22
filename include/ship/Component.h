#pragma once

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <stdint.h>

namespace Ship {
#define INVALID_COMPONENT_ID -1

class Component : private std::enable_shared_from_this<Component> {
  public:
    Component(const std::string& name);
    ~Component();

    int32_t GetId() const;
    std::string GetName() const;
    std::string ToString() const;
    explicit operator std::string() const;
    bool operator==(const Component& other) const;

    bool HasParent(std::shared_ptr<Component> parent);
    bool HasChild(std::shared_ptr<Component> child);
    template <typename T> bool HasParent(const std::string name);
    template <typename T> bool HasChild(const std::string name);
    template <typename T> bool HasParent();
    template <typename T> bool HasChild();
    bool HasParent(const int32_t parent);
    bool HasChild(const int32_t child);
    bool HasParent(const std::string& parent);
    bool HasChild(const std::string& child);
    bool HasParent();
    bool HasChild();
    std::shared_ptr<Component> GetParent(const int32_t parent);
    std::shared_ptr<Component> GetChild(const int32_t child);
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetParents();
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetChildren();
    size_t GetParentsCount();
    size_t GetChildrenCount();
    template <typename T> std::shared_ptr<T> GetParents(const std::string& parent);
    template <typename T> std::shared_ptr<T> GetChildren(const std::string& child);
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> GetParents();
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> GetChildren();
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetParents(const std::vector<int32_t>& parents);
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetChildren(const std::vector<int32_t>& children);
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetParents(const std::string& parent);
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetChildren(const std::string& child);
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetParents(const std::vector<std::string>& parents);
    std::shared_ptr<std::vector<std::shared_ptr<Component>>> GetChildren(const std::vector<std::string>& children);
    bool AddParent(std::shared_ptr<Component> parent, const bool force = false);
    bool AddChild(std::shared_ptr<Component> child, const bool force = false);
    bool AddParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force = false);
    bool AddChildren(const std::vector<std::shared_ptr<Component>>& children, const bool force = false);
    bool RemoveParent(std::shared_ptr<Component> parent, const bool force = false);
    bool RemoveChild(std::shared_ptr<Component> child, const bool force = false);
    bool RemoveParent(const int32_t parent, bool force = false);
    bool RemoveChild(const int32_t child, bool force = false);
    bool RemoveParents(const bool force = false);
    bool RemoveChildren(const bool force = false);
    bool RemoveParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force = false);
    bool RemoveChildren(const std::vector<std::shared_ptr<Component>>& children, const bool force = false);
    template <typename T> bool RemoveParents(const std::string& name, const bool force = false);
    template <typename T> bool RemoveChildren(const std::string& name, const bool force = false);
    template <typename T> bool RemoveParents(const bool force = false);
    template <typename T> bool RemoveChildren(const bool force = false);
    bool RemoveParents(const std::vector<int32_t>& parents, const bool force = false);
    bool RemoveChildren(const std::vector<int32_t>& children, const bool force = false);
    bool RemoveParents(const std::string& parent, const bool force = false);
    bool RemoveChildren(const std::string& child, const bool force = false);
    bool RemoveParents(const std::vector<std::string>& parents, const bool force = false);
    bool RemoveChildren(const std::vector<std::string>& children, const bool force = false);

protected:
    virtual bool CanAddParent(std::shared_ptr<Component> parent) = 0;
    virtual bool CanAddChild(std::shared_ptr<Component> child) = 0;
    virtual bool CanRemoveParent(std::shared_ptr<Component> parent) = 0;
    virtual bool CanRemoveChild(std::shared_ptr<Component> child) = 0;
    virtual void AddedParent(std::shared_ptr<Component> parent, const bool forced) = 0;
    virtual void AddedChild(std::shared_ptr<Component> child, const bool forced) = 0;
    virtual void RemovedParent(std::shared_ptr<Component> parent, const bool forced) = 0;
    virtual void RemovedChild(std::shared_ptr<Component> child, const bool forced) = 0;

private:
    Component& AddParentRaw(std::shared_ptr<Component> parent);
    Component& AddChildRaw(std::shared_ptr<Component> child);
    Component& RemoveParentRaw(std::shared_ptr<Component> parent);
    Component& RemoveChildRaw(std::shared_ptr<Component> child);

    static std::atomic_int NextComponentId;

    int32_t mId;
    std::string mName;
    std::vector<std::shared_ptr<Component>> mParents;
    std::vector<std::shared_ptr<Component>> mChildren;
#ifdef INCLUDE_MUTEX
    std::recursive_mutex mMutex;
#endif
};

} // namespace Ship
