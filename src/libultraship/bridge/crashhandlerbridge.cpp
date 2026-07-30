#include "libultraship/bridge/crashhandlerbridge.h"
#include "ship/debug/CrashHandler.h"
#include <atomic>

static std::atomic<std::shared_ptr<Ship::CrashHandler>> sCrashHandler;

void CrashHandlerSetComponent(std::shared_ptr<Ship::CrashHandler> crashHandler) {
    sCrashHandler.store(std::move(crashHandler), std::memory_order_release);
}

std::shared_ptr<Ship::CrashHandler> CrashHandlerGetComponent() {
    return sCrashHandler.load(std::memory_order_acquire);
}

void CrashHandlerRegisterCallback(CrashHandlerCallback callback) {
    if (auto crashHandler = CrashHandlerGetComponent()) {
        crashHandler->RegisterCallback(callback);
    }
}
