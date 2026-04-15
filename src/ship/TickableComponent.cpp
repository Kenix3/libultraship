#include "ship/TickableComponent.h"
#include "ship/Context.h"
#include "ship/actions/TickAction.h"
#include "ship/actions/DrawAction.h"
#include "ship/actions/DrawDebugMenuAction.h"

#include <spdlog/spdlog.h>

namespace Ship {

TickableComponent::TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                                     const TickGroup tickGroup, const TickPriority tickPriority,
                                     const bool isTicking, const bool isDrawing,
                                     const bool isDrawingDebugMenu)
    : Tickable(false), Component(name), mTickGroup(tickGroup), mTickPriority(tickPriority),
      mContext(context) {
    if (isTicking) {
        AddAction(std::make_shared<TickAction>(Tickable::shared_from_this()));
    }
    if (isDrawing) {
        AddAction(std::make_shared<DrawAction>(Tickable::shared_from_this()));
    }
    if (isDrawingDebugMenu) {
        AddAction(std::make_shared<DrawDebugMenuAction>(Tickable::shared_from_this()));
    }
    if (isTicking || isDrawing || isDrawingDebugMenu) {
        Start();
    }

    if (GetContext() != nullptr) {
        GetContext()->AddTickableComponent(
            std::static_pointer_cast<TickableComponent>(Component::shared_from_this()));
    }
}

TickableComponent::TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                                     const TickGroup tickGroup, const TickPriority tickPriority,
                                     const std::vector<std::shared_ptr<Action>>& actions)
    : Tickable(true, actions), Component(name), mTickGroup(tickGroup),
      mTickPriority(tickPriority), mContext(context) {
    if (GetContext() != nullptr) {
        GetContext()->AddTickableComponent(
            std::static_pointer_cast<TickableComponent>(Component::shared_from_this()));
    }
}

TickableComponent::~TickableComponent() {
    if (GetContext() != nullptr) {
        GetContext()->RemoveTickableComponent(
            std::static_pointer_cast<TickableComponent>(Component::shared_from_this()));
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
        oldContext->RemoveTickableComponent(
            std::static_pointer_cast<TickableComponent>(Component::shared_from_this()));
    }
    mContext = context;
    if (context != nullptr) {
        context->AddTickableComponent(
            std::static_pointer_cast<TickableComponent>(Component::shared_from_this()));
    }
    return *this;
}

bool TickableComponent::ActionRan(const uint32_t action, const double durationSinceLastTick) {
    if (action == static_cast<uint32_t>(ActionType::DrawDebugMenu)) {
        return DebugMenuDrawn(durationSinceLastTick);
    }
    return true;
}

bool TickableComponent::DebugMenuDrawn(const double durationSinceLastTick) {
    return true;
}

} // namespace Ship

