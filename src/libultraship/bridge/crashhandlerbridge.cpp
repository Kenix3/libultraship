#include "libultraship/bridge/crashhandlerbridge.h"
#include "ship/Context.h"
#include "ship/debug/CrashHandler.h"

void CrashHandlerRegisterCallback(CrashHandlerCallback callback) {
    Ship::Context::GetInstance()->GetCrashHandler()->RegisterCallback(callback);
}
