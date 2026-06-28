#pragma once

#include "message.h"
#include "ship/Api.h"

#define EEPROM_TYPE_4K 0x01
#define EEPROM_TYPE_16K 0x02

API_EXPORT int32_t osEepromProbe(OSMesgQueue*);
API_EXPORT int32_t osEepromLongRead(OSMesgQueue*, uint8_t, uint8_t*, int32_t);
API_EXPORT int32_t osEepromLongWrite(OSMesgQueue*, uint8_t, uint8_t*, int32_t);
