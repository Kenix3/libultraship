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
