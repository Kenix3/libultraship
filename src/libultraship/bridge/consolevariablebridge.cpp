#include "libultraship/bridge/consolevariablebridge.h"
#include "ship/Context.h"

static std::shared_ptr<Ship::ConsoleVariable> sConsoleVariable;

static Ship::ConsoleVariable* GetConsoleVariable() {
    if (!sConsoleVariable) {
        sConsoleVariable = Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>();
    }
    return sConsoleVariable.get();
}


std::shared_ptr<Ship::CVar> CVarGet(const char* name) {
    return GetConsoleVariable()->Get(name);
}

extern "C" {
int32_t CVarGetInteger(const char* name, int32_t defaultValue) {
    return GetConsoleVariable()->GetInteger(name,
                                                                                                     defaultValue);
}

float CVarGetFloat(const char* name, float defaultValue) {
    return GetConsoleVariable()->GetFloat(name, defaultValue);
}

const char* CVarGetString(const char* name, const char* defaultValue) {
    return GetConsoleVariable()->GetString(name, defaultValue);
}

Color_RGBA8 CVarGetColor(const char* name, Color_RGBA8 defaultValue) {
    return GetConsoleVariable()->GetColor(name, defaultValue);
}

Color_RGB8 CVarGetColor24(const char* name, Color_RGB8 defaultValue) {
    return GetConsoleVariable()->GetColor24(name,
                                                                                                     defaultValue);
}

void CVarSetInteger(const char* name, int32_t value) {
    GetConsoleVariable()->SetInteger(name, value);
}

void CVarSetFloat(const char* name, float value) {
    GetConsoleVariable()->SetFloat(name, value);
}

void CVarSetString(const char* name, const char* value) {
    GetConsoleVariable()->SetString(name, value);
}

void CVarSetColor(const char* name, Color_RGBA8 value) {
    GetConsoleVariable()->SetColor(name, value);
}

void CVarSetColor24(const char* name, Color_RGB8 value) {
    GetConsoleVariable()->SetColor24(name, value);
}

void CVarRegisterInteger(const char* name, int32_t defaultValue) {
    GetConsoleVariable()->RegisterInteger(name, defaultValue);
}

void CVarRegisterFloat(const char* name, float defaultValue) {
    GetConsoleVariable()->RegisterFloat(name, defaultValue);
}

void CVarRegisterString(const char* name, const char* defaultValue) {
    GetConsoleVariable()->RegisterString(name, defaultValue);
}

void CVarRegisterColor(const char* name, Color_RGBA8 defaultValue) {
    GetConsoleVariable()->RegisterColor(name, defaultValue);
}

void CVarRegisterColor24(const char* name, Color_RGB8 defaultValue) {
    GetConsoleVariable()->RegisterColor24(name, defaultValue);
}

void CVarClear(const char* name) {
    GetConsoleVariable()->ClearVariable(name);
}

void CVarClearBlock(const char* name) {
    GetConsoleVariable()->ClearBlock(name);
}

void CVarCopy(const char* from, const char* to) {
    GetConsoleVariable()->CopyVariable(from, to);
}

void CVarLoad() {
    GetConsoleVariable()->Load();
}

void CVarSave() {
    GetConsoleVariable()->Save();
}
}
