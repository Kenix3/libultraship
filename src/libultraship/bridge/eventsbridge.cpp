#include "libultraship/bridge/eventsbridge.h"
#include "ship/events/Events.h"

static std::shared_ptr<Ship::Events> sEvents;

void EventSystemSetEvents(std::shared_ptr<Ship::Events> events) {
    sEvents = std::move(events);
}

std::shared_ptr<Ship::Events> EventSystemGetEvents() {
    return sEvents;
}

static Ship::Events* GetEvents() {
    return sEvents.get();
}

extern "C" {

EventID EventSystemRegisterEvent(const char* name) {
    return GetEvents()->RegisterEvent(name);
}

ListenerID EventSystemRegisterListener(EventID id, EventCallback callback, EventPriority priority, const char* file,
                                       int line) {
    return GetEvents()->RegisterListener(id, callback, priority, file, line);
}

void EventSystemUnregisterListener(EventID ev, ListenerID id) {
    GetEvents()->UnregisterListener(ev, id);
}

void EventSystemCallEvent(EventID id, void* event, const char* file, int line, const char* key) {
    GetEvents()->CallEvent(id, static_cast<IEvent*>(event), file, line, key);
}
}