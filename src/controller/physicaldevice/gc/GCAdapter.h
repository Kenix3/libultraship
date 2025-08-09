// Copyright 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include "GCAdapterStubs.h"

struct GCPadStatus;

namespace GCAdapter {
void Init();
void ResetRumble();
void Shutdown();
void SetAdapterCallback(std::function<void(void)> func);
void StartScanThread();
void StopScanThread();

// Buttons have PAD_GET_ORIGIN set on new connection
// Netplay and CSIDevice_GCAdapter make use of this.
GCPadStatus Input(int chan);

void Output(int chan, uint8_t rumble_command);
bool IsDetected(const char** error_message);
bool DeviceConnected(int chan);
void ResetDeviceType(int chan);
bool UseAdapter();

} // namespace GCAdapter
