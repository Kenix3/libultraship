#include "crashhandlerbridge.h"
#include "Context.h"

void CrashHandlerRegisterCallback(CrashHandlerCallback callback) {
    Ship::Context::GetInstance()->GetCrashHandler()->RegisterCallback(callback);
}
