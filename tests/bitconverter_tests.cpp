#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "ship/utils/binarytools/BitConverter.h"

// ============================================================
// Helper to build a raw byte array
// ============================================================

static std::vector<uint8_t> Bytes(std::initializer_list<uint8_t> il) {
    return std::vector<uint8_t>(il);
}

// ============================================================
// ToInt8BE / ToUInt8BE
// ============================================================

TEST(BitConverterInt8, PointerOverload) {
    auto data = Bytes({ 0xAB, 0x00 });
    EXPECT_EQ(BitConverter::ToInt8BE(data.data(), 0), static_cast<int8_t>(0xAB));
}

TEST(BitConverterInt8, VectorOverload) {
    auto data = Bytes({ 0x7F, 0x00 });
    EXPECT_EQ(BitConverter::ToInt8BE(data, 0), static_cast<int8_t>(0x7F));
}

TEST(BitConverterUInt8, PointerOverload) {
    auto data = Bytes({ 0xFF, 0x00 });
    EXPECT_EQ(BitConverter::ToUInt8BE(data.data(), 0), static_cast<uint8_t>(0xFF));
}

TEST(BitConverterUInt8, VectorOverload) {
    auto data = Bytes({ 0x42, 0x00 });
    EXPECT_EQ(BitConverter::ToUInt8BE(data, 0), static_cast<uint8_t>(0x42));
}

// ============================================================
// ToInt16BE / ToUInt16BE
// ============================================================

TEST(BitConverterInt16, PointerOverload) {
    // 0x01, 0x02 in BE → 0x0102
    auto data = Bytes({ 0x01, 0x02 });
    EXPECT_EQ(BitConverter::ToInt16BE(data.data(), 0), static_cast<int16_t>(0x0102));
}

TEST(BitConverterInt16, VectorOverload) {
    auto data = Bytes({ 0xFF, 0xFE });
    // 0xFFFE as int16 = -2
    EXPECT_EQ(BitConverter::ToInt16BE(data, 0), static_cast<int16_t>(-2));
}

TEST(BitConverterUInt16, PointerOverload) {
    auto data = Bytes({ 0xAB, 0xCD });
    EXPECT_EQ(BitConverter::ToUInt16BE(data.data(), 0), static_cast<uint16_t>(0xABCD));
}

TEST(BitConverterUInt16, VectorOverload) {
    auto data = Bytes({ 0x00, 0x01 });
    EXPECT_EQ(BitConverter::ToUInt16BE(data, 0), static_cast<uint16_t>(0x0001));
}

TEST(BitConverterInt16, NonZeroOffset) {
    auto data = Bytes({ 0x00, 0x01, 0x02 });
    EXPECT_EQ(BitConverter::ToInt16BE(data.data(), 1), static_cast<int16_t>(0x0102));
}

// ============================================================
// ToInt32BE / ToUInt32BE
// ============================================================

TEST(BitConverterInt32, PointerOverload) {
    auto data = Bytes({ 0x01, 0x02, 0x03, 0x04 });
    EXPECT_EQ(BitConverter::ToInt32BE(data.data(), 0), static_cast<int32_t>(0x01020304));
}

TEST(BitConverterInt32, VectorOverload) {
    auto data = Bytes({ 0xFF, 0xFF, 0xFF, 0xFF });
    EXPECT_EQ(BitConverter::ToInt32BE(data, 0), static_cast<int32_t>(-1));
}

TEST(BitConverterUInt32, PointerOverload) {
    auto data = Bytes({ 0xDE, 0xAD, 0xBE, 0xEF });
    EXPECT_EQ(BitConverter::ToUInt32BE(data.data(), 0), static_cast<uint32_t>(0xDEADBEEF));
}

TEST(BitConverterUInt32, VectorOverload) {
    auto data = Bytes({ 0x00, 0x00, 0x00, 0x01 });
    EXPECT_EQ(BitConverter::ToUInt32BE(data, 0), static_cast<uint32_t>(1));
}

TEST(BitConverterInt32, NonZeroOffset) {
    auto data = Bytes({ 0xFF, 0x01, 0x02, 0x03, 0x04 });
    EXPECT_EQ(BitConverter::ToInt32BE(data.data(), 1), static_cast<int32_t>(0x01020304));
}

// ============================================================
// ToInt64BE / ToUInt64BE
// ============================================================

TEST(BitConverterInt64, PointerOverload) {
    auto data = Bytes({ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 });
    EXPECT_EQ(BitConverter::ToInt64BE(data.data(), 0), static_cast<int64_t>(0x0102030405060708LL));
}

TEST(BitConverterInt64, VectorOverload) {
    auto data = Bytes({ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
    EXPECT_EQ(BitConverter::ToInt64BE(data, 0), static_cast<int64_t>(-1LL));
}

TEST(BitConverterUInt64, PointerOverload) {
    auto data = Bytes({ 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE });
    EXPECT_EQ(BitConverter::ToUInt64BE(data.data(), 0), static_cast<uint64_t>(0xDEADBEEFCAFEBABEULL));
}

TEST(BitConverterUInt64, VectorOverload) {
    auto data = Bytes({ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 });
    EXPECT_EQ(BitConverter::ToUInt64BE(data, 0), static_cast<uint64_t>(1));
}

// ============================================================
// ToFloatBE
// ============================================================

TEST(BitConverterFloat, PointerOverload) {
    // IEEE 754 BE representation of 1.0f: 0x3F800000
    auto data = Bytes({ 0x3F, 0x80, 0x00, 0x00 });
    EXPECT_FLOAT_EQ(BitConverter::ToFloatBE(data.data(), 0), 1.0f);
}

TEST(BitConverterFloat, VectorOverload) {
    // IEEE 754 BE representation of -1.0f: 0xBF800000
    auto data = Bytes({ 0xBF, 0x80, 0x00, 0x00 });
    EXPECT_FLOAT_EQ(BitConverter::ToFloatBE(data, 0), -1.0f);
}

// ============================================================
// ToDoubleBE
// ============================================================

TEST(BitConverterDouble, PointerOverload) {
    // IEEE 754 BE representation of 1.0: 0x3FF0000000000000
    auto data = Bytes({ 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
    EXPECT_DOUBLE_EQ(BitConverter::ToDoubleBE(data.data(), 0), 1.0);
}

TEST(BitConverterDouble, VectorOverload) {
    // IEEE 754 BE representation of -1.0: 0xBFF0000000000000
    auto data = Bytes({ 0xBF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
    EXPECT_DOUBLE_EQ(BitConverter::ToDoubleBE(data, 0), -1.0);
}

// ============================================================
// RomToBigEndian
// ============================================================

TEST(BitConverterRomToBigEndian, EmptyRomIsNoOp) {
    uint8_t rom[] = { 0x80 };
    // romSize=0 should return without touching any bytes
    BitConverter::RomToBigEndian(rom, 0);
    EXPECT_EQ(rom[0], 0x80); // unchanged
}

TEST(BitConverterRomToBigEndian, AlreadyBigEndianUnchanged) {
    // First byte 0x80 → already z64 (big-endian), no swap needed
    uint8_t rom[] = { 0x80, 0x37, 0x12, 0x40 };
    BitConverter::RomToBigEndian(rom, sizeof(rom));
    EXPECT_EQ(rom[0], 0x80);
    EXPECT_EQ(rom[1], 0x37);
    EXPECT_EQ(rom[2], 0x12);
    EXPECT_EQ(rom[3], 0x40);
}

TEST(BitConverterRomToBigEndian, V64FormatSwapsUInt16Pairs) {
    // First byte 0x37 → v64 format: each pair of bytes is swapped
    // Original: { 0x37, 0x80, 0x01, 0x02 }
    // After swap of 16-bit pairs: { 0x80, 0x37, 0x02, 0x01 }
    uint8_t rom[] = { 0x37, 0x80, 0x01, 0x02 };
    BitConverter::RomToBigEndian(rom, sizeof(rom));
    EXPECT_EQ(rom[0], 0x80);
    EXPECT_EQ(rom[1], 0x37);
    EXPECT_EQ(rom[2], 0x02);
    EXPECT_EQ(rom[3], 0x01);
}

TEST(BitConverterRomToBigEndian, N64FormatSwapsUInt32Words) {
    // First byte 0x40 → n64 format: each group of 4 bytes is byte-reversed
    // Original: { 0x40, 0x12, 0x34, 0x56 }
    // After conversion to big-endian byte order, rom contains:
    // { 0x56, 0x34, 0x12, 0x40 }.
    uint8_t rom[] = { 0x40, 0x12, 0x34, 0x56 };
    BitConverter::RomToBigEndian(rom, sizeof(rom));
    // We memcpy those bytes into a native uint32_t; on little-endian hosts,
    // bytes { 0x56, 0x34, 0x12, 0x40 } correspond to integer 0x40123456.
    uint32_t stored;
    std::memcpy(&stored, rom, 4);
    EXPECT_EQ(stored, static_cast<uint32_t>(0x40123456));
}
