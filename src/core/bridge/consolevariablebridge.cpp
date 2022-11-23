#include "core/bridge/consolevariablebridge.h"
#include "core/ConsoleVariable.h"
#include "core/Window.h"

std::shared_ptr<Ship::CVar> CVarGet(const char* name) {
    return Ship::Window::GetInstance()->GetConsoleVariables()->Get(name);
}

extern "C" {
int32_t CVarGetInteger(const char* name, int32_t defaultValue) {
    return Ship::Window::GetInstance()->GetConsoleVariables()->GetInteger(name, defaultValue);
}

float CVarGetFloat(const char* name, float defaultValue) {
    return Ship::Window::GetInstance()->GetConsoleVariables()->GetFloat(name, defaultValue);
}

const char* CVarGetString(const char* name, const char* defaultValue) {
    return Ship::Window::GetInstance()->GetConsoleVariables()->GetString(name, defaultValue);
}

Color_RGBA8 CVarGetColor(const char* name, Color_RGBA8 defaultValue) {
    return Ship::Window::GetInstance()->GetConsoleVariables()->GetColor(name, defaultValue);
}

Color_RGB8 CVarGetColor24(const char* name, Color_RGB8 defaultValue) {
    return Ship::Window::GetInstance()->GetConsoleVariables()->GetColor24(name, defaultValue);
}

void CVarSetInteger(const char* name, int32_t value) {
    Ship::Window::GetInstance()->GetConsoleVariables()->SetInteger(name, value);
}

void CVarSetFloat(const char* name, float value) {
    Ship::Window::GetInstance()->GetConsoleVariables()->SetFloat(name, value);
}

void CVarSetString(const char* name, const char* value) {
    Ship::Window::GetInstance()->GetConsoleVariables()->SetString(name, value);
}

void CVarSetColor(const char* name, Color_RGBA8 value) {
    Ship::Window::GetInstance()->GetConsoleVariables()->SetColor(name, value);
}

void CVarSetColor24(const char* name, Color_RGB8 value) {
    Ship::Window::GetInstance()->GetConsoleVariables()->SetColor24(name, value);
}

void CVarRegisterInteger(const char* name, int32_t defaultValue) {
    Ship::Window::GetInstance()->GetConsoleVariables()->RegisterInteger(name, defaultValue);
}

void CVarRegisterFloat(const char* name, float defaultValue) {
    Ship::Window::GetInstance()->GetConsoleVariables()->RegisterFloat(name, defaultValue);
}

void CVarRegisterString(const char* name, const char* defaultValue) {
    Ship::Window::GetInstance()->GetConsoleVariables()->RegisterString(name, defaultValue);
}

void CVarRegisterColor(const char* name, Color_RGBA8 defaultValue) {
    Ship::Window::GetInstance()->GetConsoleVariables()->RegisterColor(name, defaultValue);
}

void CVarRegisterColor24(const char* name, Color_RGB8 defaultValue) {
    Ship::Window::GetInstance()->GetConsoleVariables()->RegisterColor24(name, defaultValue);
}

void CVarClear(const char* name) {
    Ship::Window::GetInstance()->GetConsoleVariables()->ClearVariable(name);
}

void CVarLoad() {
    Ship::Window::GetInstance()->GetConsoleVariables()->Load();
}

void CVarSave() {
    Ship::Window::GetInstance()->GetConsoleVariables()->Save();
}
}
