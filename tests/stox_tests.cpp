#include <gtest/gtest.h>
#include <climits>
#include "ship/utils/stox.h"

// ============================================================
// stob — string to bool
// ============================================================

TEST(StobTest, IntegerOneIsTrue) {
    EXPECT_TRUE(Ship::stob("1"));
}

TEST(StobTest, IntegerZeroIsFalse) {
    EXPECT_FALSE(Ship::stob("0"));
}

TEST(StobTest, BoolAlphaTrue) {
    EXPECT_TRUE(Ship::stob("true"));
}

TEST(StobTest, BoolAlphaFalse) {
    EXPECT_FALSE(Ship::stob("false"));
}

TEST(StobTest, InvalidStringReturnsDefaultTrue) {
    EXPECT_TRUE(Ship::stob("notabool", true));
}

TEST(StobTest, InvalidStringReturnsDefaultFalse) {
    EXPECT_FALSE(Ship::stob("notabool", false));
}

TEST(StobTest, EmptyStringReturnsDefault) {
    EXPECT_TRUE(Ship::stob("", true));
}

// ============================================================
// stoi — string to int32_t
// ============================================================

TEST(StoiTest, ParsesPositiveInt) {
    EXPECT_EQ(Ship::stoi("42"), 42);
}

TEST(StoiTest, ParsesNegativeInt) {
    EXPECT_EQ(Ship::stoi("-7"), -7);
}

TEST(StoiTest, ParsesZero) {
    EXPECT_EQ(Ship::stoi("0"), 0);
}

TEST(StoiTest, InvalidStringReturnsDefault) {
    EXPECT_EQ(Ship::stoi("abc", -99), -99);
}

TEST(StoiTest, EmptyStringReturnsDefault) {
    EXPECT_EQ(Ship::stoi("", 5), 5);
}

TEST(StoiTest, OutOfRangeReturnsDefault) {
    // Value larger than INT32_MAX
    EXPECT_EQ(Ship::stoi("99999999999999999", 0), 0);
}

// ============================================================
// stof — string to float
// ============================================================

TEST(StofTest, ParsesPositiveFloat) {
    EXPECT_FLOAT_EQ(Ship::stof("3.14"), 3.14f);
}

TEST(StofTest, ParsesNegativeFloat) {
    EXPECT_FLOAT_EQ(Ship::stof("-1.5"), -1.5f);
}

TEST(StofTest, ParsesZero) {
    EXPECT_FLOAT_EQ(Ship::stof("0.0"), 0.0f);
}

TEST(StofTest, InvalidStringReturnsDefault) {
    EXPECT_FLOAT_EQ(Ship::stof("xyz", 2.0f), 2.0f);
}

TEST(StofTest, EmptyStringReturnsDefault) {
    EXPECT_FLOAT_EQ(Ship::stof("", 1.0f), 1.0f);
}

// ============================================================
// stoll — string to int64_t
// ============================================================

TEST(StollTest, ParsesPositive) {
    EXPECT_EQ(Ship::stoll("1234567890123"), 1234567890123LL);
}

TEST(StollTest, ParsesNegative) {
    EXPECT_EQ(Ship::stoll("-9876543210"), -9876543210LL);
}

TEST(StollTest, ParsesZero) {
    EXPECT_EQ(Ship::stoll("0"), 0LL);
}

TEST(StollTest, InvalidStringReturnsDefault) {
    EXPECT_EQ(Ship::stoll("notanumber", -42LL), -42LL);
}

TEST(StollTest, EmptyStringReturnsDefault) {
    EXPECT_EQ(Ship::stoll("", 7LL), 7LL);
}
