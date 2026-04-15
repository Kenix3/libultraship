#include "ship/TickableComponent.h"
#include "ship/Context.h"
#include "ship/actions/TickAction.h"
#include "ship/actions/DrawAction.h"
#include "ship/actions/DrawDebugMenuAction.h"

#include <spdlog/spdlog.h>

namespace Ship {

TickableComponent::TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                                     const TickGroup tickGroup, const TickPriority tickPriority, const bool isTicking,
                                     const bool isDrawing, const bool isDrawingDebugMenu)
    : Tickable(false), Component(name), mTickGroup(tickGroup), mTickPriority(tickPriority), mContext(context) {
    // Note: Actions and context registration are deferred to RegisterWithContext()
    // because shared_from_this() cannot be called in a constructor.
    mPendingTicking = isTicking;
    mPendingDrawing = isDrawing;
    mPendingDrawingDebugMenu = isDrawingDebugMenu;
}

TickableComponent::TickableComponent(const std::string& name, std::shared_ptr<Context> context,
                                     const TickGroup tickGroup, const TickPriority tickPriority,
                                     const std::vector<std::shared_ptr<Action>>& actions)
    : Tickable(true, actions), Component(name), mTickGroup(tickGroup), mTickPriority(tickPriority), mContext(context),
      mPendingTicking(false), mPendingDrawing(false), mPendingDrawingDebugMenu(false) {
}

TickableComponent::~TickableComponent() {
    // Do not call shared_from_this() in destructor - it is unsafe.
    // UnregisterFromContext() should be called explicitly before destruction,
    // or the Context should clean up its own references.
}

void TickableComponent::RegisterWithContext() {
    if (mPendingTicking) {
        AddAction(std::make_shared<TickAction>(Tickable::shared_from_this()));
    }
    if (mPendingDrawing) {
        AddAction(std::make_shared<DrawAction>(Tickable::shared_from_this()));
    }
    if (mPendingDrawingDebugMenu) {
        AddAction(std::make_shared<DrawDebugMenuAction>(Tickable::shared_from_this()));
    }
    if (mPendingTicking || mPendingDrawing || mPendingDrawingDebugMenu) {
        Start();
    }

    if (GetContext() != nullptr) {
        GetContext()->GetTickableComponents().Add(
            std::static_pointer_cast<TickableComponent>(Component::shared_from_this()));
    }
}

void TickableComponent::UnregisterFromContext() {
    if (GetContext() != nullptr) {
        GetContext()->GetTickableComponents().Remove(
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
    return *this;
}

TickableComponent& TickableComponent::SetTickPriority(const TickPriority tickPriority) {
    mTickPriority = tickPriority;
    return *this;
}

TickableComponent& TickableComponent::SetContext(std::shared_ptr<Context> context) {
    const auto& oldContext = GetContext();
    if (oldContext != nullptr) {
        oldContext->GetTickableComponents().Remove(
            std::static_pointer_cast<TickableComponent>(Component::shared_from_this()));
    }
    mContext = context;
    if (context != nullptr) {
        context->GetTickableComponents().Add(
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
