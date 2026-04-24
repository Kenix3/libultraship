#include "libultraship/bridge/scriptingbridge.h"

#ifndef DISABLE_SCRIPTING

#include "ship/scripting/ScriptLoader.h"
#include "ship/Context.h"

extern "C" void* ScriptGetFunction(const char* module, const char* function) {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ScriptLoader>()->GetFunction(module, function);
}

#endif // DISABLE_SCRIPTING