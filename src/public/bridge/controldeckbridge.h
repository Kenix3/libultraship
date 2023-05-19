#pragma once

#ifndef CONTROLDECKBRIDGE_H
#define CONTROLDECKBRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void BlockGameInput(uint16_t inputBlockId);
void UnblockGameInput(uint16_t inputBlockId);

#ifdef __cplusplus
};
#endif

#endif