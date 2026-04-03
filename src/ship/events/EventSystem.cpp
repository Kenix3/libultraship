#include "ship/events/EventSystem.h"
#include <stdexcept>
#include <algorithm>

#include "ship/events/CoreEvents.h"

namespace Ship {

EventID EventSystem::RegisterEvent(const char* name) {
    const EventID id = this->mInternalEventID++;
    this->mEventRegistry[id] = EventRegistration{ .name = name == nullptr ? "Unknown" : name };
    return id;
}

ListenerID EventSystem::RegisterListener(EventID id, EventCallback callback, EventPriority priority, const char* file,
                                         int line) {
    if (id == -1) {
        throw std::runtime_error("Trying to register listener for unregistered event");
    }

    auto& registry = this->mEventRegistry[id];

    if (std::find_if(registry.listeners.begin(), registry.listeners.end(), [callback](const EventListener listener) {
            return listener.function == callback;
        }) != registry.listeners.end()) {
        throw std::runtime_error("Listener already registered");
    }

    registry.listeners.push_back({ priority, callback, { file, line, 0 } });

    std::sort(registry.listeners.begin(), registry.listeners.end(),
              [](const EventListener a, const EventListener b) { return a.priority < b.priority; });

    return registry.listeners.size() - 1;
}

void EventSystem::UnregisterListener(EventID id, ListenerID listenerId) {
    auto& registry = this->mEventRegistry[id];

    registry.listeners.erase(registry.listeners.begin() + listenerId);
}

void EventSystem::CallEvent(const EventID id, IEvent* event, const char* file, const int line, const char* key) {
    auto& registry = this->mEventRegistry[id];

    for (auto& [priority, function, _] : registry.listeners) {
        function(event);
    }

    auto& info = registry.callers[key];

    if (info.path == nullptr) {
        info.path = file;
        info.line = line;
    }

    info.count++;
}

} // namespace Ship