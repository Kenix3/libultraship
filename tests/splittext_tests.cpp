#include <gtest/gtest.h>
#include "ship/utils/Utils.h"

// ============================================================
// splitText — basic space separation
// ============================================================

TEST(SplitText, SingleToken) {
    auto v = Ship::splitText("hello", ' ', false);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], "hello");
}

TEST(SplitText, TwoSpaceSeparatedTokens) {
    auto v = Ship::splitText("foo bar", ' ', false);
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], "foo");
    EXPECT_EQ(v[1], "bar");
}

TEST(SplitText, MultipleTokens) {
    auto v = Ship::splitText("a b c d", ' ', false);
    ASSERT_EQ(v.size(), 4u);
    EXPECT_EQ(v[0], "a");
    EXPECT_EQ(v[3], "d");
}

TEST(SplitText, EmptyStringReturnsEmpty) {
    auto v = Ship::splitText("", ' ', false);
    EXPECT_TRUE(v.empty());
}

TEST(SplitText, OnlySeparatorsReturnsEmpty) {
    auto v = Ship::splitText("   ", ' ', false);
    EXPECT_TRUE(v.empty());
}

TEST(SplitText, LeadingSeparatorIgnored) {
    auto v = Ship::splitText(" hello", ' ', false);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], "hello");
}

TEST(SplitText, TrailingSeparatorIgnored) {
    auto v = Ship::splitText("hello ", ' ', false);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], "hello");
}

TEST(SplitText, AdjacentSeparatorsIgnored) {
    auto v = Ship::splitText("a  b", ' ', false);
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], "a");
    EXPECT_EQ(v[1], "b");
}

// ============================================================
// splitText — custom separator
// ============================================================

TEST(SplitText, CommaSeparator) {
    auto v = Ship::splitText("a,b,c", ',', false);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], "a");
    EXPECT_EQ(v[1], "b");
    EXPECT_EQ(v[2], "c");
}

TEST(SplitText, SlashSeparator) {
    auto v = Ship::splitText("textures/link/idle", '/', false);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], "textures");
    EXPECT_EQ(v[1], "link");
    EXPECT_EQ(v[2], "idle");
}

TEST(SplitText, SeparatorNotPresentReturnsSingleToken) {
    auto v = Ship::splitText("hello", ',', false);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], "hello");
}

// ============================================================
// splitText — quoted strings (quotes stripped, keepQuotes=false)
// ============================================================

TEST(SplitText, QuotedStringWithSpaceIsSingleToken) {
    auto v = Ship::splitText(R"("hello world")", ' ', false);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], "hello world");
}

TEST(SplitText, QuotedStringQuotesStrippedByDefault) {
    auto v = Ship::splitText(R"("foo bar" baz)", ' ', false);
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], "foo bar");
    EXPECT_EQ(v[1], "baz");
}

TEST(SplitText, UnquotedTokensMixedWithQuoted) {
    auto v = Ship::splitText(R"(alpha "beta gamma" delta)", ' ', false);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], "alpha");
    EXPECT_EQ(v[1], "beta gamma");
    EXPECT_EQ(v[2], "delta");
}

// ============================================================
// splitText — keepQuotes=true
// ============================================================

TEST(SplitText, KeepQuotesTruePreservesQuotes) {
    auto v = Ship::splitText(R"("hello world")", ' ', true);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], R"("hello world")");
}
