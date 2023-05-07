#include "crashhandlerbridge.h"
#include "core/Context.h"

void CrashHandlerRegisterCallback(CrashHandlerCallback callback) {
    Ship::Context::GetInstance()->GetCrashHandler()->RegisterCallback(callback);
}
