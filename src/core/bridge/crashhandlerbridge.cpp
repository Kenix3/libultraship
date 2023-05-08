#include "crashhandlerbridge.h"
#include "core/Context.h"

void CrashHandlerRegisterCallback(CrashHandlerCallback callback) {
    LUS::Context::GetInstance()->GetCrashHandler()->RegisterCallback(callback);
}
