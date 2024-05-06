#ifndef ULTRA64_EEPROM_H
#define ULTRA64_EEPROM_H

#include "message.h"

#define EEPROM_TYPE_4K 0x01
#define EEPROM_TYPE_16K 0x02

#ifdef __cplusplus
extern "C" {
#endif

int32_t osEepromProbe(OSMesgQueue*);
int32_t osEepromLongRead(OSMesgQueue*, uint8_t, uint8_t*, int32_t);
int32_t osEepromLongWrite(OSMesgQueue*, uint8_t, uint8_t*, int32_t);

#ifdef __cplusplus
}
#endif

#endif