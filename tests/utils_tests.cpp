#include <gtest/gtest.h>
#include "ship/utils/Utils.h"

// ============================================================
// Ship::Math::clamp
// ============================================================

TEST(MathClamp, ValueBelowMinClampsToMin) {
    EXPECT_FLOAT_EQ(Ship::Math::clamp(-10.0f, 0.0f, 1.0f), 0.0f);
}

TEST(MathClamp, ValueAboveMaxClampsToMax) {
    EXPECT_FLOAT_EQ(Ship::Math::clamp(5.0f, 0.0f, 1.0f), 1.0f);
}

TEST(MathClamp, ValueInRangeUnchanged) {
    EXPECT_FLOAT_EQ(Ship::Math::clamp(0.5f, 0.0f, 1.0f), 0.5f);
}

TEST(MathClamp, ValueAtMinEdge) {
    EXPECT_FLOAT_EQ(Ship::Math::clamp(0.0f, 0.0f, 1.0f), 0.0f);
}

TEST(MathClamp, ValueAtMaxEdge) {
    EXPECT_FLOAT_EQ(Ship::Math::clamp(1.0f, 0.0f, 1.0f), 1.0f);
}

TEST(MathClamp, NegativeRange) {
    EXPECT_FLOAT_EQ(Ship::Math::clamp(-3.0f, -5.0f, -1.0f), -3.0f);
}

// ============================================================
// Ship::Math::HashCombine
// ============================================================

TEST(MathHashCombine, SameInputGivesSameOutput) {
    size_t a = Ship::Math::HashCombine(42u, 100u);
    size_t b = Ship::Math::HashCombine(42u, 100u);
    EXPECT_EQ(a, b);
}

TEST(MathHashCombine, DifferentOrderedInputsAreDeterministic) {
    size_t a1 = Ship::Math::HashCombine(1u, 2u);
    size_t a2 = Ship::Math::HashCombine(1u, 2u);
    size_t b1 = Ship::Math::HashCombine(2u, 1u);
    size_t b2 = Ship::Math::HashCombine(2u, 1u);
    EXPECT_EQ(a1, a2);
    EXPECT_EQ(b1, b2);
}

TEST(MathHashCombine, ZeroAndZero) {
    // Just make sure it doesn't crash and produces a value
    size_t result = Ship::Math::HashCombine(0u, 0u);
    (void)result; // result value is implementation-defined
}

// ============================================================
// Ship::Math::IsNumber
// ============================================================

TEST(MathIsNumber, IntegerString) {
    EXPECT_TRUE(Ship::Math::IsNumber<int>("42"));
}

TEST(MathIsNumber, NegativeIntegerString) {
    EXPECT_TRUE(Ship::Math::IsNumber<int>("-7"));
}

TEST(MathIsNumber, FloatStringAsFloat) {
    EXPECT_TRUE(Ship::Math::IsNumber<float>("3.14"));
}

TEST(MathIsNumber, FloatStringAsInt) {
    EXPECT_FALSE(Ship::Math::IsNumber<int>("3.14"));
}

TEST(MathIsNumber, AlphaStringIsNotNumber) {
    EXPECT_FALSE(Ship::Math::IsNumber<int>("abc"));
}

// ============================================================
// Ship::toLowerCase
// ============================================================

TEST(ToLowerCase, AllUppercase) {
    EXPECT_EQ(Ship::toLowerCase("HELLO"), "hello");
}

TEST(ToLowerCase, MixedCase) {
    EXPECT_EQ(Ship::toLowerCase("HeLLo WoRLd"), "hello world");
}

TEST(ToLowerCase, AlreadyLowercase) {
    EXPECT_EQ(Ship::toLowerCase("already"), "already");
}

TEST(ToLowerCase, EmptyString) {
    EXPECT_EQ(Ship::toLowerCase(""), "");
}

TEST(ToLowerCase, WithDigitsAndSymbols) {
    EXPECT_EQ(Ship::toLowerCase("ABC123!"), "abc123!");
}
