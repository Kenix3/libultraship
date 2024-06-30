#include "gfxdebuggerbridge.h"
#include "Context.h"
#include "graphic/Fast3D/debug/GfxDebugger.h"

void GfxDebuggerRequestDebugging(void) {
    Ship::Context::GetInstance()->GetGfxDebugger()->RequestDebugging();
}
bool GfxDebuggerIsDebugging(void) {
    return Ship::Context::GetInstance()->GetGfxDebugger()->IsDebugging();
}
bool GfxDebuggerIsDebuggingRequested(void) {
    return Ship::Context::GetInstance()->GetGfxDebugger()->IsDebuggingRequested();
}
void GfxDebuggerDebugDisplayList(void* cmds) {
    Ship::Context::GetInstance()->GetGfxDebugger()->DebugDisplayList((F3DGfx*)cmds);
}
