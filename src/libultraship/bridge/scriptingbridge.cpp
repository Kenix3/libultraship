#include "libultraship/bridge/scriptingbridge.h"
#include "ship/scripting/ScriptLoader.h"
#include "ship/Context.h"

extern "C" void* ScriptGetFunction(const char* module, const char* function) {
    return Ship::Context::GetInstance()->GetScriptLoader()->GetFunction(module, function);
}