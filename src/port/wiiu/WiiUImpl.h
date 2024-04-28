#pragma once

#include <string>
#include <vpad/input.h>
#include <padscore/kpad.h>

namespace Ship {
namespace WiiU {

void Init(const std::string& shortName);

void Exit();

void ThrowMissingOTR(const char* otrPath);

void ThrowInvalidOTR();

void Update();

VPADStatus* GetVPADStatus(VPADReadError* error);

KPADStatus* GetKPADStatus(WPADChan chan, KPADError* error);

void SetControllersInitialized();

}; // namespace WiiU
}; // namespace Ship
