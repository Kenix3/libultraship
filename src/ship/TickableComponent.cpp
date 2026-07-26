#include "ship/TickableComponent.h"
#include "ship/Context.h"

#include "ship/actions/EventAction.h"

#include <spdlog/spdlog.h>

namespace Ship {

TickableComponent::TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                                     const TickGroup tickGroup, const TickPriority tickPriority,
                                     const std::vector<EventID>& eventIds)
    : Tickable(false), Component(name, context), mTickGroup(tickGroup), mTickPriority(tickPriority),
      mPendingEventIds(eventIds) {
    // Note: Actions and context registration are deferred to RegisterWithContext()
    // because shared_from_this() cannot be called in a constructor.
}

TickableComponent::TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                                     const TickGroup tickGroup, const TickPriority tickPriority,
                                     const std::vector<std::shared_ptr<Action>>& actions)
    : Tickable(false), Component(name, context), mTickGroup(tickGroup), mTickPriority(tickPriority),
      mPendingActions(actions) {
    // Note: Actions and context registration are deferred to RegisterWithContext()
    // because shared_from_this() cannot be called in a constructor.
}

TickableComponent::~TickableComponent() {
    // shared_from_this() is unsafe in a destructor. If the component is still
    // registered with its Context at this point, that is a usage error — callers
    // should have called UnregisterFromContext() before allowing destruction.
    // We emit a warning in debug builds to catch this early.
#ifdef _DEBUG
    if (GetContext() != nullptr) {
        SPDLOG_WARN("TickableComponent '{}' destroyed while still potentially registered with Context. "
                    "Call UnregisterFromContext() before releasing the last shared_ptr.",
                    GetName());
    }
#endif
}

bool TickableComponent::RegisterWithContext() {
    auto self = std::dynamic_pointer_cast<TickableComponent>(TryGetSharedComponent());
    if (!self) {
        SPDLOG_WARN("RegisterWithContext failed for {}: shared self unavailable", ToString());
        return false;
    }

    // Register pending EventActions.
    for (const auto& eventId : mPendingEventIds) {
        GetActionList().Add(std::make_shared<EventAction>(eventId, self));
    }

    // Register explicitly provided Actions.
    for (const auto& action : mPendingActions) {
        GetActionList().Add(action);
    }

    if (!mPendingEventIds.empty() || !mPendingActions.empty()) {
        Start();
    }

    if (GetContext() != nullptr) {
        GetContext()->GetTickableComponents().Add(self);
    }

    return true;
}

void TickableComponent::UnregisterFromContext() {
    auto self = std::dynamic_pointer_cast<TickableComponent>(TryGetSharedComponent());
    if (GetContext() != nullptr && self) {
        GetContext()->GetTickableComponents().Remove(self);
    }
}

std::shared_ptr<Context> TickableComponent::GetContext() const {
    return Part::GetContext();
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
    auto self = std::dynamic_pointer_cast<TickableComponent>(TryGetSharedComponent());
    const auto& oldContext = GetContext();
    if (oldContext != nullptr && self) {
        oldContext->GetTickableComponents().Remove(self);
    }
    Part::SetContext(context);
    if (context != nullptr && self) {
        context->GetTickableComponents().Add(self);
    }
    return *this;
}

bool TickableComponent::ActionRan(EventID eventId, const double durationSinceLastTick) {
    return true;
}

} // namespace Ship
