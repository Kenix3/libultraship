#include "ship/events/EventSystem.h"
#include <stdexcept>
#include <algorithm>

#include "ship/events/CoreEvents.h"

namespace Ship {

EventID EventSystem::RegisterEvent(const char* name) {
    const EventID id = this->mInternalEventID++;
    this->mEventRegistry[id] = EventRegistration{ .Name = name == nullptr ? "Unknown" : name };
    return id;
}

ListenerID EventSystem::RegisterListener(EventID id, EventCallback callback, EventPriority priority, const char* file,
                                         int line) {
    if (id == -1) {
        throw std::runtime_error("Trying to register listener for unregistered event");
    }

    auto& registry = this->mEventRegistry[id];

    if (std::find_if(registry.Listeners.begin(), registry.Listeners.end(), [callback](const EventListener listener) {
            return listener.Function == callback;
        }) != registry.Listeners.end()) {
        throw std::runtime_error("Listener already registered");
    }

    registry.Listeners.push_back({ priority, callback, { file, line, 0 } });

    std::sort(registry.Listeners.begin(), registry.Listeners.end(),
              [](const EventListener a, const EventListener b) { return a.Priority < b.Priority; });

    return registry.Listeners.size() - 1;
}

void EventSystem::UnregisterListener(EventID id, ListenerID listenerId) {
    auto& registry = this->mEventRegistry[id];

    registry.Listeners.erase(registry.Listeners.begin() + listenerId);
}

void EventSystem::CallEvent(const EventID id, IEvent* event, const char* file, const int line, const char* key) {
    auto& registry = this->mEventRegistry[id];

    for (auto& [priority, function, _] : registry.Listeners) {
        function(event);
    }

    auto& info = registry.Callers[key];

    if (info.Path == nullptr) {
        info.Path = file;
        info.Line = line;
    }

    info.Count++;
}

} // namespace Ship