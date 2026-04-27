#include "libultraship/bridge/scriptingbridge.h"

#ifdef ENABLE_SCRIPTING

#include "ship/scripting/ScriptLoader.h"
#include "ship/Context.h"

extern "C" void* ScriptGetFunction(const char* module, const char* function) {
    return Ship::Context::GetInstance()->GetScriptLoader()->GetFunction(module, function);
}

#endif // ENABLE_SCRIPTING