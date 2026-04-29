#pragma once

#ifdef ENABLE_SCRIPTING

#include "stdint.h"
#include "ship/Api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Looks up and returns a function pointer exported from the scripting runtime.
 *
 * The scripting bridge exposes compiled script functions to native C/C++ code via
 * this single entry point. The caller is responsible for casting the returned pointer
 * to the correct function signature before calling it.
 *
 * @param module   Name of the scripting module that exports the function.
 * @param function Name of the function to look up within @p module.
 * @return Opaque function pointer, or nullptr if the module or function is not found.
 */
API_EXPORT void* ScriptGetFunction(const char* module, const char* function);

#ifdef __cplusplus
}
#endif

#endif // ENABLE_SCRIPTING
