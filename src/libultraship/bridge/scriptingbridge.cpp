#include "libultraship/bridge/scriptingbridge.h"
#include "ship/scripting/ScriptSystem.h"
#include "ship/Context.h"

extern "C" void* ScriptGetFunction(const char* module, const char* function) {
    return Ship::Context::GetInstance()->GetScriptSystem()->GetFunction(module, function);
}