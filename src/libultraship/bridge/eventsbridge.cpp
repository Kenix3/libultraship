#include "libultraship/bridge/eventsbridge.h"
#include "ship/events/Events.h"
#include <atomic>

static std::atomic<std::shared_ptr<Ship::Events>> sEvents;

void EventSystemSetEvents(std::shared_ptr<Ship::Events> events) {
    sEvents.store(std::move(events), std::memory_order_release);
}

std::shared_ptr<Ship::Events> EventSystemGetEvents() {
    return sEvents.load(std::memory_order_acquire);
}

extern "C" {

EventID EventSystemRegisterEvent(const char* name) {
    auto events = EventSystemGetEvents();
    return events ? events->RegisterEvent(name) : -1;
}

ListenerID EventSystemRegisterListener(EventID id, EventCallback callback, EventPriority priority, const char* file,
                                       int line) {
    auto events = EventSystemGetEvents();
    return events ? events->RegisterListener(id, callback, priority, file, line) : -1;
}

void EventSystemUnregisterEvent(EventID id) {
    if (auto events = EventSystemGetEvents()) {
        events->RemoveEvent(id);
    }
}

void EventSystemUnregisterListener(EventID ev, ListenerID id) {
    if (auto events = EventSystemGetEvents()) {
        events->UnregisterListener(ev, id);
    }
}

void EventSystemCallEvent(EventID id, void* event, const char* file, int line, const char* key) {
    if (auto events = EventSystemGetEvents()) {
        events->CallEvent(id, static_cast<IEvent*>(event), file, line, key);
    }
}
}