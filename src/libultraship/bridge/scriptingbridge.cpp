#include "libultraship/bridge/scriptingbridge.h"

#ifdef ENABLE_SCRIPTING

#include "ship/scripting/ScriptLoader.h"

// Dependency: requires Ship::ScriptLoader component to be present in Ship::Context (only when ENABLE_SCRIPTING is set).

static std::shared_ptr<Ship::ScriptLoader> sScriptLoader;

void ScriptSetLoader(std::shared_ptr<Ship::ScriptLoader> scriptLoader) {
    sScriptLoader = std::move(scriptLoader);
}

std::shared_ptr<Ship::ScriptLoader> ScriptGetLoader() {
    return sScriptLoader;
}

static Ship::ScriptLoader* GetScriptLoader() {
    return sScriptLoader.get();
}

extern "C" void* ScriptGetFunction(const char* module, const char* function) {
    return GetScriptLoader()->GetFunction(module, function);
}

#endif // ENABLE_SCRIPTING