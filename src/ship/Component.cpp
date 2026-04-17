#include "ship/Component.h"

#ifdef COMPONENT_THREAD_SAFE
#include <shared_mutex>
#include <mutex>
#endif

#include <spdlog/spdlog.h>
#include <algorithm>

namespace Ship {

Component::Component(const std::string& name)
    : Part(), mName(name), mParents(), mChildren()
#ifdef COMPONENT_THREAD_SAFE
      ,
      mMutex()
#endif
{
    SPDLOG_INFO("Constructing component {}", ToString());
}

Component::~Component() {
    SPDLOG_INFO("Destructing component {}", ToString());
}

const std::string& Component::GetName() const {
    return mName;
}

std::string Component::ToString() const {
    return std::to_string(GetId()) + "-" + GetName() + "-" + typeid(*this).name();
}

Component::operator std::string() const {
    return ToString();
}

#ifdef COMPONENT_THREAD_SAFE
std::shared_mutex& Component::GetMutex() const {
    return mMutex;
}
#endif

// ---- Get ----

PartList<Component>& Component::GetParents() {
    return mParents;
}

const PartList<Component>& Component::GetParents() const {
    return mParents;
}

PartList<Component>& Component::GetChildren() {
    return mChildren;
}

const PartList<Component>& Component::GetChildren() const {
    return mChildren;
}

} // namespace Ship
