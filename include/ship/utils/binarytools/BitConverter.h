#pragma once

#include <cstdint>
#include <limits>
#include <vector>
#include <cstring>

#ifdef _MSC_VER
#define BSWAP16 _byteswap_ushort ///< Byte-swap a 16-bit value (MSVC intrinsic).
#define BSWAP32 _byteswap_ulong  ///< Byte-swap a 32-bit value (MSVC intrinsic).
#define BSWAP64 _byteswap_uint64 ///< Byte-swap a 64-bit value (MSVC intrinsic).
#else
#define BSWAP16 __builtin_bswap16 ///< Byte-swap a 16-bit value (GCC/Clang built-in).
#define BSWAP32 __builtin_bswap32 ///< Byte-swap a 32-bit value (GCC/Clang built-in).
#define BSWAP64 __builtin_bswap64 ///< Byte-swap a 64-bit value (GCC/Clang built-in).
#endif

/**
 * @brief Identifies the byte order of the host or target platform.
 *
 * Used by BitConverter helpers that need to swap bytes between little-endian and
 * big-endian (Z64) representations.
 */
enum class Endianness {
    Little = 0, ///< Least-significant byte first.
    Big = 1,    ///< Most-significant byte first.

#if (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)) || defined(__BIG_ENDIAN__)
    Native = Big, ///< Platform is big-endian; Native resolves to Big.
#else
    Native = Little, ///< Platform is little-endian; Native resolves to Little.
#endif
};

/**
 * @brief Static helpers for reading big-endian binary values from raw buffers.
 *
 * BitConverter provides a set of inline static methods that extract integers and
 * floating-point values from an arbitrary byte offset within a raw @c uint8_t
 * buffer or @c std::vector<uint8_t>.  All read methods use big-endian (network)
 * byte order, matching the Z64 ROM format used by Nintendo 64 assets.
 *
 * An additional helper, RomToBigEndian(), detects the byte order of an N64 ROM
 * image (Z64, V64, or N64) and swaps it in-place to the canonical Z64 big-endian
 * layout expected by the resource loading pipeline.
 *
 * All methods are static — BitConverter is not intended to be instantiated.
 */
class BitConverter {
  public:
    /**
     * @brief Reads a signed 8-bit integer from @p data at @p offset (no byte swap needed).
     */
    static inline int8_t ToInt8BE(const uint8_t* data, int32_t offset) {
        return (uint8_t)data[offset + 0];
    }

    /** @copydoc ToInt8BE(const uint8_t*, int32_t) */
    static inline int8_t ToInt8BE(const std::vector<uint8_t>& data, int32_t offset) {
        return (uint8_t)data[offset + 0];
    }

    /**
     * @brief Reads an unsigned 8-bit integer from @p data at @p offset.
     */
    static inline uint8_t ToUInt8BE(const uint8_t* data, int32_t offset) {
        return (uint8_t)data[offset + 0];
    }

    /** @copydoc ToUInt8BE(const uint8_t*, int32_t) */
    static inline uint8_t ToUInt8BE(const std::vector<uint8_t>& data, int32_t offset) {
        return (uint8_t)data[offset + 0];
    }

    /**
     * @brief Reads a signed 16-bit big-endian integer from @p data at @p offset.
     */
    static inline int16_t ToInt16BE(const uint8_t* data, int32_t offset) {
        return ((uint16_t)data[offset + 0] << 8) + (uint16_t)data[offset + 1];
    }

    /** @copydoc ToInt16BE(const uint8_t*, int32_t) */
    static inline int16_t ToInt16BE(const std::vector<uint8_t>& data, int32_t offset) {
        return ((uint16_t)data[offset + 0] << 8) + (uint16_t)data[offset + 1];
    }

    /**
     * @brief Reads an unsigned 16-bit big-endian integer from @p data at @p offset.
     */
    static inline uint16_t ToUInt16BE(const uint8_t* data, int32_t offset) {
        return ((uint16_t)data[offset + 0] << 8) + (uint16_t)data[offset + 1];
    }

    /** @copydoc ToUInt16BE(const uint8_t*, int32_t) */
    static inline uint16_t ToUInt16BE(const std::vector<uint8_t>& data, int32_t offset) {
        return ((uint16_t)data[offset + 0] << 8) + (uint16_t)data[offset + 1];
    }

    /**
     * @brief Reads a signed 32-bit big-endian integer from @p data at @p offset.
     */
    static inline int32_t ToInt32BE(const uint8_t* data, int32_t offset) {
        return ((uint32_t)data[offset + 0] << 24) + ((uint32_t)data[offset + 1] << 16) +
               ((uint32_t)data[offset + 2] << 8) + (uint32_t)data[offset + 3];
    }

    /** @copydoc ToInt32BE(const uint8_t*, int32_t) */
    static inline int32_t ToInt32BE(const std::vector<uint8_t>& data, int32_t offset) {
        return ((uint32_t)data[offset + 0] << 24) + ((uint32_t)data[offset + 1] << 16) +
               ((uint32_t)data[offset + 2] << 8) + (uint32_t)data[offset + 3];
    }

    /**
     * @brief Reads an unsigned 32-bit big-endian integer from @p data at @p offset.
     */
    static inline uint32_t ToUInt32BE(const uint8_t* data, int32_t offset) {
        return ((uint32_t)data[offset + 0] << 24) + ((uint32_t)data[offset + 1] << 16) +
               ((uint32_t)data[offset + 2] << 8) + (uint32_t)data[offset + 3];
    }

    /** @copydoc ToUInt32BE(const uint8_t*, int32_t) */
    static inline uint32_t ToUInt32BE(const std::vector<uint8_t>& data, int32_t offset) {
        return ((uint32_t)data[offset + 0] << 24) + ((uint32_t)data[offset + 1] << 16) +
               ((uint32_t)data[offset + 2] << 8) + (uint32_t)data[offset + 3];
    }

    /**
     * @brief Reads a signed 64-bit big-endian integer from @p data at @p offset.
     */
    static inline int64_t ToInt64BE(const uint8_t* data, int32_t offset) {
        return ((uint64_t)data[offset + 0] << 56) + ((uint64_t)data[offset + 1] << 48) +
               ((uint64_t)data[offset + 2] << 40) + ((uint64_t)data[offset + 3] << 32) +
               ((uint64_t)data[offset + 4] << 24) + ((uint64_t)data[offset + 5] << 16) +
               ((uint64_t)data[offset + 6] << 8) + ((uint64_t)data[offset + 7]);
    }

    /** @copydoc ToInt64BE(const uint8_t*, int32_t) */
    static inline int64_t ToInt64BE(const std::vector<uint8_t>& data, int32_t offset) {
        return ((uint64_t)data[offset + 0] << 56) + ((uint64_t)data[offset + 1] << 48) +
               ((uint64_t)data[offset + 2] << 40) + ((uint64_t)data[offset + 3] << 32) +
               ((uint64_t)data[offset + 4] << 24) + ((uint64_t)data[offset + 5] << 16) +
               ((uint64_t)data[offset + 6] << 8) + ((uint64_t)data[offset + 7]);
    }

    /**
     * @brief Reads an unsigned 64-bit big-endian integer from @p data at @p offset.
     */
    static inline uint64_t ToUInt64BE(const uint8_t* data, int32_t offset) {
        return ((uint64_t)data[offset + 0] << 56) + ((uint64_t)data[offset + 1] << 48) +
               ((uint64_t)data[offset + 2] << 40) + ((uint64_t)data[offset + 3] << 32) +
               ((uint64_t)data[offset + 4] << 24) + ((uint64_t)data[offset + 5] << 16) +
               ((uint64_t)data[offset + 6] << 8) + ((uint64_t)data[offset + 7]);
    }

    /** @copydoc ToUInt64BE(const uint8_t*, int32_t) */
    static inline uint64_t ToUInt64BE(const std::vector<uint8_t>& data, int32_t offset) {
        return ((uint64_t)data[offset + 0] << 56) + ((uint64_t)data[offset + 1] << 48) +
               ((uint64_t)data[offset + 2] << 40) + ((uint64_t)data[offset + 3] << 32) +
               ((uint64_t)data[offset + 4] << 24) + ((uint64_t)data[offset + 5] << 16) +
               ((uint64_t)data[offset + 6] << 8) + ((uint64_t)data[offset + 7]);
    }

    /**
     * @brief Reads a 32-bit big-endian IEEE 754 float from @p data at @p offset.
     *
     * The raw bytes are assembled in big-endian order and then reinterpreted as a
     * @c float via @c memcpy to avoid strict-aliasing violations.
     */
    static inline float ToFloatBE(const uint8_t* data, int32_t offset) {
        float value;
        uint32_t floatData = ((uint32_t)data[offset + 0] << 24) + ((uint32_t)data[offset + 1] << 16) +
                             ((uint32_t)data[offset + 2] << 8) + (uint32_t)data[offset + 3];
        static_assert(sizeof(uint32_t) == sizeof(float), "expected 32-bit float");
        std::memcpy(&value, &floatData, sizeof(value));
        return value;
    }

    /** @copydoc ToFloatBE(const uint8_t*, int32_t) */
    static inline float ToFloatBE(const std::vector<uint8_t>& data, int32_t offset) {
        float value;
        uint32_t floatData = ((uint32_t)data[offset + 0] << 24) + ((uint32_t)data[offset + 1] << 16) +
                             ((uint32_t)data[offset + 2] << 8) + (uint32_t)data[offset + 3];
        static_assert(sizeof(uint32_t) == sizeof(float), "expected 32-bit float");
        std::memcpy(&value, &floatData, sizeof(value));
        return value;
    }

    /**
     * @brief Reads a 64-bit big-endian IEEE 754 double from @p data at @p offset.
     *
     * Requires the host to use IEC 559 (IEEE 754) floating-point representation.
     */
    static inline double ToDoubleBE(const uint8_t* data, int32_t offset) {
        double value;
        uint64_t floatData = ((uint64_t)data[offset + 0] << 56) + ((uint64_t)data[offset + 1] << 48) +
                             ((uint64_t)data[offset + 2] << 40) + ((uint64_t)data[offset + 3] << 32) +
                             ((uint64_t)data[offset + 4] << 24) + ((uint64_t)data[offset + 5] << 16) +
                             ((uint64_t)data[offset + 6] << 8) + ((uint64_t)data[offset + 7]);
        static_assert(sizeof(uint64_t) == sizeof(double), "expected 64-bit double");
        // Checks if the float format on the platform the ZAPD binary is running on supports the
        // same float format as the object file.
        static_assert(std::numeric_limits<float>::is_iec559, "expected IEC559 floats on host machine");
        std::memcpy(&value, &floatData, sizeof(value));
        return value;
    }

    /** @copydoc ToDoubleBE(const uint8_t*, int32_t) */
    static inline double ToDoubleBE(const std::vector<uint8_t>& data, int32_t offset) {
        double value;
        uint64_t floatData = ((uint64_t)data[offset + 0] << 56) + ((uint64_t)data[offset + 1] << 48) +
                             ((uint64_t)data[offset + 2] << 40) + ((uint64_t)data[offset + 3] << 32) +
                             ((uint64_t)data[offset + 4] << 24) + ((uint64_t)data[offset + 5] << 16) +
                             ((uint64_t)data[offset + 6] << 8) + ((uint64_t)data[offset + 7]);
        static_assert(sizeof(uint64_t) == sizeof(double), "expected 64-bit double");
        // Checks if the float format on the platform the ZAPD binary is running on supports the
        // same float format as the object file.
        static_assert(std::numeric_limits<double>::is_iec559, "expected IEC559 doubles on host machine");
        std::memcpy(&value, &floatData, sizeof(value));
        return value;
    }

    /**
     * @brief Swaps the byte order of a ROM image in-place to the Z64 (big-endian) format.
     *
     * Detects the current byte order by examining the first byte of @p rom:
     * - @c 0x37 → V64 (byte-swapped 16-bit words): swaps every 16-bit word.
     * - @c 0x40 → N64 (little-endian 32-bit words): swaps every 32-bit word.
     * - @c 0x80 → Z64 (already big-endian): no operation.
     *
     * @param rom     Pointer to the ROM image buffer.
     * @param romSize Size of @p rom in bytes. If 0, the function is a no-op.
     */
    static inline void RomToBigEndian(uint8_t* rom, size_t romSize) {
        if (romSize <= 0) {
            return;
        }

        // Use the first byte to determine byte order
        uint8_t firstByte = rom[0];

        switch (firstByte) {
            case 0x37: // v64
                for (size_t pos = 0; pos < (romSize / 2); pos++) {
                    ((uint16_t*)rom)[pos] = ToUInt16BE(rom, pos * 2);
                }
                break;
            case 0x40: // n64
                for (size_t pos = 0; pos < (romSize / 4); pos++) {
                    ((uint32_t*)rom)[pos] = ToUInt32BE(rom, pos * 4);
                }
                break;
            case 0x80: // z64
                break; // Already BE, no need to swap
        }
    }
};
