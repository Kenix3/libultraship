#include <gtest/gtest.h>
#include "ship/utils/glob.h"

// ============================================================
// Exact matches
// ============================================================

TEST(GlobMatch, ExactMatchReturnsTrue) {
    EXPECT_TRUE(glob_match("hello", "hello"));
}

TEST(GlobMatch, ExactMatchEmptyBoth) {
    EXPECT_TRUE(glob_match("", ""));
}

TEST(GlobMatch, ExactMismatchReturnsFalse) {
    EXPECT_FALSE(glob_match("hello", "world"));
}

TEST(GlobMatch, PatternLongerThanStringFalse) {
    EXPECT_FALSE(glob_match("hello!", "hello"));
}

TEST(GlobMatch, StringLongerThanPatternFalse) {
    EXPECT_FALSE(glob_match("hello", "helloworld"));
}

// ============================================================
// * wildcard
// ============================================================

TEST(GlobMatch, StarAloneMatchesEmpty) {
    EXPECT_TRUE(glob_match("*", ""));
}

TEST(GlobMatch, StarAloneMatchesSingleChar) {
    EXPECT_TRUE(glob_match("*", "a"));
}

TEST(GlobMatch, StarAloneMatchesLongString) {
    EXPECT_TRUE(glob_match("*", "textures/player/link.bin"));
}

TEST(GlobMatch, StarAtEndMatchesSuffix) {
    EXPECT_TRUE(glob_match("textures/*", "textures/link.bin"));
    EXPECT_TRUE(glob_match("textures/*", "textures/"));
    EXPECT_FALSE(glob_match("textures/*", "audio/bgm.bin"));
}

TEST(GlobMatch, StarAtStartMatchesPrefix) {
    EXPECT_TRUE(glob_match("*.bin", "link.bin"));
    EXPECT_TRUE(glob_match("*.bin", "path/to/link.bin"));
    EXPECT_FALSE(glob_match("*.bin", "link.txt"));
}

TEST(GlobMatch, StarInMiddle) {
    EXPECT_TRUE(glob_match("tex*/link.bin", "textures/link.bin"));
    EXPECT_TRUE(glob_match("tex*/link.bin", "tex/link.bin"));
    EXPECT_FALSE(glob_match("tex*/link.bin", "audio/link.bin"));
}

TEST(GlobMatch, MultipleStars) {
    EXPECT_TRUE(glob_match("*/*/*.bin", "a/b/c.bin"));
    EXPECT_FALSE(glob_match("*/*/*.bin", "a/b.bin"));
}

TEST(GlobMatch, StarMatchesZeroCharacters) {
    EXPECT_TRUE(glob_match("ab*cd", "abcd"));
    EXPECT_TRUE(glob_match("ab*cd", "abXcd"));
    EXPECT_TRUE(glob_match("ab*cd", "abXYZcd"));
}

TEST(GlobMatch, NonEmptyPatternDoesNotMatchEmptyString) {
    EXPECT_FALSE(glob_match("a*", ""));
    EXPECT_FALSE(glob_match("?", ""));
}

// ============================================================
// ? wildcard
// ============================================================

TEST(GlobMatch, QuestionMatchesSingleChar) {
    EXPECT_TRUE(glob_match("?", "a"));
    EXPECT_TRUE(glob_match("?", "Z"));
    EXPECT_TRUE(glob_match("?", "3"));
}

TEST(GlobMatch, QuestionDoesNotMatchEmpty) {
    EXPECT_FALSE(glob_match("?", ""));
}

TEST(GlobMatch, QuestionDoesNotMatchTwoChars) {
    EXPECT_FALSE(glob_match("?", "ab"));
}

TEST(GlobMatch, MultipleQuestions) {
    EXPECT_TRUE(glob_match("???", "abc"));
    EXPECT_FALSE(glob_match("???", "ab"));
    EXPECT_FALSE(glob_match("???", "abcd"));
}

TEST(GlobMatch, QuestionCombinedWithLiterals) {
    EXPECT_TRUE(glob_match("l?nk", "link"));
    EXPECT_TRUE(glob_match("l?nk", "lunk"));
    EXPECT_FALSE(glob_match("l?nk", "lnk"));
    EXPECT_FALSE(glob_match("l?nk", "loonk"));
}

// ============================================================
// [abc] character class
// ============================================================

TEST(GlobMatch, CharClassMatchesMember) {
    EXPECT_TRUE(glob_match("[abc]", "a"));
    EXPECT_TRUE(glob_match("[abc]", "b"));
    EXPECT_TRUE(glob_match("[abc]", "c"));
}

TEST(GlobMatch, CharClassNoMatchNonMember) {
    EXPECT_FALSE(glob_match("[abc]", "d"));
    EXPECT_FALSE(glob_match("[abc]", "A"));
}

TEST(GlobMatch, CharClassInPattern) {
    EXPECT_TRUE(glob_match("l[io]nk", "link"));
    EXPECT_TRUE(glob_match("l[io]nk", "lonk"));
    EXPECT_FALSE(glob_match("l[io]nk", "lunk"));
}

// ============================================================
// [a-z] range in character class
// ============================================================

TEST(GlobMatch, RangeClassMatchesInRange) {
    EXPECT_TRUE(glob_match("[a-z]", "a"));
    EXPECT_TRUE(glob_match("[a-z]", "m"));
    EXPECT_TRUE(glob_match("[a-z]", "z"));
}

TEST(GlobMatch, RangeClassNoMatchOutsideRange) {
    EXPECT_FALSE(glob_match("[a-z]", "A"));
    EXPECT_FALSE(glob_match("[a-z]", "0"));
}

TEST(GlobMatch, RangeClassDigits) {
    EXPECT_TRUE(glob_match("[0-9]", "5"));
    EXPECT_FALSE(glob_match("[0-9]", "a"));
}

// ============================================================
// [!abc] inverted character class
// ============================================================

TEST(GlobMatch, InvertedClassNoMatchMember) {
    EXPECT_FALSE(glob_match("[!abc]", "a"));
    EXPECT_FALSE(glob_match("[!abc]", "b"));
}

TEST(GlobMatch, InvertedClassMatchesNonMember) {
    EXPECT_TRUE(glob_match("[!abc]", "d"));
    EXPECT_TRUE(glob_match("[!abc]", "Z"));
}

TEST(GlobMatch, InvertedRangeClass) {
    EXPECT_FALSE(glob_match("[!a-z]", "m"));
    EXPECT_TRUE(glob_match("[!a-z]", "A"));
    EXPECT_TRUE(glob_match("[!a-z]", "3"));
}

// ============================================================
// \ escape
// ============================================================

TEST(GlobMatch, BackslashEscapesStar) {
    EXPECT_TRUE(glob_match("a\\*b", "a*b"));
    EXPECT_FALSE(glob_match("a\\*b", "axb"));
}

TEST(GlobMatch, BackslashEscapesQuestion) {
    EXPECT_TRUE(glob_match("a\\?b", "a?b"));
    EXPECT_FALSE(glob_match("a\\?b", "axb"));
}

// ============================================================
// Combined / realistic patterns
// ============================================================

TEST(GlobMatch, ExtensionFilter) {
    EXPECT_TRUE(glob_match("*.[ch]", "main.c"));
    EXPECT_TRUE(glob_match("*.[ch]", "main.h"));
    EXPECT_FALSE(glob_match("*.[ch]", "main.cpp"));
}

TEST(GlobMatch, DirectoryAndExtension) {
    EXPECT_TRUE(glob_match("textures/*.bin", "textures/link.bin"));
    EXPECT_FALSE(glob_match("textures/*.bin", "audio/theme.bin"));
    EXPECT_FALSE(glob_match("textures/*.bin", "textures/link.png"));
}

TEST(GlobMatch, PrefixSuffixWithWildcard) {
    EXPECT_TRUE(glob_match("link_*_hero", "link_the_hero"));
    EXPECT_TRUE(glob_match("link_*_hero", "link__hero"));
    EXPECT_FALSE(glob_match("link_*_hero", "link_hero"));
}

TEST(GlobMatch, AnchoredToEntireString) {
    // Pattern must match the full string, not a substring
    EXPECT_FALSE(glob_match("lin", "link"));
    EXPECT_FALSE(glob_match("ink", "link"));
    EXPECT_TRUE(glob_match("link", "link"));
}
