#include "ship/TickableComponent.h"
#include "ship/Context.h"
#include "ship/actions/EventAction.h"

#include <spdlog/spdlog.h>

namespace Ship {

TickableComponent::TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                                     const TickGroup tickGroup, const TickPriority tickPriority,
                                     const std::vector<EventID>& eventIds)
    : Tickable(false), Component(name), mTickGroup(tickGroup), mTickPriority(tickPriority), mContext(context),
      mPendingEventIds(eventIds) {
    // Note: Actions and context registration are deferred to RegisterWithContext()
    // because shared_from_this() cannot be called in a constructor.
}

TickableComponent::TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                                     const TickGroup tickGroup, const TickPriority tickPriority,
                                     const std::vector<std::shared_ptr<Action>>& actions)
    : Tickable(true, actions), Component(name), mTickGroup(tickGroup), mTickPriority(tickPriority), mContext(context) {
}

TickableComponent::~TickableComponent() {
    // Do not call shared_from_this() in destructor - it is unsafe.
    // UnregisterFromContext() should be called explicitly before destruction,
    // or the Context should clean up its own references.
}

void TickableComponent::InitWeakSelf(std::shared_ptr<TickableComponent> self) {
    if (mWeakSelf.expired()) {
        mWeakSelf = self;
    }
}

void TickableComponent::RegisterWithContext(std::shared_ptr<TickableComponent> self) {
    InitWeakSelf(self);

    // Create EventActions for all pending EventIDs.
    for (const auto& eventId : mPendingEventIds) {
        GetActionList().Add(std::make_shared<EventAction>(eventId, self));
    }

    if (!mPendingEventIds.empty()) {
        Start();
    }

    if (GetContext() != nullptr) {
        GetContext()->GetTickableComponents().Add(self);
    }
}

void TickableComponent::UnregisterFromContext() {
    auto self = std::static_pointer_cast<TickableComponent>(mWeakSelf.lock());
    if (GetContext() != nullptr && self) {
        GetContext()->GetTickableComponents().Remove(self);
    }
}

std::shared_ptr<Context> TickableComponent::GetContext() const {
    return mContext;
}

TickGroup TickableComponent::GetTickGroup() const {
    return mTickGroup;
}

TickPriority TickableComponent::GetTickPriority() const {
    return mTickPriority;
}

uint64_t TickableComponent::GetOrder() const {
    return (static_cast<uint64_t>(mTickGroup) << 32) | static_cast<uint64_t>(mTickPriority);
}

TickableComponent& TickableComponent::SetTickGroup(const TickGroup tickGroup) {
    mTickGroup = tickGroup;
    return *this;
}

TickableComponent& TickableComponent::SetTickPriority(const TickPriority tickPriority) {
    mTickPriority = tickPriority;
    return *this;
}

TickableComponent& TickableComponent::SetContext(std::shared_ptr<Context> context) {
    auto self = std::static_pointer_cast<TickableComponent>(mWeakSelf.lock());
    const auto& oldContext = GetContext();
    if (oldContext != nullptr && self) {
        oldContext->GetTickableComponents().Remove(self);
    }
    mContext = context;
    if (context != nullptr && self) {
        context->GetTickableComponents().Add(self);
    }
    return *this;
}

bool TickableComponent::ActionRan(EventID eventId, const double durationSinceLastTick) {
    return true;
}

std::shared_ptr<Component> TickableComponent::GetSharedComponent() {
    return std::static_pointer_cast<Component>(mWeakSelf.lock());
}

} // namespace Ship
