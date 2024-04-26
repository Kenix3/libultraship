#include "crashhandlerbridge.h"
#include "Context.h"

void CrashHandlerRegisterCallback(CrashHandlerCallback callback) {
    ShipDK::Context::GetInstance()->GetCrashHandler()->RegisterCallback(callback);
}
