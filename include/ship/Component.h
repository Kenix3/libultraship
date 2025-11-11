#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace Ship {
class Component : private std::enable_shared_from_this<Component> {
  public:
    Component(const std::string& name);
    ~Component();

    bool DrawDebugMenu();

    int GetId() const;
    std::string GetName() const;
    std::string ToString() const;
    explicit operator std::string() const;

    std::shared_ptr<Component> GetParent(const std::string& parent);
    std::shared_ptr<Component> GetChild(const std::string& child);
    std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Component>>> GetChildren();
    std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Component>>> GetParents();
    bool HasParent(std::shared_ptr<Component> parent);
    bool HasParent(const std::string& parent);
    bool HasParent();
    bool HasChild(std::shared_ptr<Component> child);
    bool HasChild(const std::string& child);
    bool HasChild();
    Component& AddParent(std::shared_ptr<Component> parent, bool now = false);
    Component& AddChild(std::shared_ptr<Component> child, bool now = false);
    Component& RemoveParent(std::shared_ptr<Component> parent, bool now = false);
    Component& RemoveChild(std::shared_ptr<Component> child, bool now = false);
    Component& RemoveParent(const std::string& parent, bool now = false);
    Component& RemoveChild(const std::string& child, bool now = false);
    Component& AddParents(const std::unordered_map<std::string, std::shared_ptr<Component>>& parents, bool now = false);
    Component& AddChildren(const std::unordered_map<std::string, std::shared_ptr<Component>>& children,
                           bool now = false);
    Component& AddParents(const std::vector<std::shared_ptr<Component>>& parents, bool now = false);
    Component& AddChildren(const std::vector<std::shared_ptr<Component>>& children, bool now = false);
    Component& RemoveParents(const std::unordered_map<std::string, std::shared_ptr<Component>>& parents,
                             bool now = false);
    Component& RemoveChildren(const std::unordered_map<std::string, std::shared_ptr<Component>>& children,
                              bool now = false);
    Component& RemoveParents(const std::vector<std::shared_ptr<Component>>& parents, bool now = false);
    Component& RemoveChildren(const std::vector<std::shared_ptr<Component>>& children, bool now = false);
    Component& RemoveParents(const std::vector<std::string>& parents, bool now = false);
    Component& RemoveChildren(const std::vector<std::string>& children, bool now = false);
    Component& RemoveParents(bool now = false);
    Component& RemoveChildren(bool now = false);

protected:
    virtual bool DebugMenuDrawn() = 0;
    virtual bool AddedParent(std::shared_ptr<Component> parent) = 0;
    virtual bool AddedChild(std::shared_ptr<Component> child) = 0;
    virtual bool RemovedParent(std::shared_ptr<Component> parent) = 0;
    virtual bool RemovedChild(std::shared_ptr<Component> child) = 0;

private:
    Component& AddParentRaw(std::shared_ptr<Component> parent);
    Component& AddChildRaw(std::shared_ptr<Component> child);
    Component& RemoveParentRaw(std::shared_ptr<Component> parent);
    Component& RemoveChildRaw(std::shared_ptr<Component> child);

    static std::atomic_int NextComponentId;

    int mId;
    std::string mName;

    std::recursive_mutex mMutex;

    std::unordered_map<std::string, std::shared_ptr<Component>> mParentsToAdd;
    std::unordered_map<std::string, std::shared_ptr<Component>> mChildrenToAdd;
    std::unordered_map<std::string, std::shared_ptr<Component>> mParentsToRemove;
    std::unordered_map<std::string, std::shared_ptr<Component>> mChildrenToRemove;
    std::unordered_map<std::string, std::shared_ptr<Component>> mParents;
    std::unordered_map<std::string, std::shared_ptr<Component>> mChildren;
};

} // namespace Ship
