#include "StrHash.h"
#include <cstring>
#include <cstdint>
#include <iostream>

#define A(x) B(x) B(x + 128)
#define B(x) C(x) C(x +  64)
#define C(x) D(x) D(x +  32)
#define D(x) E(x) E(x +  16)
#define E(x) F(x) F(x +   8)
#define F(x) G(x) G(x +   4)
#define G(x) H(x) H(x +   2)
#define H(x) I(x) I(x +   1)
#define I(x) f<x>::value ,

namespace OtrLib
{
    /*uint32_t crc32_for_byte(uint32_t r)
    {
        for (int j = 0; j < 8; ++j)
            r = (r & 1 ? 0 : (uint32_t)0xEDB88320L) ^ r >> 1;

        return r ^ (uint32_t)0xFF000000L;
    }

    uint32_t crc32(const void* data, size_t len)
    {
        uint32_t crc = 0;
        static uint32_t table[0x100];

        if (!*table)
        {
            for (size_t i = 0; i < 0x100; ++i)
                table[i] = crc32_for_byte(i);
        }

        for (size_t i = 0; i < len; ++i)
            crc = table[(uint8_t)crc ^ ((uint8_t*)data)[i]] ^ crc >> 8;

        return crc;
    }*/

    // Generate CRC lookup table
    template <unsigned c, int k = 8>
    struct f : f<((c & 1) ? 0xedb88320 : 0) ^ (c >> 1), k - 1> {};
    template <unsigned c> struct f<c, 0> { enum { value = c }; };

    constexpr unsigned crc_table[] = { A(0) };

    // Constexpr implementation and helpers
    constexpr uint32_t crc32_impl(const uint8_t* p, size_t len, uint32_t crc) 
    {
        return len ?
            crc32_impl(p + 1, len - 1, (crc >> 8) ^ crc_table[(crc & 0xFF) ^ *p])
            : crc;
    }

    constexpr uint32_t crc32(const uint8_t* data, size_t length) 
    {
        return ~crc32_impl(data, length, ~0);
    }

    constexpr size_t strlen_c(const char* str) 
    {
        return *str ? 1 + strlen_c(str + 1) : 0;
    }

    constexpr int WSID(const char* str) 
    {
        return crc32((uint8_t*)str, strlen_c(str));
    }
}