// Adds functions to search components by name

//bool AddPart(std::shared_ptr<C> parent, const bool force = false);
//bool AddParts(const std::vector<std::shared_ptr<C>>& parts, const bool force = false);
//bool RemovePart(std::shared_ptr<C> parent, const bool force = false);
//bool RemovePart(const uint64_t id, bool force = false);
//bool RemoveParts(const bool force = false);
//bool RemoveParts(const std::vector<std::shared_ptr<C>>& parts, const bool force = false);
//template <typename T> bool RemoveParts(const bool force = false);
//bool RemoveParts(const std::vector<uint64_t>& parents, const bool force = false);

/*
bool Component::AddParent(std::shared_ptr<Component> parent, const bool force) {
    if (parent == nullptr) {
        return false;
    }

    const auto ptr = shared_from_this();
    const bool canAddParent = CanAddParent(parent);
    const bool canAddChild = parent->CanAddChild(ptr);
    const bool forced = (!canAddParent || !canAddChild) && force;
    {
#ifdef INCLUDE_MUTEX
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif

        if (HasParent(parent)) {
            return true;
        }

        if ((!canAddParent || !canAddChild) && !force) {
            return false;
        }

        AddParentRaw(parent);
    }
    AddedParent(parent, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be added as a parent to {}", parent->ToString(), ptr->ToString());
    }
    parent->AddChild(ptr, force);

    return true;
}

bool Component::AddChild(std::shared_ptr<Component> child, const bool force) {
    if (child == nullptr) {
        return false;
    }

    const auto ptr = shared_from_this();
    const bool canAddChild = CanAddChild(child);
    const bool canAddParent = child->CanAddParent(ptr);
    const bool forced = (!canAddParent || !canAddChild) && force;
    {
#ifdef INCLUDE_MUTEX
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif

        if (HasChild(child)) {
            return true;
        }

        if ((!canAddParent || !canAddChild) && !force) {
            return false;
        }

        AddChildRaw(child);
    }
    AddedChild(child, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be added as a child to {}", child->ToString(), ptr->ToString());
    }
    child->AddParent(ptr, force);

    return true;
}

bool Component::RemoveParent(std::shared_ptr<Component> parent, const bool force) {
    if (parent == nullptr) {
        return false;
    }

    const auto ptr = shared_from_this();
    const bool canRemoveParent = CanRemoveParent(parent);
    const bool canRemoveChild = parent->CanRemoveChild(ptr);
    const bool forced = (!canRemoveParent || !canRemoveChild) && force;
    {
#ifdef INCLUDE_MUTEX
        // This use of recursive mutex allows us to make sure duplicates are never added to the parent list
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif

        if (!HasParent(parent)) {
            return true;
        }

        if ((!canRemoveParent || !canRemoveChild) && !force) {
            return false;
        }

        RemoveParentRaw(parent);
    }
    RemovedParent(parent, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be removed as a parent to {}", parent->ToString(), ptr->ToString());
    }
    parent->RemoveChild(ptr, force);

    return true;
}

bool Component::RemoveChild(std::shared_ptr<Component> child, const bool force) {
    if (child == nullptr) {
        return false;
    }

    const auto ptr = shared_from_this();
    const bool canRemoveChild = CanRemoveChild(child);
    const bool canRemoveParent = child->CanRemoveParent(ptr);
    const bool forced = (!canRemoveParent || !canRemoveChild) && force;
    {
#ifdef INCLUDE_MUTEX
        // This use of recursive mutex allows us to make sure duplicates are never added to the children list
        const std::lock_guard<std::recursive_mutex> lock(mMutex);
#endif

        if (!HasChild(child)) {
            return true;
        }

        if ((!canRemoveParent || !canRemoveChild) && !force) {
            return false;
        }

        RemoveChildRaw(child);
    }
    RemovedChild(child, forced);
    if (forced) {
        SPDLOG_WARN("Forcing {} to be removed as a child to {}", child->ToString(), ptr->ToString());
    }
    child->RemoveParent(ptr, force);

    return true;
}
*/

