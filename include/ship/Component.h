#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

// TODO: Compiler define for the mutex.

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
    bool AddParent(std::shared_ptr<Component> parent, const bool force = false);
    bool AddChild(std::shared_ptr<Component> child, const bool force = false);
    bool RemoveParent(std::shared_ptr<Component> parent, const bool force = false);
    bool RemoveChild(std::shared_ptr<Component> child, const bool force = false);
    bool RemoveParent(const std::string& parent, const bool force = false);
    bool RemoveChild(const std::string& child, const bool force = false);
    bool AddParents(const std::unordered_map<std::string, std::shared_ptr<Component>>& parents,
                          const bool force = false);
    bool AddChildren(const std::unordered_map<std::string, std::shared_ptr<Component>>& children,
                           const bool force = false);
    bool AddParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force = false);
    bool AddChildren(const std::vector<std::shared_ptr<Component>>& children, const bool force = false);
    bool RemoveParents(const std::unordered_map<std::string, std::shared_ptr<Component>>& parents,
                             const bool force = false);
    bool RemoveChildren(const std::unordered_map<std::string, std::shared_ptr<Component>>& children,
                              const bool force = false);
    bool RemoveParents(const std::vector<std::shared_ptr<Component>>& parents, const bool force = false);
    bool RemoveChildren(const std::vector<std::shared_ptr<Component>>& children, const bool force = false);
    bool RemoveParents(const std::vector<std::string>& parents, const bool force = false);
    bool RemoveChildren(const std::vector<std::string>& children, const bool force = false);
    bool RemoveParents(const bool force = false);
    bool RemoveChildren(const bool force = false);

protected:
    virtual bool DebugMenuDrawn() = 0;
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

    int mId;
    std::string mName;
    std::unordered_map<std::string, std::shared_ptr<Component>> mParents;
    std::unordered_map<std::string, std::shared_ptr<Component>> mChildren;
#ifdef INCLUDE_MUTEX
    std::mutex mMutex;
#endif
};

} // namespace Ship
