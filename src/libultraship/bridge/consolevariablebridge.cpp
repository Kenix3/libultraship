#include "libultraship/bridge/consolevariablebridge.h"
#include "ship/Context.h"

std::shared_ptr<Ship::CVar> CVarGet(const char* name) {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->Get(name);
}

extern "C" {
int32_t CVarGetInteger(const char* name, int32_t defaultValue) {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->GetInteger(name, defaultValue);
}

float CVarGetFloat(const char* name, float defaultValue) {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->GetFloat(name, defaultValue);
}

const char* CVarGetString(const char* name, const char* defaultValue) {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->GetString(name, defaultValue);
}

Color_RGBA8 CVarGetColor(const char* name, Color_RGBA8 defaultValue) {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->GetColor(name, defaultValue);
}

Color_RGB8 CVarGetColor24(const char* name, Color_RGB8 defaultValue) {
    return Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->GetColor24(name, defaultValue);
}

void CVarSetInteger(const char* name, int32_t value) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->SetInteger(name, value);
}

void CVarSetFloat(const char* name, float value) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->SetFloat(name, value);
}

void CVarSetString(const char* name, const char* value) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->SetString(name, value);
}

void CVarSetColor(const char* name, Color_RGBA8 value) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->SetColor(name, value);
}

void CVarSetColor24(const char* name, Color_RGB8 value) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->SetColor24(name, value);
}

void CVarRegisterInteger(const char* name, int32_t defaultValue) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->RegisterInteger(name, defaultValue);
}

void CVarRegisterFloat(const char* name, float defaultValue) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->RegisterFloat(name, defaultValue);
}

void CVarRegisterString(const char* name, const char* defaultValue) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->RegisterString(name, defaultValue);
}

void CVarRegisterColor(const char* name, Color_RGBA8 defaultValue) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->RegisterColor(name, defaultValue);
}

void CVarRegisterColor24(const char* name, Color_RGB8 defaultValue) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->RegisterColor24(name, defaultValue);
}

void CVarClear(const char* name) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->ClearVariable(name);
}

void CVarClearBlock(const char* name) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->ClearBlock(name);
}

void CVarCopy(const char* from, const char* to) {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->CopyVariable(from, to);
}

void CVarLoad() {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->Load();
}

void CVarSave() {
    Ship::Context::GetInstance()->GetChildren().GetFirst<Ship::ConsoleVariable>()->Save();
}
}
