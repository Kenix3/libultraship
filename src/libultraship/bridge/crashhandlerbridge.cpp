#include "libultraship/bridge/crashhandlerbridge.h"
#include "ship/debug/CrashHandler.h"

static std::shared_ptr<Ship::CrashHandler> sCrashHandler;

void CrashHandlerSetComponent(std::shared_ptr<Ship::CrashHandler> crashHandler) {
    sCrashHandler = std::move(crashHandler);
}

std::shared_ptr<Ship::CrashHandler> CrashHandlerGetComponent() {
    return sCrashHandler;
}

static Ship::CrashHandler* GetCrashHandler() {
    return sCrashHandler.get();
}

void CrashHandlerRegisterCallback(CrashHandlerCallback callback) {
    GetCrashHandler()->RegisterCallback(callback);
}
