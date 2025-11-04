#include "ship/Component.h"

#include "spdlog/spdlog.h"


namespace Ship {
std::atomic_int Component::NextComponentId = 0;

Component::Component(const std::string& name, bool isUpdating, bool isDrawing, bool isUpdatingChildren,
                     bool isDrawingChildren)
    : mId(NextComponentId++), mName(name), mIsUpdating(isUpdating), mIsDrawing(isDrawing), mIsUpdatingChildren(isUpdatingChildren),
      mIsDrawingChildren(isDrawingChildren), mMutex(),
      mParentsToAdd(), mChildrenToAdd(), mParentsToRemove(), mChildrenToRemove(), mParents(), mChildren(),
      mUpdateStartClock(std::chrono::high_resolution_clock::now()),
      mDrawStartClock(std::chrono::high_resolution_clock::now()),
      mUpdateEndClock(std::chrono::high_resolution_clock::now()),
      mDrawEndClock(std::chrono::high_resolution_clock::now()),
      mUpdateFullEndClock(std::chrono::high_resolution_clock::now()),
      mDrawFullEndClock(std::chrono::high_resolution_clock::now()),
      mPreviousUpdateStartClock(std::chrono::high_resolution_clock::now()),
      mPreviousDrawStartClock(std::chrono::high_resolution_clock::now()),
      mPreviousUpdateEndClock(std::chrono::high_resolution_clock::now()),
      mPreviousDrawEndClock(std::chrono::high_resolution_clock::now()),
      mPreviousUpdateFullEndClock(std::chrono::high_resolution_clock::now()),
      mPreviousDrawFullEndClock(std::chrono::high_resolution_clock::now()) {
    SPDLOG_INFO("Constructing component {}", static_cast<std::string>(*this));
}

Component::~Component() {
    SPDLOG_INFO("Destructing component {}", static_cast<std::string>(*this));
}

bool Component::Update() {
    // There is NO protection for cyclical references. If this happens, you will crash via recursion.

    // Set Previous clocks to the current clock.
    SetPreviousUpdateStartClock(GetUpdateStartClock());
    SetPreviousUpdateEndClock(GetUpdateEndClock());
    SetPreviousUpdateFullEndClock(GetUpdateFullEndClock());

    // Set Start clock to now, and end clocks to "empty"
    // Then run the logic. After the logic, set the end time.
    // Then run the child logic. After that, set the full end time.
    SetUpdateStartClock(std::chrono::high_resolution_clock::now());
    SetUpdateEndClock(std::chrono::time_point<std::chrono::steady_clock>(std::chrono::system_clock::duration::zero()));
    SetUpdateFullEndClock(GetUpdateEndClock());
    bool result = true;
    if (IsUpdating()) {
        result &= Updated(GetDurationSinceLastTick());
    }
    SetUpdateEndClock(std::chrono::high_resolution_clock::now());
    if (IsUpdatingChildren()) {
        mMutex.lock();
        const auto childrenCopy = mChildren;
        mMutex.unlock();
        for (const auto& pair : childrenCopy) {
            auto child = pair.second;
            if (child != nullptr) {
                result &= child->Draw();
            }
        }
    }
    SetUpdateFullEndClock(std::chrono::high_resolution_clock::now());

    return result;
}

bool Component::Draw() {
    // There is NO protection for cyclical references. If this happens, you will crash via recursion.

    // Set Previous clocks to the current clock.
    SetPreviousDrawStartClock(GetDrawStartClock());
    SetPreviousDrawEndClock(GetDrawEndClock());
    SetPreviousDrawFullEndClock(GetDrawFullEndClock());

    // Set Start clock to now, and end clocks to "empty"
    // Then run the logic. After the logic, set the end time.
    // Then run the child logic. After that, set the full end time.
    SetDrawStartClock(std::chrono::high_resolution_clock::now());
    SetDrawEndClock(std::chrono::time_point<std::chrono::steady_clock>(std::chrono::system_clock::duration::zero()));
    bool result = true;
    SetDrawFullEndClock(GetDrawEndClock());
    if (IsDrawing()) {
        result &= Drawn(GetDurationSinceLastTick());
    }
    SetDrawEndClock(std::chrono::high_resolution_clock::now());
    if (IsDrawingChildren()) {
        mMutex.lock();
        const auto childrenCopy = mChildren;
        mMutex.unlock();
        for (const auto& pair : childrenCopy) {
            auto child = pair.second;
            if (child != nullptr) {
                result &= child->Draw();
            }
        }
    }
    SetDrawFullEndClock(std::chrono::high_resolution_clock::now());

    return result;
}

bool Component::DrawDebugMenu() {
    // TODO: Not implemented.
    // The idea is that we display generic properties of the Component, then call the overridable method which will draw the specific stuff.
    // Then we create a collapsable box to show all children and recursively call DrawDebugMenu
    return true;
}

int Component::GetId() const {
    return mId;
}

std::string Component::GetName() const {
    return mName;
}

std::string Component::ToString() const {
    return std::to_string(GetId()) + "-" + GetName() + "-" + typeid(*this).name();
}

Component::operator std::string() const {
    return ToString();
}

bool Component::IsUpdating() const {
    return mIsUpdating;
}

bool Component::IsDrawing() const {
    return mIsDrawing;
}

bool Component::IsUpdatingChildren() const {
    return mIsUpdatingChildren;
}

bool Component::IsDrawingChildren() const {
    return mIsDrawingChildren;
}

bool Component::StartUpdating(bool force) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (IsUpdating()) {
        return true;
    }

    bool success = UpdatingStarted(force);
    if (force || success) {
        mIsUpdating = true;

        if (force && !success) {
            SPDLOG_WARN("Forcing Update Start on Component {}", static_cast<std::string>(*this));
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

bool Component::StartDrawing(bool force) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (IsDrawing()) {
        return true;
    }

    bool success = DrawingStarted(force);
    if (force || success) {
        mIsDrawing = true;

        if (force && !success) {
            SPDLOG_WARN("Forcing Draw Start on Component {}", static_cast<std::string>(*this));
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

bool Component::StopUpdating(bool force) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (!IsUpdating()) {
        return true;
    }

    bool success = UpdatingStopped(force);
    if (force || success) {
        mIsUpdating = false;

        if (force && !success) {
            SPDLOG_WARN("Forcing Update Stop on Component {}", static_cast<std::string>(*this));
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

bool Component::StopDrawing(bool force) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (!IsDrawing()) {
        return true;
    }

    bool success = DrawingStopped(force);
    if (force || success) {
        mIsDrawing = false;

        if (force && !success) {
            SPDLOG_WARN("Forcing Draw Stop on Component {}", static_cast<std::string>(*this));
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

bool Component::StartUpdatingChildren(bool force) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (IsUpdatingChildren()) {
        return true;
    }

    bool success = UpdatingChildrenStarted(force);
    if (force || success) {
        mIsUpdatingChildren = true;

        if (force && !success) {
            SPDLOG_WARN("Forcing Update Children Start on Component {}", static_cast<std::string>(*this));
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

bool Component::StartDrawingChildren(bool force) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (IsDrawingChildren()) {
        return true;
    }

    bool success = DrawingChildrenStarted(force);
    if (force || success) {
        mIsDrawingChildren = true;

        if (force && !success) {
            SPDLOG_WARN("Forcing Draw Children Start on Component {}", static_cast<std::string>(*this));
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

bool Component::StopUpdatingChildren(bool force) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (!IsUpdatingChildren()) {
        return true;
    }

    bool success = UpdatingChildrenStopped(force);
    if (force || success) {
        mIsUpdatingChildren = false;

        if (force && !success) {
            SPDLOG_WARN("Forcing Update Children Stop on Component {}", static_cast<std::string>(*this));
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

bool Component::StopDrawingChildren(bool force) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (!IsDrawingChildren()) {
        return true;
    }

    bool success = DrawingChildrenStopped(force);
    if (force || success) {
        mIsDrawingChildren = false;

        if (force && !success) {
            SPDLOG_WARN("Forcing Draw Children Stop on Component {}", static_cast<std::string>(*this));
        } else if (!force && !success) {
            return false;
        }
    }

    return true;
}

bool Component::StartUpdatingAll(bool force) {
    return StartUpdating(force) & StartUpdatingChildren(force);
}

bool Component::StartDrawingAll(bool force) {
    return StartDrawing(force) & StartDrawingChildren(force);
}

bool Component::StopUpdatingAll(bool force) {
    return StopUpdating(force) & StopUpdatingChildren(force);
}

bool Component::StopDrawingAll(bool force) {
    return StopDrawing(force) & StopDrawingChildren(force);
}

std::shared_ptr<Component> Component::GetParent(const std::string& parent) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (HasParent(parent)) {
        return mParents[parent];
    }
    return nullptr;
}

std::shared_ptr<Component> Component::GetChild(const std::string& child) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    if (HasChild(child)) {
        return mChildren[child];
    }
    return nullptr;
}

std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Component>>> Component::GetParents() {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return std::make_shared<std::unordered_map<std::string, std::shared_ptr<Component>>>(mParents);
}

std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Component>>> Component::GetChildren() {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return std::make_shared<std::unordered_map<std::string, std::shared_ptr<Component>>>(mChildren);
}

bool Component::HasParent(std::shared_ptr<Component> parent) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return HasParent(parent->GetName());
}

bool Component::HasParent(const std::string& parent) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mParents.contains(parent);
}

bool Component::HasParent() {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return !mParents.empty();
}

bool Component::HasChild(std::shared_ptr<Component> child) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return HasChild(child->GetName());
}
bool Component::HasChild(const std::string& child) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mChildren.contains(child);
}

bool Component::HasChild() {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    return !mChildren.empty();
}

Component& Component::AddParent(std::shared_ptr<Component> parent, bool now) {
    if (parent == nullptr) {
        return *this;
    }

    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasParent(parent->GetName())) {
        return *this;
    }

    if (now) {
        AddParentRaw(parent);
    } else {
        if (mParentsToAdd.contains(parent->GetName())) {
            return *this;
        }
        mParentsToAdd[parent->GetName()] = parent;
    }

    parent->AddChild(shared_from_this(), now);

    return *this;
}

Component& Component::AddChild(std::shared_ptr<Component> child, bool now) {
    if (child == nullptr) {
        return *this;
    }

    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasChild(child->GetName())) {
        return *this;
    }

    if (now) {
        AddChildRaw(child);
    } else {
        if (mChildrenToAdd.contains(child->GetName())) {
            return *this;
        }
        mChildrenToAdd[child->GetName()] = child;
    }

    child->AddParent(shared_from_this(), now);

    return *this;
}

Component& Component::RemoveParent(std::shared_ptr<Component> parent, bool now) {
    if (parent == nullptr) {
        return *this;
    }

    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasChild(parent->GetName())) {
        return *this;
    }

    if (now) {
        RemoveParentRaw(parent);
    } else {
        if (mParentsToRemove.contains(parent->GetName())) {
            return *this;
        }
        mParentsToRemove[parent->GetName()] = parent;
    }

    parent->RemoveChild(shared_from_this(), now);

    return *this;
}
Component& Component::RemoveChild(std::shared_ptr<Component> child, bool now) {
    if (child == nullptr) {
        return *this;
    }

    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasParent(child->GetName())) {
        return *this;
    }

    if (now) {
        RemoveChildRaw(child);
    } else {
        if (mChildrenToRemove.contains(child->GetName())) {
            return *this;
        }
        mParentsToRemove[child->GetName()] = child;
    }

    child->RemoveParent(shared_from_this(), now);

    return *this;
}

Component& Component::RemoveParent(const std::string& parent, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasParent(parent)) {
        return *this;
    }
    return RemoveParent(mParents[parent]);
}

Component& Component::RemoveChild(const std::string& child, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (HasChild(child)) {
        return *this;
    }
    return RemoveChild(mChildren[child]);
}

Component& Component::AddParents(const std::unordered_map<std::string, std::shared_ptr<Component>>& parents, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : parents) {
        AddParent(pair.second, now);
    }

    return *this;
}

Component& Component::AddChildren(const std::unordered_map<std::string, std::shared_ptr<Component>>& children,
                                  bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : children) {
        AddChild(pair.second, now);
    }

    return *this;
}

Component& Component::AddParents(const std::vector<std::shared_ptr<Component>>& parents, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& parent : parents) {
        AddParent(parent, now);
    }

    return *this;
}

Component& Component::AddChildren(const std::vector<std::shared_ptr<Component>>& children, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& child : children) {
        AddChild(child, now);
    }

    return *this;
}

Component& Component::RemoveParents(const std::unordered_map<std::string, std::shared_ptr<Component>>& parents,
                                    bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : parents) {
        RemoveParent(pair.second, now);
    }

    return *this;
}

Component& Component::RemoveChildren(const std::unordered_map<std::string, std::shared_ptr<Component>>& children,
                                     bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : children) {
        RemoveChild(pair.second, now);
    }

    return *this;
}

Component& Component::RemoveParents(const std::vector<std::shared_ptr<Component>>& parents, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& parent : parents) {
        RemoveParent(parent, now);
    }

    return *this;
}

Component& Component::RemoveChildren(const std::vector<std::shared_ptr<Component>>& children, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& child : children) {
        RemoveChild(child, now);
    }

    return *this;
}

Component& Component::RemoveParents(const std::vector<std::string>& parents, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& parent : parents) {
        RemoveParent(parent, now);
    }

    return *this;
}

Component& Component::RemoveChildren(const std::vector<std::string>& children, bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& child : children) {
        RemoveChild(child, now);
    }

    return *this;
}

Component& Component::RemoveParents(bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : mParents) {
        RemoveParent(pair.second, now);
    }

    return *this;
}

Component& Component::RemoveChildren(bool now) {
    const std::lock_guard<std::recursive_mutex> lock(mMutex);

    for (const auto& pair : mParents) {
        RemoveParent(pair.second, now);
    }

    return *this;
}

double Component::GetUpdateStartTime() {
    return std::chrono::duration<double>(GetUpdateStartClock().time_since_epoch()).count();
}

double Component::GetDrawStartTime() {
    return std::chrono::duration<double>(GetDrawStartClock().time_since_epoch()).count();
}

double Component::GetUpdateEndTime() {
    return std::chrono::duration<double>(GetUpdateEndClock().time_since_epoch()).count();
}

double Component::GetDrawEndTime() {
    return std::chrono::duration<double>(GetDrawEndClock().time_since_epoch()).count();
}

double Component::GetUpdateFullEndTime() {
    return std::chrono::duration<double>(GetUpdateFullEndClock().time_since_epoch()).count();
}

double Component::GetDrawFullEndTime() {
    return std::chrono::duration<double>(GetDrawFullEndClock().time_since_epoch()).count();
}

double Component::GetPreviousUpdateStartTime() {
    return std::chrono::duration<double>(GetPreviousUpdateStartClock().time_since_epoch()).count();
}

double Component::GetPreviousDrawStartTime() {
    return std::chrono::duration<double>(GetPreviousDrawStartClock().time_since_epoch()).count();
}

double Component::GetPreviousUpdateEndTime() {
    return std::chrono::duration<double>(GetPreviousUpdateEndClock().time_since_epoch()).count();
}
double Component::GetPreviousDrawEndTime() {
    return std::chrono::duration<double>(GetPreviousDrawEndClock().time_since_epoch()).count();
}

double Component::GetPreviousUpdateFullEndTime() {
    return std::chrono::duration<double>(GetPreviousUpdateFullEndClock().time_since_epoch()).count();
}
double Component::GetPreviousDrawFullEndTime() {
    return std::chrono::duration<double>(GetPreviousDrawFullEndClock().time_since_epoch()).count();
}

double Component::GetDurationSinceLastTick() {
    return std::chrono::duration<double>(GetUpdateStartClock() - GetPreviousUpdateStartClock()).count();
}

double Component::GetPreviousUpdateDuration() {
    return std::chrono::duration<double>(GetPreviousUpdateStartClock() - GetPreviousUpdateEndClock()).count();
}

double Component::GetPreviousDrawDuration() {
    return std::chrono::duration<double>(GetPreviousDrawStartClock() - GetPreviousDrawEndClock()).count();
}

double Component::GetPreviousUpdateFullDuration() {
    return std::chrono::duration<double>(GetPreviousUpdateStartClock() - GetPreviousUpdateFullEndClock()).count();
}

double Component::GetPreviousDrawFullDuration() {
    return std::chrono::duration<double>(GetPreviousDrawStartClock() - GetPreviousDrawFullEndClock()).count();
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetUpdateStartClock() {
    return mUpdateStartClock;
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetDrawStartClock() {
    return mDrawStartClock;
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetUpdateEndClock() {
    return mUpdateEndClock;
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetDrawEndClock() {
    return mDrawEndClock;
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetUpdateFullEndClock() {
    return mUpdateFullEndClock;
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetDrawFullEndClock() {
    return mDrawFullEndClock;
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetPreviousUpdateStartClock() {
    return mPreviousUpdateStartClock;
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetPreviousDrawStartClock() {
    return mPreviousDrawStartClock;
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetPreviousUpdateEndClock() {
    return mPreviousUpdateEndClock;
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetPreviousDrawEndClock() {
    return mPreviousDrawEndClock;
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetPreviousUpdateFullEndClock() {
    return mPreviousUpdateFullEndClock;
}

std::chrono::time_point<std::chrono::steady_clock> Component::GetPreviousDrawFullEndClock() {
    return mPreviousDrawFullEndClock;
}

Component& Component::AddParentRaw(std::shared_ptr<Component> parent) {
    SPDLOG_INFO("Adding parent {} to this component {}", static_cast<std::string>(*parent), static_cast<std::string>(*this));
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    mParents[parent->GetName()] = parent;
    return *this;
}

Component& Component::AddChildRaw(std::shared_ptr<Component> child) {
    SPDLOG_INFO("Adding parent {} to this component {}", static_cast<std::string>(*child),
                static_cast<std::string>(*this));
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    mChildren[child->GetName()] = child;
    return *this;
}

Component& Component::RemoveParentRaw(std::shared_ptr<Component> parent) {
    SPDLOG_INFO("Removing parent {} from this component {}", static_cast<std::string>(*parent),
                static_cast<std::string>(*this));

    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    mParents.erase(parent->GetName());
    return *this;
}

Component& Component::RemoveChildRaw(std::shared_ptr<Component> child) {
    SPDLOG_INFO("Adding child {} from this component {}", static_cast<std::string>(*child),
                static_cast<std::string>(*this));
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    mChildren.erase(child->GetName());
    return *this;
}

Component& Component::SetUpdateStartClock(std::chrono::time_point<std::chrono::steady_clock> updateStartClock) {
    mUpdateStartClock = updateStartClock;
    return *this;
}

Component& Component::SetDrawStartClock(std::chrono::time_point<std::chrono::steady_clock> drawStartClock) {
    mDrawStartClock = drawStartClock;
    return *this;
}

Component& Component::SetUpdateEndClock(std::chrono::time_point<std::chrono::steady_clock> updateEndClock) {
    mUpdateEndClock = updateEndClock;
    return *this;
}

Component& Component::SetDrawEndClock(std::chrono::time_point<std::chrono::steady_clock> drawEndClock) {
    mDrawEndClock = drawEndClock;
    return *this;
}

Component& Component::SetUpdateFullEndClock(std::chrono::time_point<std::chrono::steady_clock> updateFullEndClock) {
    mUpdateFullEndClock = updateFullEndClock;
    return *this;
}

Component& Component::SetDrawFullEndClock(std::chrono::time_point<std::chrono::steady_clock> drawFullEndClock) {
    mDrawFullEndClock = drawFullEndClock;
    return *this;
}

Component&
Component::SetPreviousUpdateStartClock(std::chrono::time_point<std::chrono::steady_clock> previousUpdateStartClock) {
    mPreviousUpdateStartClock = previousUpdateStartClock;
    return *this;
}

Component&
Component::SetPreviousDrawStartClock(std::chrono::time_point<std::chrono::steady_clock> previousDrawStartClock) {
    mPreviousDrawStartClock = previousDrawStartClock;
    return *this;
}

Component&
Component::SetPreviousUpdateEndClock(std::chrono::time_point<std::chrono::steady_clock> previousUpdateEndClock) {
    mPreviousUpdateEndClock = previousUpdateEndClock;
    return *this;
}

Component&
Component::SetPreviousDrawEndClock(std::chrono::time_point<std::chrono::steady_clock> previousUpdateEndClock) {
    mPreviousDrawEndClock = previousUpdateEndClock;
    return *this;
}

Component&
Component::SetPreviousUpdateFullEndClock(std::chrono::time_point<std::chrono::steady_clock> previousUpdateFullEndClock) {
    mPreviousUpdateFullEndClock = previousUpdateFullEndClock;
    return *this;
}

Component&
Component::SetPreviousDrawFullEndClock(std::chrono::time_point<std::chrono::steady_clock> previousDrawFullEndClock) {
    mPreviousDrawFullEndClock = previousDrawFullEndClock;
    return *this;
}
} // namespace Ship
