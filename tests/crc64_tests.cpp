#include <gtest/gtest.h>
#include "ship/utils/StrHash64.h"

// ============================================================
// CRC64 — null-terminated string interface
// ============================================================

TEST(CRC64String, EmptyStringIsInitialCRC) {
    // CRC64() does NOT apply ~ finalization (unlike crc64()/update_crc64()).
    // For an empty string the loop never runs, returning INITIAL_CRC64 unchanged.
    EXPECT_EQ(CRC64(""), INITIAL_CRC64);
}

TEST(CRC64String, Deterministic) {
    EXPECT_EQ(CRC64("hello"), CRC64("hello"));
}

TEST(CRC64String, DifferentStringsAreDifferent) {
    EXPECT_NE(CRC64("foo"), CRC64("bar"));
}

TEST(CRC64String, CaseSensitive) {
    EXPECT_NE(CRC64("Hello"), CRC64("hello"));
}

TEST(CRC64String, TrailingSpaceDiffers) {
    EXPECT_NE(CRC64("foo"), CRC64("foo "));
}

TEST(CRC64String, LongerStringDiffers) {
    EXPECT_NE(CRC64("textures/link"), CRC64("textures/link2"));
}

// ============================================================
// crc64 — buffer + length interface
// Note: crc64()/update_crc64() apply ~ finalization; CRC64() does NOT.
// They are not equivalent and should not be compared directly.
// ============================================================

TEST(CRC64Buffer, LengthZeroReturnsFinalized) {
    // crc64 with length 0: loop doesn't run, update_crc64 returns ~INITIAL_CRC64 = 0
    EXPECT_EQ(crc64("ignored", 0), 0ULL);
}

TEST(CRC64Buffer, Deterministic) {
    const char* s = "hello";
    uint32_t len = static_cast<uint32_t>(strlen(s));
    EXPECT_EQ(crc64(s, len), crc64(s, len));
}

TEST(CRC64Buffer, CanHashBinaryData) {
    const uint8_t buf1[] = { 0x00, 0x01, 0x02, 0x03 };
    const uint8_t buf2[] = { 0x00, 0x01, 0x02, 0x04 };
    uint64_t h1 = crc64(buf1, sizeof(buf1));
    uint64_t h2 = crc64(buf2, sizeof(buf2));
    EXPECT_NE(h1, h2);
}

// ============================================================
// update_crc64 — incremental interface
// ============================================================

TEST(CRC64Incremental, ExplicitInitialValueMatchesCrc64) {
    // crc64(buf, len) is defined as update_crc64(buf, len, INITIAL_CRC64).
    // Passing INITIAL_CRC64 explicitly should produce the same result.
    const char* s = "helloworld";
    uint32_t len = static_cast<uint32_t>(strlen(s));
    EXPECT_EQ(update_crc64(s, len, INITIAL_CRC64), crc64(s, len));
}

TEST(CRC64Incremental, DifferentInitialValueGivesDifferentResult) {
    const char* s = "hello";
    uint32_t len = static_cast<uint32_t>(strlen(s));
    EXPECT_NE(update_crc64(s, len, 0ULL), update_crc64(s, len, INITIAL_CRC64));
}
