#include "libultraship/bridge/consolevariablebridge.h"
#include <atomic>

static std::atomic<std::shared_ptr<Ship::ConsoleVariable>> sConsoleVariable;

void CVarSetConsoleVariable(std::shared_ptr<Ship::ConsoleVariable> consoleVariable) {
    sConsoleVariable.store(std::move(consoleVariable), std::memory_order_release);
}

std::shared_ptr<Ship::ConsoleVariable> CVarGetConsoleVariable() {
    return sConsoleVariable.load(std::memory_order_acquire);
}

std::shared_ptr<Ship::CVar> CVarGet(const char* name) {
    auto cvars = CVarGetConsoleVariable();
    return cvars ? cvars->Get(name) : nullptr;
}

extern "C" {
int32_t CVarGetInteger(const char* name, int32_t defaultValue) {
    auto cvars = CVarGetConsoleVariable();
    return cvars ? cvars->GetInteger(name, defaultValue) : defaultValue;
}

float CVarGetFloat(const char* name, float defaultValue) {
    auto cvars = CVarGetConsoleVariable();
    return cvars ? cvars->GetFloat(name, defaultValue) : defaultValue;
}

const char* CVarGetString(const char* name, const char* defaultValue) {
    auto cvars = CVarGetConsoleVariable();
    return cvars ? cvars->GetString(name, defaultValue) : defaultValue;
}

Color_RGBA8 CVarGetColor(const char* name, Color_RGBA8 defaultValue) {
    auto cvars = CVarGetConsoleVariable();
    return cvars ? cvars->GetColor(name, defaultValue) : defaultValue;
}

Color_RGB8 CVarGetColor24(const char* name, Color_RGB8 defaultValue) {
    auto cvars = CVarGetConsoleVariable();
    return cvars ? cvars->GetColor24(name, defaultValue) : defaultValue;
}

void CVarSetInteger(const char* name, int32_t value) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->SetInteger(name, value);
    }
}

void CVarSetFloat(const char* name, float value) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->SetFloat(name, value);
    }
}

void CVarSetString(const char* name, const char* value) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->SetString(name, value);
    }
}

void CVarSetColor(const char* name, Color_RGBA8 value) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->SetColor(name, value);
    }
}

void CVarSetColor24(const char* name, Color_RGB8 value) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->SetColor24(name, value);
    }
}

void CVarRegisterInteger(const char* name, int32_t defaultValue) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->RegisterInteger(name, defaultValue);
    }
}

void CVarRegisterFloat(const char* name, float defaultValue) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->RegisterFloat(name, defaultValue);
    }
}

void CVarRegisterString(const char* name, const char* defaultValue) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->RegisterString(name, defaultValue);
    }
}

void CVarRegisterColor(const char* name, Color_RGBA8 defaultValue) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->RegisterColor(name, defaultValue);
    }
}

void CVarRegisterColor24(const char* name, Color_RGB8 defaultValue) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->RegisterColor24(name, defaultValue);
    }
}

void CVarClear(const char* name) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->ClearVariable(name);
    }
}

void CVarClearBlock(const char* name) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->ClearBlock(name);
    }
}

void CVarCopy(const char* from, const char* to) {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->CopyVariable(from, to);
    }
}

void CVarLoad() {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->Load();
    }
}

void CVarSave() {
    if (auto cvars = CVarGetConsoleVariable()) {
        cvars->Save();
    }
}
}
