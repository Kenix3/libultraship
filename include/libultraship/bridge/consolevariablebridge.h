#pragma once

#include "stdint.h"
#include "libultraship/color.h"
#include "ship/Api.h"

#ifdef __cplusplus
#include <memory>
#include "ship/config/ConsoleVariable.h"
std::shared_ptr<Ship::CVar> CVarGet(const char* name);

extern "C" {
#endif

API_EXPORT int32_t CVarGetInteger(const char* name, int32_t defaultValue);
API_EXPORT float CVarGetFloat(const char* name, float defaultValue);
API_EXPORT const char* CVarGetString(const char* name, const char* defaultValue);
API_EXPORT Color_RGBA8 CVarGetColor(const char* name, Color_RGBA8 defaultValue);
API_EXPORT Color_RGB8 CVarGetColor24(const char* name, Color_RGB8 defaultValue);

API_EXPORT void CVarSetInteger(const char* name, int32_t value);
API_EXPORT void CVarSetFloat(const char* name, float value);
API_EXPORT void CVarSetString(const char* name, const char* value);
API_EXPORT void CVarSetColor(const char* name, Color_RGBA8 value);
API_EXPORT void CVarSetColor24(const char* name, Color_RGB8 value);

API_EXPORT void CVarRegisterInteger(const char* name, int32_t defaultValue);
API_EXPORT void CVarRegisterFloat(const char* name, float defaultValue);
API_EXPORT void CVarRegisterString(const char* name, const char* defaultValue);
API_EXPORT void CVarRegisterColor(const char* name, Color_RGBA8 defaultValue);
API_EXPORT void CVarRegisterColor24(const char* name, Color_RGB8 defaultValue);

API_EXPORT void CVarClear(const char* name);
API_EXPORT bool CVarExists(const char* name);
API_EXPORT void CVarClearBlock(const char* name);
API_EXPORT void CVarCopy(const char* from, const char* to);

API_EXPORT void CVarLoad();
API_EXPORT void CVarSave();

#ifdef __cplusplus
};
#endif
