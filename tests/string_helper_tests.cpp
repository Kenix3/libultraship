#include <gtest/gtest.h>
#include "ship/utils/StringHelper.h"

// ============================================================
// Split
// ============================================================

TEST(StringHelperSplit, BasicDelimiter) {
    std::string input = "a.b.c";
    auto parts = StringHelper::Split(input, ".");
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST(StringHelperSplit, NoDelimiterPresent) {
    std::string input = "abc";
    auto parts = StringHelper::Split(input, ".");
    ASSERT_EQ(parts.size(), 1u);
    EXPECT_EQ(parts[0], "abc");
}

TEST(StringHelperSplit, MultiCharDelimiter) {
    std::string input = "foo::bar::baz";
    auto parts = StringHelper::Split(input, "::");
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[1], "bar");
}

TEST(StringHelperSplit, EmptyString) {
    std::string input = "";
    auto parts = StringHelper::Split(input, ".");
    ASSERT_EQ(parts.size(), 1u);
    EXPECT_EQ(parts[0], "");
}

// ============================================================
// Strip
// ============================================================

TEST(StringHelperStrip, RemovesAllOccurrences) {
    EXPECT_EQ(StringHelper::Strip("aXbXc", "X"), "abc");
}

TEST(StringHelperStrip, NoOccurrences) {
    EXPECT_EQ(StringHelper::Strip("abc", "X"), "abc");
}

// ============================================================
// Replace
// ============================================================

TEST(StringHelperReplace, ReplacesAll) {
    EXPECT_EQ(StringHelper::Replace("hello world world", "world", "LUS"), "hello LUS LUS");
}

TEST(StringHelperReplace, NoMatch) {
    EXPECT_EQ(StringHelper::Replace("hello", "xyz", "abc"), "hello");
}

// ============================================================
// StartsWith / EndsWith / Contains
// ============================================================

TEST(StringHelperStartsWith, BasicTrue) {
    EXPECT_TRUE(StringHelper::StartsWith("hello world", "hello"));
}

TEST(StringHelperStartsWith, BasicFalse) {
    EXPECT_FALSE(StringHelper::StartsWith("hello world", "world"));
}

TEST(StringHelperStartsWith, PrefixLongerThanString) {
    EXPECT_FALSE(StringHelper::StartsWith("hi", "hello"));
}

TEST(StringHelperEndsWith, BasicTrue) {
    EXPECT_TRUE(StringHelper::EndsWith("hello world", "world"));
}

TEST(StringHelperEndsWith, BasicFalse) {
    EXPECT_FALSE(StringHelper::EndsWith("hello world", "hello"));
}

TEST(StringHelperContains, BasicTrue) {
    EXPECT_TRUE(StringHelper::Contains("hello world", "lo wo"));
}

TEST(StringHelperContains, BasicFalse) {
    EXPECT_FALSE(StringHelper::Contains("hello world", "xyz"));
}

// ============================================================
// IEquals
// ============================================================

TEST(StringHelperIEquals, SameCase) {
    EXPECT_TRUE(StringHelper::IEquals("hello", "hello"));
}

TEST(StringHelperIEquals, DifferentCase) {
    EXPECT_TRUE(StringHelper::IEquals("Hello", "hELLO"));
}

TEST(StringHelperIEquals, NotEqual) {
    EXPECT_FALSE(StringHelper::IEquals("hello", "world"));
}

// ============================================================
// IsValidHex
// ============================================================

TEST(StringHelperIsValidHex, ValidLowercase) {
    std::string hex = "0x1a2b";
    EXPECT_TRUE(StringHelper::IsValidHex(hex));
}

TEST(StringHelperIsValidHex, ValidUppercase) {
    std::string hex = "0X1A2B";
    EXPECT_TRUE(StringHelper::IsValidHex(hex));
}

TEST(StringHelperIsValidHex, TooShort) {
    std::string hex = "0x";
    EXPECT_FALSE(StringHelper::IsValidHex(hex));
}

TEST(StringHelperIsValidHex, NoPrefix) {
    std::string hex = "1A2B";
    EXPECT_FALSE(StringHelper::IsValidHex(hex));
}

TEST(StringHelperIsValidHex, InvalidChars) {
    std::string hex = "0xGG";
    EXPECT_FALSE(StringHelper::IsValidHex(hex));
}

// ============================================================
// IsValidOffset
// ============================================================

TEST(StringHelperIsValidOffset, ZeroIsValid) {
    std::string offset = "0";
    EXPECT_TRUE(StringHelper::IsValidOffset(offset));
}

TEST(StringHelperIsValidOffset, HexIsValid) {
    std::string offset = "0x1000";
    EXPECT_TRUE(StringHelper::IsValidOffset(offset));
}

TEST(StringHelperIsValidOffset, BareNonZeroDigitIsValid) {
    std::string offset = "5";
    EXPECT_TRUE(StringHelper::IsValidOffset(offset));
}

// ============================================================
// BoolStr
// ============================================================

TEST(StringHelperBoolStr, True) {
    EXPECT_EQ(StringHelper::BoolStr(true), "true");
}

TEST(StringHelperBoolStr, False) {
    EXPECT_EQ(StringHelper::BoolStr(false), "false");
}

// ============================================================
// Implode
// ============================================================

TEST(StringHelperImplode, JoinsWithSeparator) {
    std::vector<std::string> v = { "a", "b", "c" };
    EXPECT_EQ(StringHelper::Implode(v, ","), "a,b,c");
}

TEST(StringHelperImplode, EmptyVector) {
    std::vector<std::string> v = {};
    EXPECT_EQ(StringHelper::Implode(v, ","), "");
}

TEST(StringHelperImplode, SingleElement) {
    std::vector<std::string> v = { "a" };
    EXPECT_EQ(StringHelper::Implode(v, ","), "a");
}

// ============================================================
// ReplaceOriginal
// ============================================================

TEST(StringHelperReplaceOriginal, ReplacesInPlace) {
    std::string s = "hello world world";
    StringHelper::ReplaceOriginal(s, "world", "LUS");
    EXPECT_EQ(s, "hello LUS LUS");
}

TEST(StringHelperReplaceOriginal, NoMatchUnchanged) {
    std::string s = "hello";
    StringHelper::ReplaceOriginal(s, "xyz", "abc");
    EXPECT_EQ(s, "hello");
}

// ============================================================
// Sprintf
// ============================================================

TEST(StringHelperSprintf, FormatsInteger) {
    std::string result = StringHelper::Sprintf("%d", 42);
    EXPECT_EQ(result, "42");
}

TEST(StringHelperSprintf, FormatsString) {
    std::string result = StringHelper::Sprintf("hello %s", "world");
    EXPECT_EQ(result, "hello world");
}

TEST(StringHelperSprintf, FormatsMultipleArgs) {
    std::string result = StringHelper::Sprintf("%d + %d = %d", 1, 2, 3);
    EXPECT_EQ(result, "1 + 2 = 3");
}

// ============================================================
// StrToL
// ============================================================

TEST(StringHelperStrToL, DecimalBase) {
    EXPECT_EQ(StringHelper::StrToL("255", 10), 255LL);
}

TEST(StringHelperStrToL, HexBase) {
    EXPECT_EQ(StringHelper::StrToL("ff", 16), 255LL);
}

TEST(StringHelperStrToL, OctalBase) {
    EXPECT_EQ(StringHelper::StrToL("10", 8), 8LL);
}

TEST(StringHelperStrToL, AutoDetectHexPrefix) {
    // With base=16, strtoull accepts hexadecimal input with an optional 0x prefix.
    EXPECT_EQ(StringHelper::StrToL("0xff", 16), 255LL);
}

// ============================================================
// HasOnlyDigits
// ============================================================

TEST(StringHelperHasOnlyDigits, AllDigits) {
    EXPECT_TRUE(StringHelper::HasOnlyDigits("1234567890"));
}

TEST(StringHelperHasOnlyDigits, ContainsLetter) {
    EXPECT_FALSE(StringHelper::HasOnlyDigits("123a"));
}

TEST(StringHelperHasOnlyDigits, EmptyString) {
    EXPECT_TRUE(StringHelper::HasOnlyDigits(""));
}

TEST(StringHelperHasOnlyDigits, WithSpaces) {
    EXPECT_FALSE(StringHelper::HasOnlyDigits("12 34"));
}

// ============================================================
// HexToBytes / BytesToHex
// ============================================================

TEST(StringHelperHexToBytes, BasicConversion) {
    auto bytes = StringHelper::HexToBytes("deadbeef");
    ASSERT_EQ(bytes.size(), 4u);
    EXPECT_EQ(bytes[0], 0xDE);
    EXPECT_EQ(bytes[1], 0xAD);
    EXPECT_EQ(bytes[2], 0xBE);
    EXPECT_EQ(bytes[3], 0xEF);
}

TEST(StringHelperHexToBytes, SingleByte) {
    auto bytes = StringHelper::HexToBytes("ff");
    ASSERT_EQ(bytes.size(), 1u);
    EXPECT_EQ(bytes[0], 0xFF);
}

TEST(StringHelperBytesToHex, BasicConversion) {
    std::vector<unsigned char> bytes = { 0xDE, 0xAD, 0xBE, 0xEF };
    EXPECT_EQ(StringHelper::BytesToHex(bytes), "deadbeef");
}

TEST(StringHelperBytesToHex, EmptyVector) {
    std::vector<unsigned char> bytes = {};
    EXPECT_EQ(StringHelper::BytesToHex(bytes), "");
}

TEST(StringHelperBytesToHex, SingleByte) {
    std::vector<unsigned char> bytes = { 0x0A };
    EXPECT_EQ(StringHelper::BytesToHex(bytes), "0a");
}

TEST(StringHelperHexRoundTrip, BytesToHexAndBack) {
    std::vector<unsigned char> original = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
    std::string hex = StringHelper::BytesToHex(original);
    auto recovered = StringHelper::HexToBytes(hex);
    ASSERT_EQ(recovered.size(), original.size());
    for (size_t i = 0; i < original.size(); i++) {
        EXPECT_EQ(recovered[i], original[i]);
    }
}

// ============================================================
// Split (string_view overload)
// ============================================================

TEST(StringHelperSplitView, BasicDelimiter) {
    std::string_view input = "a.b.c";
    auto parts = StringHelper::Split(input, ".");
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST(StringHelperSplitView, NoDelimiter) {
    std::string_view input = "abc";
    auto parts = StringHelper::Split(input, ".");
    ASSERT_EQ(parts.size(), 1u);
    EXPECT_EQ(parts[0], "abc");
}

TEST(StringHelperSplitView, MultiCharDelimiter) {
    std::string_view input = "foo::bar::baz";
    auto parts = StringHelper::Split(input, "::");
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[1], "bar");
}

// ============================================================
// EndsWith edge cases
// ============================================================

TEST(StringHelperEndsWith, SuffixLongerThanString) {
    EXPECT_FALSE(StringHelper::EndsWith("hi", "hello world"));
}

TEST(StringHelperEndsWith, EmptySuffix) {
    // An empty suffix is always a suffix
    EXPECT_TRUE(StringHelper::EndsWith("hello", ""));
}
