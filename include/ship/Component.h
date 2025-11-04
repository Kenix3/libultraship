#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <atomic>

namespace Ship {
class Component : private std::enable_shared_from_this<Component> {
  public:
    Component(const std::string& name, bool isUpdating = true, bool isDrawing = true, bool isUpdatingChildren = true,
              bool isDrawingChildren = true);
    ~Component();

    bool Update();
    bool Draw();
    bool DrawDebugMenu();

    int GetId() const;
    std::string GetName() const;
    std::string ToString() const;
    explicit operator std::string() const;

    bool IsUpdating() const;
    bool IsDrawing() const;
    bool IsUpdatingChildren() const;
    bool IsDrawingChildren() const;
    bool StartUpdating(bool force = false);
    bool StartDrawing(bool force = false);
    bool StopUpdating(bool force = false);
    bool StopDrawing(bool force = false);
    bool StartUpdatingChildren(bool force = false);
    bool StartDrawingChildren(bool force = false);
    bool StopUpdatingChildren(bool force = false);
    bool StopDrawingChildren(bool force = false);
    bool StartUpdatingAll(bool force = false);
    bool StartDrawingAll(bool force = false);
    bool StopUpdatingAll(bool force = false);
    bool StopDrawingAll(bool force = false);

    std::shared_ptr<Component> GetParent(const std::string& parent);
    std::shared_ptr<Component> GetChild(const std::string& child);
    std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Component>>> GetChildren();
    std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Component>>> GetParents();
    std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Component>>> GetChildren();
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

    double GetUpdateStartTime();
    double GetDrawStartTime();
    double GetUpdateEndTime();
    double GetDrawEndTime();
    double GetUpdateFullEndTime();
    double GetDrawFullEndTime();
    double GetPreviousUpdateStartTime();
    double GetPreviousDrawStartTime();
    double GetPreviousUpdateEndTime();
    double GetPreviousDrawEndTime();
    double GetPreviousUpdateFullEndTime();
    double GetPreviousDrawFullEndTime();
    double GetDurationSinceLastTick();
    double GetPreviousUpdateDuration();
    double GetPreviousDrawDuration();
    double GetPreviousUpdateFullDuration();
    double GetPreviousDrawFullDuration();

protected:
    std::chrono::time_point<std::chrono::steady_clock> GetUpdateStartClock();
    std::chrono::time_point<std::chrono::steady_clock> GetDrawStartClock();
    std::chrono::time_point<std::chrono::steady_clock> GetUpdateEndClock();
    std::chrono::time_point<std::chrono::steady_clock> GetDrawEndClock();
    std::chrono::time_point<std::chrono::steady_clock> GetUpdateFullEndClock();
    std::chrono::time_point<std::chrono::steady_clock> GetDrawFullEndClock();
    std::chrono::time_point<std::chrono::steady_clock> GetPreviousUpdateStartClock();
    std::chrono::time_point<std::chrono::steady_clock> GetPreviousDrawStartClock();
    std::chrono::time_point<std::chrono::steady_clock> GetPreviousUpdateEndClock();
    std::chrono::time_point<std::chrono::steady_clock> GetPreviousDrawEndClock();
    std::chrono::time_point<std::chrono::steady_clock> GetPreviousUpdateFullEndClock();
    std::chrono::time_point<std::chrono::steady_clock> GetPreviousDrawFullEndClock();

    virtual bool Updated(const double durationSinceLastUpdate) = 0;
    virtual bool Drawn(const double durationSinceLastUpdate) = 0;
    virtual bool DebugMenuDrawn(const double durationSinceLastUpdate) = 0;
    virtual bool UpdatingStarted(bool forced) = 0;
    virtual bool DrawingStarted(bool forced) = 0;
    virtual bool UpdatingStopped(bool forced) = 0;
    virtual bool DrawingStopped(bool forced) = 0;
    virtual bool UpdatingChildrenStarted(bool forced) = 0;
    virtual bool DrawingChildrenStarted(bool forced) = 0;
    virtual bool UpdatingChildrenStopped(bool forced) = 0;
    virtual bool DrawingChildrenStopped(bool forced) = 0;
    virtual bool AddedParent(std::shared_ptr<Component> parent) = 0;
    virtual bool AddedChild(std::shared_ptr<Component> child) = 0;
    virtual bool RemovedParent(std::shared_ptr<Component> parent) = 0;
    virtual bool RemovedChild(std::shared_ptr<Component> child) = 0;

private:
    Component& AddParentRaw(std::shared_ptr<Component> parent);
    Component& AddChildRaw(std::shared_ptr<Component> child);
    Component& RemoveParentRaw(std::shared_ptr<Component> parent);
    Component& RemoveChildRaw(std::shared_ptr<Component> child);

    Component& SetUpdateStartClock(std::chrono::time_point<std::chrono::steady_clock> updateStartClock);
    Component& SetDrawStartClock(std::chrono::time_point<std::chrono::steady_clock> drawStartClock);
    Component& SetUpdateEndClock(std::chrono::time_point<std::chrono::steady_clock> updateEndClock);
    Component& SetDrawEndClock(std::chrono::time_point<std::chrono::steady_clock> drawEndClock);
    Component& SetUpdateFullEndClock(std::chrono::time_point<std::chrono::steady_clock> updateFullEndClock);
    Component& SetDrawFullEndClock(std::chrono::time_point<std::chrono::steady_clock> drawFullEndClock);
    Component& SetPreviousUpdateStartClock(std::chrono::time_point<std::chrono::steady_clock> previousUpdateStartClock);
    Component& SetPreviousDrawStartClock(std::chrono::time_point<std::chrono::steady_clock> previousDrawStartClock);
    Component& SetPreviousUpdateEndClock(std::chrono::time_point<std::chrono::steady_clock> previousUpdateEndClock);
    Component& SetPreviousDrawEndClock(std::chrono::time_point<std::chrono::steady_clock> previousUpdateEndClock);
    Component& SetPreviousUpdateFullEndClock(std::chrono::time_point<std::chrono::steady_clock> previousUpdateFullEndClock);
    Component& SetPreviousDrawFullEndClock(std::chrono::time_point<std::chrono::steady_clock> previousUpdateFullEndClock);

    static std::atomic_int NextComponentId;

    int mId;
    std::string mName;

    bool mIsUpdating;
    bool mIsDrawing;
    bool mIsUpdatingChildren;
    bool mIsDrawingChildren;

    std::recursive_mutex mMutex;

    std::unordered_map<std::string, std::shared_ptr<Component>> mParentsToAdd;
    std::unordered_map<std::string, std::shared_ptr<Component>> mChildrenToAdd;
    std::unordered_map<std::string, std::shared_ptr<Component>> mParentsToRemove;
    std::unordered_map<std::string, std::shared_ptr<Component>> mChildrenToRemove;
    std::unordered_map<std::string, std::shared_ptr<Component>> mParents;
    std::unordered_map<std::string, std::shared_ptr<Component>> mChildren;

    std::chrono::time_point<std::chrono::steady_clock> mUpdateStartClock;
    std::chrono::time_point<std::chrono::steady_clock> mDrawStartClock;
    std::chrono::time_point<std::chrono::steady_clock> mUpdateEndClock;
    std::chrono::time_point<std::chrono::steady_clock> mDrawEndClock;
    std::chrono::time_point<std::chrono::steady_clock> mUpdateFullEndClock;
    std::chrono::time_point<std::chrono::steady_clock> mDrawFullEndClock;
    std::chrono::time_point<std::chrono::steady_clock> mPreviousUpdateStartClock;
    std::chrono::time_point<std::chrono::steady_clock> mPreviousDrawStartClock;
    std::chrono::time_point<std::chrono::steady_clock> mPreviousUpdateEndClock;
    std::chrono::time_point<std::chrono::steady_clock> mPreviousDrawEndClock;
    std::chrono::time_point<std::chrono::steady_clock> mPreviousUpdateFullEndClock;
    std::chrono::time_point<std::chrono::steady_clock> mPreviousDrawFullEndClock;
};

} // namespace Ship
