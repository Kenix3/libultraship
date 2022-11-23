#include "crashhandlerbridge.h"
#include "core/Window.h"

void CrashHandlerRegisterCallback(CrashHandlerCallback callback) {
    Ship::Window::GetInstance()->GetCrashHandler()->RegisterCallback(callback);
}
