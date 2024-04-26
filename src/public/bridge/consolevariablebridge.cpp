#include "public/bridge/consolevariablebridge.h"
#include "Context.h"

std::shared_ptr<ShipDK::CVar> CVarGet(const char* name) {
    return ShipDK::Context::GetInstance()->GetConsoleVariables()->Get(name);
}

extern "C" {
int32_t CVarGetInteger(const char* name, int32_t defaultValue) {
    return ShipDK::Context::GetInstance()->GetConsoleVariables()->GetInteger(name, defaultValue);
}

float CVarGetFloat(const char* name, float defaultValue) {
    return ShipDK::Context::GetInstance()->GetConsoleVariables()->GetFloat(name, defaultValue);
}

const char* CVarGetString(const char* name, const char* defaultValue) {
    return ShipDK::Context::GetInstance()->GetConsoleVariables()->GetString(name, defaultValue);
}

Color_RGBA8 CVarGetColor(const char* name, Color_RGBA8 defaultValue) {
    return ShipDK::Context::GetInstance()->GetConsoleVariables()->GetColor(name, defaultValue);
}

Color_RGB8 CVarGetColor24(const char* name, Color_RGB8 defaultValue) {
    return ShipDK::Context::GetInstance()->GetConsoleVariables()->GetColor24(name, defaultValue);
}

void CVarSetInteger(const char* name, int32_t value) {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->SetInteger(name, value);
}

void CVarSetFloat(const char* name, float value) {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->SetFloat(name, value);
}

void CVarSetString(const char* name, const char* value) {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->SetString(name, value);
}

void CVarSetColor(const char* name, Color_RGBA8 value) {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->SetColor(name, value);
}

void CVarSetColor24(const char* name, Color_RGB8 value) {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->SetColor24(name, value);
}

void CVarRegisterInteger(const char* name, int32_t defaultValue) {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->RegisterInteger(name, defaultValue);
}

void CVarRegisterFloat(const char* name, float defaultValue) {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->RegisterFloat(name, defaultValue);
}

void CVarRegisterString(const char* name, const char* defaultValue) {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->RegisterString(name, defaultValue);
}

void CVarRegisterColor(const char* name, Color_RGBA8 defaultValue) {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->RegisterColor(name, defaultValue);
}

void CVarRegisterColor24(const char* name, Color_RGB8 defaultValue) {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->RegisterColor24(name, defaultValue);
}

void CVarClear(const char* name) {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->ClearVariable(name);
}

void CVarLoad() {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->Load();
}

void CVarSave() {
    ShipDK::Context::GetInstance()->GetConsoleVariables()->Save();
}
}
