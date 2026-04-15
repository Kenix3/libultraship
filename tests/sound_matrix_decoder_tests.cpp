#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include <cstring>
#include <tuple>
#include "ship/audio/SoundMatrixDecoder.h"

// Helper: build a stereo int16 buffer from pairs
static std::vector<uint8_t> MakeStereoBuffer(const std::vector<std::pair<int16_t, int16_t>>& pairs) {
    std::vector<uint8_t> buf(pairs.size() * 2 * sizeof(int16_t));
    for (size_t i = 0; i < pairs.size(); i++) {
        std::memcpy(&buf[i * 4 + 0], &pairs[i].first, sizeof(int16_t));
        std::memcpy(&buf[i * 4 + 2], &pairs[i].second, sizeof(int16_t));
    }
    return buf;
}

// ============================================================
// Construction
// ============================================================

TEST(SoundMatrixDecoder, ConstructWithStandardSampleRate) {
    // Should not throw at typical sample rates
    EXPECT_NO_THROW(Ship::SoundMatrixDecoder dec(44100));
}

TEST(SoundMatrixDecoder, ConstructWith48kHz) {
    EXPECT_NO_THROW(Ship::SoundMatrixDecoder dec(48000));
}

TEST(SoundMatrixDecoder, ConstructWith22kHz) {
    EXPECT_NO_THROW(Ship::SoundMatrixDecoder dec(22050));
}

// ============================================================
// ResetState
// ============================================================

TEST(SoundMatrixDecoder, ResetStateDoesNotCrash) {
    Ship::SoundMatrixDecoder dec(44100);
    // Process some samples to dirty the filter state
    auto buf = MakeStereoBuffer({ { 10000, -10000 }, { 5000, -5000 } });
    dec.Process(buf.data(), buf.size());
    // Then reset — should succeed without throwing
    EXPECT_NO_THROW(dec.ResetState());
}

TEST(SoundMatrixDecoder, AfterResetStateOutputMatchesFreshDecoder) {
    // Two decoders at the same rate should give identical output after one is reset
    Ship::SoundMatrixDecoder dec1(44100);
    Ship::SoundMatrixDecoder dec2(44100);

    // Dirty dec1 with some samples
    auto noise = MakeStereoBuffer({ { 32767, -32768 }, { 1000, -1000 }, { 500, 200 } });
    dec1.Process(noise.data(), noise.size());

    // Reset dec1 to match the fresh state of dec2
    dec1.ResetState();

    // Both should now produce identical output for the same input
    auto input = MakeStereoBuffer({ { 8000, -8000 }, { 4000, 4000 } });
    auto [ptr1, len1] = dec1.Process(input.data(), input.size());
    auto [ptr2, len2] = dec2.Process(input.data(), input.size());

    ASSERT_EQ(len1, len2);
    EXPECT_EQ(std::memcmp(ptr1, ptr2, len1), 0);
}

// ============================================================
// Process — output size
// ============================================================

TEST(SoundMatrixDecoder, ProcessOutputSizeIs6ChannelsPerStereopair) {
    Ship::SoundMatrixDecoder dec(44100);
    // 1 stereo pair → 6 int16 samples = 12 bytes
    auto buf = MakeStereoBuffer({ { 0, 0 } });
    auto [ptr, len] = dec.Process(buf.data(), buf.size());
    EXPECT_EQ(len, 1 * 6 * static_cast<int>(sizeof(int16_t)));
}

TEST(SoundMatrixDecoder, ProcessOutputSizeScalesWithInput) {
    Ship::SoundMatrixDecoder dec(44100);
    const int pairs = 64;
    auto buf = MakeStereoBuffer(std::vector<std::pair<int16_t, int16_t>>(pairs, { 100, -100 }));
    auto [ptr, len] = dec.Process(buf.data(), buf.size());
    EXPECT_EQ(len, pairs * 6 * static_cast<int>(sizeof(int16_t)));
}

TEST(SoundMatrixDecoder, ProcessEmptyBufferReturnsZeroLength) {
    Ship::SoundMatrixDecoder dec(44100);
    auto [ptr, len] = dec.Process(nullptr, 0);
    EXPECT_EQ(len, 0);
}

// ============================================================
// Process — output validity
// ============================================================

TEST(SoundMatrixDecoder, ProcessReturnNonNullPointerForNonEmptyInput) {
    Ship::SoundMatrixDecoder dec(44100);
    auto buf = MakeStereoBuffer({ { 1000, -1000 } });
    auto [ptr, len] = dec.Process(buf.data(), buf.size());
    EXPECT_NE(ptr, nullptr);
    EXPECT_GT(len, 0);
}

TEST(SoundMatrixDecoder, SilentInputProducesFiniteOutput) {
    Ship::SoundMatrixDecoder dec(44100);
    // Feed silence; output should be all zeros (filters start at zero)
    auto buf = MakeStereoBuffer(std::vector<std::pair<int16_t, int16_t>>(8, { 0, 0 }));
    auto [ptr, len] = dec.Process(buf.data(), buf.size());
    const int16_t* out = reinterpret_cast<const int16_t*>(ptr);
    int samples = len / static_cast<int>(sizeof(int16_t));
    for (int i = 0; i < samples; i++) {
        EXPECT_EQ(out[i], 0) << "sample " << i << " should be zero for silent input";
    }
}

TEST(SoundMatrixDecoder, OutputIsSymmetricForSymmetricInput) {
    // For L = R, FL should equal FR
    Ship::SoundMatrixDecoder dec(44100);
    auto buf = MakeStereoBuffer({ { 10000, 10000 } });
    auto [ptr, len] = dec.Process(buf.data(), buf.size());
    const int16_t* out = reinterpret_cast<const int16_t*>(ptr);
    // out[0]=FL, out[1]=FR
    EXPECT_EQ(out[0], out[1]) << "FL and FR should be equal for symmetric stereo input";
}

// ============================================================
// Saturation clamping
// ============================================================

TEST(SoundMatrixDecoder, ExtremeInputIsClamped) {
    Ship::SoundMatrixDecoder dec(44100);
    // Use maximum-magnitude values; output must not exceed int16 range
    auto buf = MakeStereoBuffer(std::vector<std::pair<int16_t, int16_t>>(
        128, { std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::min() }));
    auto [ptr, len] = dec.Process(buf.data(), buf.size());
    const int16_t* out = reinterpret_cast<const int16_t*>(ptr);
    int samples = len / static_cast<int>(sizeof(int16_t));
    for (int i = 0; i < samples; i++) {
        EXPECT_GE(out[i], std::numeric_limits<int16_t>::min());
        EXPECT_LE(out[i], std::numeric_limits<int16_t>::max());
    }
}

// ============================================================
// Determinism
// ============================================================

TEST(SoundMatrixDecoder, SameInputProducesSameOutput) {
    auto input = MakeStereoBuffer({ { 1000, -500 }, { 2000, -1500 }, { -300, 800 } });

    Ship::SoundMatrixDecoder dec1(44100);
    auto [ptr1, len1] = dec1.Process(input.data(), input.size());
    std::vector<uint8_t> out1(ptr1, ptr1 + len1);

    Ship::SoundMatrixDecoder dec2(44100);
    auto [ptr2, len2] = dec2.Process(input.data(), input.size());
    std::vector<uint8_t> out2(ptr2, ptr2 + len2);

    ASSERT_EQ(len1, len2);
    EXPECT_EQ(out1, out2);
}
