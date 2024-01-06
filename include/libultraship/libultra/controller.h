#pragma once

#ifndef ULTRA64_CONTROLLER_H
#define ULTRA64_CONTROLLER_H

#include "stdint.h"

#define MAXCONTROLLERS 4

// /* Buttons */
#define BTN_CRIGHT 0x00001
#define BTN_CLEFT 0x00002
#define BTN_CDOWN 0x00004
#define BTN_CUP 0x00008
#define BTN_R 0x00010
#define BTN_L 0x00020
#define BTN_DRIGHT 0x00100
#define BTN_DLEFT 0x00200
#define BTN_DDOWN 0x00400
#define BTN_DUP 0x00800
#define BTN_START 0x01000
#define BTN_Z 0x02000
#define BTN_B 0x04000
#define BTN_A 0x08000

typedef struct {
    /* 0x00 */ uint16_t type;
    /* 0x02 */ uint8_t status;
    /* 0x03 */ uint8_t err_no;
} OSContStatus; // size = 0x04

typedef struct {
    /* 0x00 */ uint16_t button;
    /* 0x02 */ int8_t stick_x;
    /* 0x03 */ int8_t stick_y;
    /* 0x04 */ uint8_t err_no;
    /* 0x05 */ float gyro_x;
    /* 0x09 */ float gyro_y;
    /* 0x1C */ int8_t right_stick_x;
    /* 0x20 */ int8_t right_stick_y;
} OSContPad; // size = 0x24

#endif
