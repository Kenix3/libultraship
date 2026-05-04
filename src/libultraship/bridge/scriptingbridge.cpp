#include "libultraship/bridge/scriptingbridge.h"

#ifdef ENABLE_SCRIPTING

#include "ship/scripting/ScriptLoader.h"
#include "ship/Context.h"

// Dependency: requires Ship::ScriptLoader component to be present in Ship::Context (only when ENABLE_SCRIPTING is set).

static std::shared_ptr<Ship::ScriptLoader> sScriptLoader;

static Ship::ScriptLoader* GetScriptLoader() {
    if (!sScriptLoader) {
        sScriptLoader = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ScriptLoader>();
    }
    return sScriptLoader.get();
}

extern "C" void* ScriptGetFunction(const char* module, const char* function) {
    return GetScriptLoader()->GetFunction(module, function);
}

#endif // ENABLE_SCRIPTING