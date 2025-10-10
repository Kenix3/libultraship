#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <fast/types.h>

typedef signed char s8;
typedef unsigned char u8;
typedef signed short int s16;
typedef unsigned short int u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long int s64;
typedef unsigned long long int u64;

typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;
typedef volatile s8 vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;

typedef float f32;
typedef double f64;
#if 0

typedef s32 ptrdiff_t;
typedef s32 intptr_t;
typedef u32 uintptr_t;
#endif
