#pragma once

#include <vpad/input.h>
#include <padscore/kpad.h>

namespace LUS {
namespace WiiU {

void Init();

void Exit();

void ThrowMissingOTR(const char* otrPath);

void ThrowInvalidOTR();

void Update();

VPADStatus* GetVPADStatus(VPADReadError* error);

KPADStatus* GetKPADStatus(WPADChan chan, KPADError* error);

}; // namespace WiiU
}; // namespace LUS
