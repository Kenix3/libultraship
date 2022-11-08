#pragma once

#ifndef LIBULTRASHIP_H
#define LIBULTRASHIP_H

#include "stdint.h"
#include "UltraController.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OSMesgQueue;
int32_t osContInit(OSMesgQueue* mq, uint8_t* controllerBits, OSContStatus* status);
int32_t osContStartReadData(OSMesgQueue* mesg);
void osContGetReadData(OSContPad* pad);

uint64_t osGetTime(void);
uint32_t osGetCount(void);

#ifdef __cplusplus
};
#endif

#endif