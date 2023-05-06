#pragma once

#include <vpad/input.h>
#include <padscore/kpad.h>
#include <string>

namespace Ship {
namespace WiiU {

void Init(const std::string& shortName);

void Exit();

void ThrowMissingOTR(const char* otrPath);

void ThrowInvalidOTR();

void Update();

VPADStatus* GetVPADStatus(VPADReadError* error);

KPADStatus* GetKPADStatus(WPADChan chan, KPADError* error);

}; // namespace WiiU
}; // namespace Ship
