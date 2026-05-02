#include "libultraship/bridge/eventsbridge.h"
#include "ship/events/Events.h"
#include "ship/Context.h"

extern "C" {

EventID EventSystemRegisterEvent(const char* name) {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Events>()->RegisterEvent(name);
}

ListenerID EventSystemRegisterListener(EventID id, EventCallback callback, EventPriority priority, const char* file,
                                       int line) {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Events>()->RegisterListener(id, callback,
                                                                                                  priority, file, line);
}

void EventSystemUnregisterListener(EventID ev, ListenerID id) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Events>()->UnregisterListener(ev, id);
}

void EventSystemCallEvent(EventID id, void* event, const char* file, int line, const char* key) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::Events>()->CallEvent(id, static_cast<IEvent*>(event),
                                                                                    file, line, key);
}
}