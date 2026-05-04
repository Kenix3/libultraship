#include "libultraship/bridge/crashhandlerbridge.h"
#include "ship/Context.h"
#include "ship/debug/CrashHandler.h"

static std::shared_ptr<Ship::CrashHandler> sCrashHandler;

static Ship::CrashHandler* GetCrashHandler() {
    if (!sCrashHandler) {
        sCrashHandler = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::CrashHandler>();
    }
    return sCrashHandler.get();
}


void CrashHandlerRegisterCallback(CrashHandlerCallback callback) {
    GetCrashHandler()->RegisterCallback(callback);
}
