#include "ship/events/Events.h"
#include <stdexcept>
#include <algorithm>

#include "ship/events/CoreEvents.h"

namespace Ship {

EventID Events::RegisterEvent(const char* name) {
    const EventID id = this->mInternalEventID++;
    this->mEventRegistry[id] = EventRegistration{ .Name = name == nullptr ? "Unknown" : name };
    return id;
}

ListenerID Events::RegisterListener(EventID id, EventCallback callback, EventPriority priority, const char* file,
                                    int line) {
    if (id == -1) {
        throw std::runtime_error("Trying to register listener for unregistered event");
    }

    auto& registry = this->mEventRegistry[id];
    const EventListener newListener = { registry.NextListenerID++, priority, callback, { file, line, 0 } };

    auto insertIt = std::lower_bound(registry.Listeners.begin(), registry.Listeners.end(), newListener,
                                     [](const EventListener& existingListener, const EventListener& listenerToInsert) {
                                         return existingListener.Priority < listenerToInsert.Priority;
                                     });

    registry.Listeners.insert(insertIt, newListener);

    return newListener.ID;
}

void Events::UnregisterListener(EventID id, ListenerID listenerId) {
    if (id == -1) {
        return;
    }

    if (listenerId == -1) {
        return;
    }

    auto& registry = this->mEventRegistry[id];

    auto it = std::find_if(registry.Listeners.begin(), registry.Listeners.end(),
                           [listenerId](const EventListener& listener) { return listener.ID == listenerId; });

    if (it == registry.Listeners.end()) {
        return;
    }

    registry.Listeners.erase(it);
}

void Events::CallEvent(const EventID id, IEvent* event, const char* file, const int line, const char* key) {
    auto& registry = this->mEventRegistry[id];

    for (auto& listener : registry.Listeners) {
        listener.Function(event);
    }

    auto& info = registry.Callers[key];

    if (info.Path == nullptr) {
        info.Path = file;
        info.Line = line;
    }

    info.Count++;
}

} // namespace Ship
