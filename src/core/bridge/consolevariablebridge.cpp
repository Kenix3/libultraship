#include "core/bridge/consolevariablebridge.h"
#include "core/ConsoleVariable.h"

extern "C" {
int32_t CVarGetInteger(const char* name, int32_t defaultValue);
float CVarGetFloat(const char* name, float defaultValue);
const char* CVarGetString(const char* name, const char* defaultValue);
Color_RGBA8 CVarGetColor(const char* name, Color_RGBA8 defaultValue);

void CVarSetInteger(const char* name, int32_t value);
void CVarSetFloat(const char* name, float value);
void CVarSetString(const char* name, const char* value);
void CVarSetColor(const char* name, Color_RGBA8 value);

void CVarRegisterInteger(const char* name, int32_t defaultValue);
void CVarRegisterFloat(const char* name, float defaultValue);
void CVarRegisterString(const char* name, const char* defaultValue);
void CVarRegisterColor(const char* name, Color_RGBA8 defaultValue);

void CVarClear(const char* name);

void CVarLoad();
void CVarSave();
}
