#include "ship/TickableComponent.h"

#include <spdlog/spdlog.h>

namespace Ship {
TickableComponent::TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                                     const TickGroup tickGroup, const TickPriority tickPriority, const bool isTicking,
                                     const bool isDrawing, const bool isDrawingDebugMenu)
    : Tickable(isTicking, isDrawing), Component(name), mTickGroup(tickGroup), mTickPriority(tickPriority), mContext(context) {
    if (isDrawingDebugMenu) {
        StartAction(ActionType::DrawDebugMenu);
    }

    if (GetContext() != nullptr) {
        GetContext()->AddTickableComponent(std::static_pointer_cast<TickableComponent>(shared_from_this()));
    }
}

TickableComponent::TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                                     const TickGroup tickGroup, const TickPriority tickPriority,
                                     const std::vector<bool>& actions)
    : Tickable(actions), Component(name), mTickGroup(tickGroup), mTickPriority(tickPriority), mContext(context) {
    if (GetContext() != nullptr) {
        GetContext()->AddTickableComponent(std::static_pointer_cast<TickableComponent>(shared_from_this()));
    }
}

TickableComponent::~TickableComponent() {
    if (GetContext() != nullptr) {
        GetContext()->RemoveTickableComponent(std::static_pointer_cast<TickableComponent>(shared_from_this()));
    }

    Component::~Component();
}

bool TickableComponent::ActionRan(const ActionType action, const double durationSinceLastTick) {
    bool val = Tickable::ActionRan(action, durationSinceLastTick);
    if (action == ActionType::DrawDebugMenu) {
        val &= DebugMenuDrawn(durationSinceLastTick);
        auto children = GetChildren();
        for (const auto& child : *children) {
            const auto& childTickable = std::dynamic_pointer_cast<Tickable>(child); 
            val &= childTickable->ActionRan(action, durationSinceLastTick);
        }
    }
    return val;
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
    return (((uint64_t)mTickGroup) << 32) | ((uint64_t)mTickPriority);
}

TickableComponent& TickableComponent::SetTickGroup(const TickGroup tickGroup) {
    mTickGroup = tickGroup;
    const auto& context = GetContext();
    if (context != nullptr) {
        context->SetTickableComponentsOrderStale();
    }
    return *this;
}

TickableComponent& TickableComponent::SetTickPriority(const TickPriority tickPriority) {
    mTickPriority = tickPriority;
    const auto& context = GetContext();
    if (context != nullptr) {
        context->SetTickableComponentsOrderStale();
    }
    return *this;
}

TickableComponent& TickableComponent::SetContext(std::shared_ptr<Context> context) {
    const auto& oldContext = GetContext();
    if (oldContext != nullptr) {
        oldContext->SetTickableComponentsOrderStale();
    }
    mContext = context;
    if (context != nullptr) {
        context->SetTickableComponentsOrderStale();
    }
    return *this;
}

} // namespace Ship
