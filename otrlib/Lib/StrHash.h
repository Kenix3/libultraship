#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

namespace OtrLib
{
	typedef uint32_t strhash;

	//uint32_t crc32(const void* data, size_t n_bytes);
	constexpr uint32_t crc32(const uint8_t* data, size_t length);
}