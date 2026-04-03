#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <unordered_map>

#include "ship/events/EventTypes.h"

namespace Ship {

struct EventRegistration {
    const char* name;
    std::unordered_map<const char*, EventMetadata> callers;
    std::vector<EventListener> listeners;
};

class EventSystem {
public:
    EventID RegisterEvent(const char* name = nullptr);
    ListenerID RegisterListener(EventID id, EventCallback callback, EventPriority priority = EVENT_PRIORITY_NORMAL, const char* file = nullptr, int line = 0);
    void UnregisterListener(EventID ev, ListenerID id);
    void CallEvent(EventID id, IEvent* event, const char* file = nullptr, int line = 0, const char* key = nullptr);

    EventRegistration* GetEventRegistration(EventID id) {
        if (mEventRegistry.contains(id)) {
            return &mEventRegistry.at(id);
        }
        return nullptr;
    }

    std::unordered_map<EventID, EventRegistration>& GetEventRegistrations() {
        return this->mEventRegistry;
    }
private:
    std::unordered_map<EventID, EventRegistration> mEventRegistry;
    EventID mInternalEventID = 0;
};

}