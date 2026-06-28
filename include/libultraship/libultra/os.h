#pragma once

#ifndef OS_H
#define OS_H

#include <stdint.h>
#include "controller.h"
#include "message.h"
#include "time.h"
#include "pi.h"
#include "vi.h"
#include "ship/Api.h"

#ifndef _HW_VERSION_1
#define MAXCONTROLLERS 4
#else
#define MAXCONTROLLERS 6
#endif

/* controller errors */
#define CONT_NO_RESPONSE_ERROR 0x8
#define CONT_OVERRUN_ERROR 0x4
#define CONT_RANGE_ERROR -1
#ifdef _HW_VERSION_1
#define CONT_FRAME_ERROR 0x2
#define CONT_COLLISION_ERROR 0x1
#endif

/* Controller type */

#define CONT_ABSOLUTE 0x0001
#define CONT_RELATIVE 0x0002
#define CONT_JOYPORT 0x0004
#define CONT_EEPROM 0x8000
#define CONT_EEP16K 0x4000
#define CONT_TYPE_MASK 0x1F07
#define CONT_TYPE_NORMAL 0x0005
#define CONT_TYPE_MOUSE 0x0002
#define CONT_TYPE_VOICE 0x0100

/* Controller status */

#define CONT_CARD_ON 0x01
#define CONT_CARD_PULL 0x02
#define CONT_ADDR_CRC_ER 0x04
#define CONT_EEPROM_BUSY 0x80

/* Buttons */

#define CONT_A 0x8000
#define CONT_B 0x4000
#define CONT_G 0x2000
#define CONT_START 0x1000
#define CONT_UP 0x0800
#define CONT_DOWN 0x0400
#define CONT_LEFT 0x0200
#define CONT_RIGHT 0x0100
#define CONT_L 0x0020
#define CONT_R 0x0010
#define CONT_E 0x0008
#define CONT_D 0x0004
#define CONT_C 0x0002
#define CONT_F 0x0001

/* Nintendo's official button names */

#define A_BUTTON CONT_A
#define B_BUTTON CONT_B
#define L_TRIG CONT_L
#define R_TRIG CONT_R
#define Z_TRIG CONT_G
#define START_BUTTON CONT_START
#define U_JPAD CONT_UP
#define L_JPAD CONT_LEFT
#define R_JPAD CONT_RIGHT
#define D_JPAD CONT_DOWN
#define U_CBUTTONS CONT_E
#define L_CBUTTONS CONT_C
#define R_CBUTTONS CONT_F
#define D_CBUTTONS CONT_D

/* Controller error number */

#define CONT_ERR_NO_CONTROLLER PFS_ERR_NOPACK /* 1 */
#define CONT_ERR_CONTRFAIL CONT_OVERRUN_ERROR /* 4 */
#define CONT_ERR_INVALID PFS_ERR_INVALID      /* 5 */
#define CONT_ERR_DEVICE PFS_ERR_DEVICE        /* 11 */
#define CONT_ERR_NOT_READY 12
#define CONT_ERR_VOICE_MEMORY 13
#define CONT_ERR_VOICE_WORD 14
#define CONT_ERR_VOICE_NO_RESPONSE 15

// EEPROM

#define EEPROM_TYPE_4K 0x01
#define EEPROM_TYPE_16K 0x02

#define EEPROM_MAXBLOCKS 64
#define EEP16K_MAXBLOCKS 256
#define EEPROM_BLOCK_SIZE 8

API_EXPORT int32_t osContInit(OSMesgQueue* mq, uint8_t* controllerBits, OSContStatus* status);
API_EXPORT int32_t osContStartReadData(OSMesgQueue* mesg);
API_EXPORT void osContGetReadData(OSContPad* pad);
API_EXPORT uint8_t osContGetStatus(uint8_t controller);

API_EXPORT void osWritebackDCacheAll();
API_EXPORT void osInvalDCache(void* p, int32_t l);
API_EXPORT void osInvalICache(void* p, int32_t x);
API_EXPORT void osWritebackDCache(void* p, int32_t x);

API_EXPORT s32 osPiStartDma(OSIoMesg* mb, s32 priority, s32 direction, uintptr_t devAddr, void* vAddr, size_t nbytes,
                            OSMesgQueue* mq);
API_EXPORT void osViSwapBuffer(void*);
API_EXPORT void osViBlack(uint8_t active);
API_EXPORT void osViFade(u8, u16);
API_EXPORT void osViRepeatLine(u8);
API_EXPORT void osViSetXScale(f32);
API_EXPORT void osViSetYScale(f32);
API_EXPORT void osViSetSpecialFeatures(u32);
API_EXPORT void osViSetMode(OSViMode*);
API_EXPORT void osViSetEvent(OSMesgQueue*, OSMesg, u32);
API_EXPORT void osCreateViManager(OSPri);
API_EXPORT void osCreatePiManager(OSPri pri, OSMesgQueue* cmdQ, OSMesg* cmdBuf, s32 cmdMsgCnt);

API_EXPORT void osSetTime(OSTime time);
API_EXPORT uint64_t osGetTime(void);
API_EXPORT uint32_t osGetCount(void);
API_EXPORT s32 osEepromProbe(OSMesgQueue*);
API_EXPORT s32 osEepromRead(OSMesgQueue*, u8, u8*);
API_EXPORT s32 osEepromWrite(OSMesgQueue*, u8, u8*);
API_EXPORT s32 osEepromLongRead(OSMesgQueue*, u8, u8*, int);
API_EXPORT s32 osEepromLongWrite(OSMesgQueue*, u8, u8*, int);

API_EXPORT int osSetTimer(OSTimer* t, OSTime countdown, OSTime interval, OSMesgQueue* mq, OSMesg msg);

API_EXPORT s32 osAiSetFrequency(u32 freq);
API_EXPORT OSPiHandle* osCartRomInit(void);
API_EXPORT s32 osEPiStartDma(OSPiHandle* pihandle, OSIoMesg* mb, s32 direction);

API_EXPORT s32 osAiSetFrequency(u32);
API_EXPORT s32 osAiSetNextBuffer(void*, size_t);
API_EXPORT u32 osAiGetLength(void);

#endif
