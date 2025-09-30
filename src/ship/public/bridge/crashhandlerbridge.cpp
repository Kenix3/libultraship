#include "crashhandlerbridge.h"
#include "ship/Context.h"
#include "debug/CrashHandler.h"

void CrashHandlerRegisterCallback(CrashHandlerCallback callback) {
    Ship::Context::GetInstance()->GetCrashHandler()->RegisterCallback(callback);
}
