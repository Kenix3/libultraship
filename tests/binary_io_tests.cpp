#include <gtest/gtest.h>
#include <cstring>
#include <stdexcept>

#include "ship/utils/binarytools/Stream.h"
#include "ship/utils/binarytools/BinaryReader.h"
#include "ship/utils/binarytools/BinaryWriter.h"
#include "ship/utils/binarytools/MemoryStream.h"

// Helper: creates a shared MemoryStream, writes to it via BinaryWriter,
// seeks back to the start, then returns a BinaryReader over the same stream.
static std::pair<std::shared_ptr<Ship::MemoryStream>, Ship::BinaryWriter> MakeWriter() {
    auto stream = std::make_shared<Ship::MemoryStream>();
    return { stream, Ship::BinaryWriter(stream) };
}

// ============================================================
// Round-trip tests (native endianness)
// ============================================================

TEST(BinaryRoundTrip, Int8) {
    auto [stream, writer] = MakeWriter();
    writer.Write(static_cast<int8_t>(-42));
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    EXPECT_EQ(reader.ReadInt8(), static_cast<int8_t>(-42));
}

TEST(BinaryRoundTrip, Int16) {
    auto [stream, writer] = MakeWriter();
    writer.Write(static_cast<int16_t>(0x1234));
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    EXPECT_EQ(reader.ReadInt16(), static_cast<int16_t>(0x1234));
}

TEST(BinaryRoundTrip, Int32) {
    auto [stream, writer] = MakeWriter();
    writer.Write(static_cast<int32_t>(0x12345678));
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    EXPECT_EQ(reader.ReadInt32(), static_cast<int32_t>(0x12345678));
}

TEST(BinaryRoundTrip, Int64) {
    auto [stream, writer] = MakeWriter();
    writer.Write(static_cast<int64_t>(0x0102030405060708LL));
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    EXPECT_EQ(reader.ReadInt64(), static_cast<int64_t>(0x0102030405060708LL));
}

TEST(BinaryRoundTrip, Float) {
    auto [stream, writer] = MakeWriter();
    writer.Write(1.5f);
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    EXPECT_FLOAT_EQ(reader.ReadFloat(), 1.5f);
}

TEST(BinaryRoundTrip, Double) {
    auto [stream, writer] = MakeWriter();
    writer.Write(3.14159265358979);
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    EXPECT_DOUBLE_EQ(reader.ReadDouble(), 3.14159265358979);
}

TEST(BinaryRoundTrip, String) {
    auto [stream, writer] = MakeWriter();
    writer.Write(std::string("hello"));
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    EXPECT_EQ(reader.ReadString(), "hello");
}

TEST(BinaryRoundTrip, EmptyString) {
    auto [stream, writer] = MakeWriter();
    writer.Write(std::string(""));
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    EXPECT_EQ(reader.ReadString(), "");
}

// ============================================================
// ReadCString
// ============================================================

TEST(BinaryReadCString, StopsAtNullTerminator) {
    auto [stream, writer] = MakeWriter();
    const char buf[] = "abc\0xyz";
    writer.Write(const_cast<char*>(buf), sizeof(buf));
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    // ReadCString reads up to and including the null terminator
    std::string result = reader.ReadCString();
    EXPECT_STREQ(result.c_str(), "abc");
}

TEST(BinaryReadCString, DoesNotOverreadAtEndOfStream) {
    auto [stream, writer] = MakeWriter();
    // Write bytes with no null terminator
    const char buf[] = { 'a', 'b', 'c' };
    writer.Write(const_cast<char*>(buf), sizeof(buf));
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    // Should stop at end of stream without crashing
    std::string result = reader.ReadCString();
    EXPECT_EQ(result.size(), 3u);
}

// ============================================================
// NaN throws
// ============================================================

TEST(BinaryReadFloat, NaNThrows) {
    // Write a quiet NaN bit pattern (IEEE 754: 0x7FC00000)
    uint32_t nanBits = 0x7FC00000;
    float nanFloat;
    std::memcpy(&nanFloat, &nanBits, sizeof(float));

    auto [stream, writer] = MakeWriter();
    writer.Write(nanFloat);
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    EXPECT_THROW(reader.ReadFloat(), std::runtime_error);
}

TEST(BinaryReadDouble, NaNThrows) {
    uint64_t nanBits = 0x7FF8000000000000ULL;
    double nanDouble;
    std::memcpy(&nanDouble, &nanBits, sizeof(double));

    auto [stream, writer] = MakeWriter();
    writer.Write(nanDouble);
    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    EXPECT_THROW(reader.ReadDouble(), std::runtime_error);
}

// ============================================================
// Endianness
// ============================================================

TEST(BinaryEndianness, WriteBigReadBigRoundTrips) {
    auto [stream, writer] = MakeWriter();
    writer.SetEndianness(Ship::Endianness::Big);
    writer.Write(static_cast<int32_t>(0x01020304));

    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    reader.SetEndianness(Ship::Endianness::Big);
    EXPECT_EQ(reader.ReadInt32(), static_cast<int32_t>(0x01020304));
}

TEST(BinaryEndianness, WriteBigReadNativeGivesByteSwappedValue) {
    // On a little-endian machine:
    //   Writing 0x01020304 as Big swaps to 0x04030201 on the wire.
    //   Reading as Native (LE) gives back 0x04030201.
    // On a big-endian machine both Native and Big are the same, so no swap occurs.
    auto [stream, writer] = MakeWriter();
    writer.SetEndianness(Ship::Endianness::Big);
    writer.Write(static_cast<int32_t>(0x01020304));

    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    reader.SetEndianness(Ship::Endianness::Native);
    int32_t result = reader.ReadInt32();

#ifdef IS_BIGENDIAN
    EXPECT_EQ(result, static_cast<int32_t>(0x01020304));
#else
    EXPECT_EQ(result, static_cast<int32_t>(0x04030201));
#endif
}

// ============================================================
// Seek
// ============================================================

TEST(BinarySeek, SeekFromStart) {
    auto [stream, writer] = MakeWriter();
    writer.Write(static_cast<int32_t>(0xAAAA));
    writer.Write(static_cast<int32_t>(0xBBBB));

    stream->Seek(4, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    EXPECT_EQ(reader.ReadInt32(), static_cast<int32_t>(0xBBBB));
}

TEST(BinarySeek, SeekToBeginningAndReread) {
    auto [stream, writer] = MakeWriter();
    writer.Write(static_cast<int32_t>(0xDEAD));

    stream->Seek(0, Ship::SeekOffsetType::Start);
    Ship::BinaryReader reader(stream);
    int32_t first = reader.ReadInt32();
    stream->Seek(0, Ship::SeekOffsetType::Start);
    int32_t second = reader.ReadInt32();
    EXPECT_EQ(first, second);
}
