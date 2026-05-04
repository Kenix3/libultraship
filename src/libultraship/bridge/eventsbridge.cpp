#include "libultraship/bridge/eventsbridge.h"
#include "ship/events/Events.h"
#include "ship/Context.h"

static std::shared_ptr<Ship::Events> sEvents;

static Ship::Events* GetEvents() {
    if (!sEvents) {
        sEvents = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Events>();
    }
    return sEvents.get();
}


extern "C" {

EventID EventSystemRegisterEvent(const char* name) {
    return GetEvents()->RegisterEvent(name);
}

ListenerID EventSystemRegisterListener(EventID id, EventCallback callback, EventPriority priority, const char* file,
                                       int line) {
    return GetEvents()->RegisterListener(id, callback,
                                                                                                  priority, file, line);
}

void EventSystemUnregisterListener(EventID ev, ListenerID id) {
    GetEvents()->UnregisterListener(ev, id);
}

void EventSystemCallEvent(EventID id, void* event, const char* file, int line, const char* key) {
    GetEvents()->CallEvent(id, static_cast<IEvent*>(event),
                                                                                    file, line, key);
}
}