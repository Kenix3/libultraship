#include "ship/events/Events.h"
#include <stdexcept>
#include <algorithm>

#include "ship/events/CoreEvents.h"

namespace Ship {

EventRegistration* Events::GetOrCreateEventRegistration(const EventID id) {
    if (id == -1) {
        return nullptr;
    }

    if (mEventRegistry.contains(id)) {
        return &mEventRegistry.at(id);
    }

    if (id >= 0) {
        auto [it, inserted] = mEventRegistry.emplace(id, EventRegistration{ .Name = "UserDefined" });
        (void)inserted;
        return &it->second;
    }

    return nullptr;
}

EventID Events::RegisterEvent(const char* name) {
    const EventID id = this->mInternalEventID--;
    this->mEventRegistry[id] = EventRegistration{ .Name = name == nullptr ? "Unknown" : name };
    return id;
}

ListenerID Events::RegisterListener(EventID id, EventCallback callback, EventPriority priority, const char* file,
                                    int line) {
    auto* registry = GetOrCreateEventRegistration(id);
    if (registry == nullptr) {
        throw std::runtime_error("Trying to register listener for an invalid or unregistered event");
    }

    const ListenerID listenerId = registry->NextListenerID++;
    const uint64_t listenerSequence = registry->NextListenerSequence++;
    auto self = std::dynamic_pointer_cast<Tickable>(TryGetSharedComponent());
    auto listenerAction = std::make_shared<ListenerAction>(id, listenerId, priority, listenerSequence, this, self,
                                                           callback, EventMetadata{ file, line, 0 });

    registry->Listeners[listenerId] = listenerAction;
    GetActionList().Add(std::static_pointer_cast<Action>(listenerAction));
    return listenerId;
}

void Events::UnregisterListener(EventID id, ListenerID listenerId) {
    if (id == -1 || listenerId == -1) {
        return;
    }

    auto* registry = GetEventRegistration(id);
    if (registry == nullptr) {
        return;
    }

    auto listenerIt = registry->Listeners.find(listenerId);
    if (listenerIt == registry->Listeners.end()) {
        return;
    }

    GetActionList().Remove(std::static_pointer_cast<Action>(listenerIt->second));
    registry->Listeners.erase(listenerIt);
}

void Events::RemoveEvent(EventID id) {
    auto* registry = GetEventRegistration(id);
    if (registry == nullptr) {
        return;
    }

    std::vector<std::shared_ptr<Action>> listenerActions;
    listenerActions.reserve(registry->Listeners.size());
    for (const auto& [listenerId, listenerAction] : registry->Listeners) {
        (void)listenerId;
        listenerActions.push_back(std::static_pointer_cast<Action>(listenerAction));
    }

    if (!listenerActions.empty()) {
        GetActionList().Remove(listenerActions);
    }

    mEventRegistry.erase(id);
}

void Events::CallEvent(const EventID id, IEvent* event, const char* file, const int line, const char* key) {
    auto* registry = GetOrCreateEventRegistration(id);
    if (registry == nullptr) {
        return;
    }

    struct DispatchScope {
        explicit DispatchScope(std::vector<Events::DispatchContext>& stack, Events::DispatchContext context)
            : Stack(stack) {
            Stack.push_back(std::move(context));
        }

        ~DispatchScope() {
            Stack.pop_back();
        }

        std::vector<Events::DispatchContext>& Stack;
    } scope(mDispatchStack, DispatchContext{ id, event, file, line, key });

    Tick(id);

    if (key == nullptr) {
        return;
    }

// For performance reasons the Event Debugger tool should not be compiled in release mode
#ifdef _DEBUG
    auto& info = registry->Callers[std::string(key)];
    if (info.Path == nullptr) {
        info.Path = file;
        info.Line = line;
    }
    info.Count++;
#endif
}

const Events::DispatchContext* Events::GetActiveDispatchContext() const {
    if (mDispatchStack.empty()) {
        return nullptr;
    }

    return &mDispatchStack.back();
}

} // namespace Ship
