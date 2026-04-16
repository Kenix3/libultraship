// Fast3D Layer 5 Tests — Reference RDP Implementation Comparison
//
// Layer 5 verifies Fast3D's RDP processing against hand-computed reference
// values derived from the N64 RDP specification and Dillonb's softrdp
// (https://github.com/Dillonb/n64, src/rdp/softrdp.cpp).
//
// These tests compare Fast3D's internal state and output against what the
// real N64 RDP hardware would produce for the same commands. This catches
// deviations in:
//   - Fill color conversion (RGBA16 packed → RGBA8)
//   - Color combiner formula normalization
//   - Scissor coordinate conversion (U10.2 → float)
//   - OtherMode bit manipulation
//   - Fill rectangle vertex color assignment
//   - Color combiner input mapping for shader generation
//   - Alpha blender mode detection
//   - Depth clear region coordinate handling
//
// The N64 RDP color combiner formula is:
//   RGB:   (A - B) * C + D
//   Alpha: (A - B) * C + D
// where A, B, C, D are selected from a set of color sources.
//
// Layer hierarchy:
//   Layer 1: Pure computation (macros, matrix math, CC features)
//   Layer 2: Single interpreter methods with state
//   Layer 3: Multi-step pipelines (vertex + lighting, display list dispatch)
//   Layer 4: End-to-end rendering pipeline (triangle draw → API calls)
//   Layer 5: Reference implementation comparison (Fast3D vs N64 RDP spec)

#include "fast3d_test_common.h"

using namespace fast3d_test;

// ============================================================
// Helper: color_comb / alpha_comb (same as interpreter.cpp)
// These pack A, B, C, D combiner inputs into the combine_mode word.
// ============================================================

static inline uint32_t test_color_comb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 0xf) | ((b & 0xf) << 4) | ((c & 0x1f) << 8) | ((d & 7) << 13);
}

static inline uint32_t test_alpha_comb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 7) | ((b & 7) << 3) | ((c & 7) << 6) | ((d & 7) << 9);
}

// Pack cycle 1 + cycle 2 into a 64-bit combine_mode
static inline uint64_t pack_combine_mode(uint32_t rgb1, uint32_t alpha1,
                                          uint32_t rgb2 = 0, uint32_t alpha2 = 0) {
    return rgb1 | ((uint64_t)alpha1 << 16) | ((uint64_t)rgb2 << 28) | ((uint64_t)alpha2 << 44);
}

// ============================================================
// Layer 5 Test Fixture
// ============================================================

class ReferenceRDPTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_unique<Fast::Interpreter>();
        stub = new StubRenderingAPI();
        interp->mRapi = stub;

        // Set screen dimensions
        interp->mNativeDimensions.width = 320.0f;
        interp->mNativeDimensions.height = 240.0f;
        interp->mCurDimensions.width = 320.0f;
        interp->mCurDimensions.height = 240.0f;
        interp->mFbActive = false;

        // Reset RSP state
        memset(interp->mRsp, 0, sizeof(Fast::RSP));
        interp->mRsp->modelview_matrix_stack_size = 1;

        // Identity matrices
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++) {
                interp->mRsp->MP_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
                interp->mRsp->modelview_matrix_stack[0][i][j] = (i == j) ? 1.0f : 0.0f;
            }

        interp->mRsp->geometry_mode = 0;
        interp->mRdp->combine_mode = 0;
        interp->mRenderingState = {};
    }

    void TearDown() override {
        interp->mRapi = nullptr;
        delete stub;
    }

    std::unique_ptr<Fast::Interpreter> interp;
    StubRenderingAPI* stub;
};

// ============================================================
// 1. Fill Color Conversion: RGBA16 Packed → RGBA8
//
// The N64 RDP packs fill color as RGBA5551 in a 32-bit word (the
// lower 16 bits are used). Fast3D should convert using SCALE_5_8.
// Reference values computed from the RDP hardware formula:
//   r8 = (r5 * 0xFF) / 0x1F
//   g8 = (g5 * 0xFF) / 0x1F
//   b8 = (b5 * 0xFF) / 0x1F
//   a8 = a1 * 255
// ============================================================

TEST_F(ReferenceRDPTest, FillColor_White) {
    // RGBA5551: r=31, g=31, b=31, a=1 → 0xFFFF
    interp->GfxDpSetFillColor(0xFFFF);
    EXPECT_EQ(interp->mRdp->fill_color.r, 255);
    EXPECT_EQ(interp->mRdp->fill_color.g, 255);
    EXPECT_EQ(interp->mRdp->fill_color.b, 255);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(ReferenceRDPTest, FillColor_Black) {
    // RGBA5551: r=0, g=0, b=0, a=0 → 0x0000
    interp->GfxDpSetFillColor(0x0000);
    EXPECT_EQ(interp->mRdp->fill_color.r, 0);
    EXPECT_EQ(interp->mRdp->fill_color.g, 0);
    EXPECT_EQ(interp->mRdp->fill_color.b, 0);
    EXPECT_EQ(interp->mRdp->fill_color.a, 0);
}

TEST_F(ReferenceRDPTest, FillColor_Red) {
    // RGBA5551: r=31, g=0, b=0, a=1 → 0xF801
    uint16_t packed = (31 << 11) | (0 << 6) | (0 << 1) | 1;
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, SCALE_5_8(31));
    EXPECT_EQ(interp->mRdp->fill_color.g, 0);
    EXPECT_EQ(interp->mRdp->fill_color.b, 0);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(ReferenceRDPTest, FillColor_Green) {
    // RGBA5551: r=0, g=31, b=0, a=1 → 0x07C1
    uint16_t packed = (0 << 11) | (31 << 6) | (0 << 1) | 1;
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, 0);
    EXPECT_EQ(interp->mRdp->fill_color.g, SCALE_5_8(31));
    EXPECT_EQ(interp->mRdp->fill_color.b, 0);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(ReferenceRDPTest, FillColor_Blue) {
    // RGBA5551: r=0, g=0, b=31, a=1 → 0x003F
    uint16_t packed = (0 << 11) | (0 << 6) | (31 << 1) | 1;
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, 0);
    EXPECT_EQ(interp->mRdp->fill_color.g, 0);
    EXPECT_EQ(interp->mRdp->fill_color.b, SCALE_5_8(31));
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(ReferenceRDPTest, FillColor_MidGray) {
    // RGBA5551: r=16, g=16, b=16, a=1
    // Reference: SCALE_5_8(16) = (16 * 0xFF) / 0x1F = 4080 / 31 = 131
    uint16_t packed = (16 << 11) | (16 << 6) | (16 << 1) | 1;
    interp->GfxDpSetFillColor(packed);
    uint8_t expected = SCALE_5_8(16);
    EXPECT_EQ(interp->mRdp->fill_color.r, expected);
    EXPECT_EQ(interp->mRdp->fill_color.g, expected);
    EXPECT_EQ(interp->mRdp->fill_color.b, expected);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(ReferenceRDPTest, FillColor_AlphaZero) {
    // RGBA5551: r=31, g=31, b=31, a=0 → 0xFFFE
    uint16_t packed = (31 << 11) | (31 << 6) | (31 << 1) | 0;
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, 255);
    EXPECT_EQ(interp->mRdp->fill_color.g, 255);
    EXPECT_EQ(interp->mRdp->fill_color.b, 255);
    EXPECT_EQ(interp->mRdp->fill_color.a, 0);
}

TEST_F(ReferenceRDPTest, FillColor_UpperWordIgnored) {
    // The upper 16 bits of packed_color should be ignored (only lower 16 used)
    uint32_t packed = 0xDEAD0000 | 0xFFFF; // upper 16 = garbage
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, 255);
    EXPECT_EQ(interp->mRdp->fill_color.g, 255);
    EXPECT_EQ(interp->mRdp->fill_color.b, 255);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

// ============================================================
// 2. Environment / Primitive / Fog / Blend Color Setting
//
// Per RDP spec these are simple 8-bit RGBA registers.
// Verify the interpreter stores them correctly.
// ============================================================

TEST_F(ReferenceRDPTest, EnvColor_FullRange) {
    interp->GfxDpSetEnvColor(10, 20, 30, 40);
    EXPECT_EQ(interp->mRdp->env_color.r, 10);
    EXPECT_EQ(interp->mRdp->env_color.g, 20);
    EXPECT_EQ(interp->mRdp->env_color.b, 30);
    EXPECT_EQ(interp->mRdp->env_color.a, 40);
}

TEST_F(ReferenceRDPTest, EnvColor_MaxValues) {
    interp->GfxDpSetEnvColor(255, 255, 255, 255);
    EXPECT_EQ(interp->mRdp->env_color.r, 255);
    EXPECT_EQ(interp->mRdp->env_color.g, 255);
    EXPECT_EQ(interp->mRdp->env_color.b, 255);
    EXPECT_EQ(interp->mRdp->env_color.a, 255);
}

TEST_F(ReferenceRDPTest, PrimColor_WithLodFraction) {
    // GfxDpSetPrimColor(minLevel, lodFrac, r, g, b, a)
    interp->GfxDpSetPrimColor(5, 128, 100, 150, 200, 250);
    EXPECT_EQ(interp->mRdp->prim_color.r, 100);
    EXPECT_EQ(interp->mRdp->prim_color.g, 150);
    EXPECT_EQ(interp->mRdp->prim_color.b, 200);
    EXPECT_EQ(interp->mRdp->prim_color.a, 250);
    // prim_lod_fraction should be set to lodFrac parameter
    EXPECT_EQ(interp->mRdp->prim_lod_fraction, 128);
}

TEST_F(ReferenceRDPTest, FogColor_AllChannels) {
    interp->GfxDpSetFogColor(11, 22, 33, 44);
    EXPECT_EQ(interp->mRdp->fog_color.r, 11);
    EXPECT_EQ(interp->mRdp->fog_color.g, 22);
    EXPECT_EQ(interp->mRdp->fog_color.b, 33);
    EXPECT_EQ(interp->mRdp->fog_color.a, 44);
}

TEST_F(ReferenceRDPTest, BlendColor_NotYetImplemented) {
    // GfxDpSetBlendColor is currently a no-op (TODO in interpreter.cpp).
    // When implemented, it should set blend_color.r/g/b/a.
    // For now, verify the method is callable without crashing.
    interp->GfxDpSetBlendColor(55, 66, 77, 88);
    // blend_color remains at its default (zero) since the method is a no-op
    EXPECT_EQ(interp->mRdp->blend_color.r, 0);
}

// ============================================================
// 3. Scissor Conversion: U10.2 Fixed-Point → Float
//
// The N64 RDP scissor command uses U10.2 fixed-point coordinates
// (10 bits integer + 2 bits fraction, scaled by 4).
// The interpreter divides by 4.0 to get pixel coordinates.
//
// Reference: scissor pixel = U10.2_value / 4.0
// ============================================================

TEST_F(ReferenceRDPTest, Scissor_FullScreen) {
    // 320x240 screen: ulx=0, uly=0, lrx=320*4=1280, lry=240*4=960
    interp->GfxDpSetScissor(0, 0, 0, 1280, 960);

    // x = ulx/4 = 0
    // y = lry/4 = 240
    // width = (lrx-ulx)/4 = 320
    // height = (lry-uly)/4 = 240
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.width, 320.0f);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.height, 240.0f);
}

TEST_F(ReferenceRDPTest, Scissor_PartialRegion) {
    // ulx=40*4=160, uly=30*4=120, lrx=280*4=1120, lry=210*4=840
    interp->GfxDpSetScissor(0, 160, 120, 1120, 840);

    // width = (1120-160)/4 = 240
    // height = (840-120)/4 = 180
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.width, 240.0f);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.height, 180.0f);
}

TEST_F(ReferenceRDPTest, Scissor_SubPixelPrecision) {
    // ulx=1 (= 0.25 pixels), uly=0, lrx=5 (= 1.25 pixels), lry=4 (= 1 pixel)
    interp->GfxDpSetScissor(0, 1, 0, 5, 4);

    // width = (5-1)/4 = 1.0
    // height = (4-0)/4 = 1.0
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.width, 1.0f);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.height, 1.0f);
}

TEST_F(ReferenceRDPTest, Scissor_SetsChangedFlag) {
    interp->mRdp->viewport_or_scissor_changed = false;
    interp->GfxDpSetScissor(0, 0, 0, 1280, 960);
    EXPECT_TRUE(interp->mRdp->viewport_or_scissor_changed);
}

// ============================================================
// 4. OtherMode Set/Clear: Bit Manipulation
//
// The N64 RDP's "other mode" register is a 64-bit value split into
// two 32-bit halves (other_mode_h and other_mode_l). GfxSpSetOtherMode
// applies a bit mask to set specific bits.
//
// Reference formula:
//   mask = ((1 << num_bits) - 1) << shift
//   other_mode = (other_mode & ~mask) | mode
// ============================================================

TEST_F(ReferenceRDPTest, OtherMode_DirectSet) {
    interp->GfxDpSetOtherMode(0x12345678, 0xABCDEF01);
    EXPECT_EQ(interp->mRdp->other_mode_h, 0x12345678u);
    EXPECT_EQ(interp->mRdp->other_mode_l, 0xABCDEF01u);
}

TEST_F(ReferenceRDPTest, OtherMode_DirectSet_OverwritesPrevious) {
    interp->GfxDpSetOtherMode(0xFFFFFFFF, 0xFFFFFFFF);
    interp->GfxDpSetOtherMode(0x00000000, 0x00000000);
    EXPECT_EQ(interp->mRdp->other_mode_h, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l, 0u);
}

// ============================================================
// 5. Color Combiner Mode Setting
//
// The N64 RDP combine_mode is a 64-bit value encoding the color
// combiner formula for 1-cycle and 2-cycle modes.
//
// For 1-cycle: combine_mode = rgb1 | (alpha1 << 16)
//   (cycle 2 fields are zero)
//
// For 2-cycle: combine_mode = rgb1 | (alpha1 << 16) | (rgb2 << 28) | (alpha2 << 44)
//
// Reference: verify the bit packing matches the N64 RDP format.
// ============================================================

TEST_F(ReferenceRDPTest, CombineMode_1Cycle_ShadeOnly) {
    // CC formula: (0 - 0) * 0 + SHADE → result = shade color
    uint32_t rgb = test_color_comb(0, 0, 0, G_CCMUX_SHADE);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_SHADE);

    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    uint64_t expected = pack_combine_mode(rgb, alpha);
    EXPECT_EQ(interp->mRdp->combine_mode, expected);
}

TEST_F(ReferenceRDPTest, CombineMode_1Cycle_EnvOnly) {
    // CC formula: (0 - 0) * 0 + ENV → result = environment color
    uint32_t rgb = test_color_comb(0, 0, 0, G_CCMUX_ENVIRONMENT);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_ENVIRONMENT);

    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    uint64_t expected = pack_combine_mode(rgb, alpha);
    EXPECT_EQ(interp->mRdp->combine_mode, expected);
}

TEST_F(ReferenceRDPTest, CombineMode_1Cycle_PrimOnly) {
    uint32_t rgb = test_color_comb(0, 0, 0, G_CCMUX_PRIMITIVE);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_PRIMITIVE);

    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    uint64_t expected = pack_combine_mode(rgb, alpha);
    EXPECT_EQ(interp->mRdp->combine_mode, expected);
}

TEST_F(ReferenceRDPTest, CombineMode_1Cycle_Texel0Only) {
    uint32_t rgb = test_color_comb(0, 0, 0, G_CCMUX_TEXEL0);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_TEXEL0);

    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    uint64_t expected = pack_combine_mode(rgb, alpha);
    EXPECT_EQ(interp->mRdp->combine_mode, expected);
}

TEST_F(ReferenceRDPTest, CombineMode_1Cycle_ShadeTimesTexel0) {
    // CC formula: (TEXEL0 - 0) * SHADE + 0
    // This is the classic "modulate" mode: TEXEL0 * SHADE
    uint32_t rgb = test_color_comb(G_CCMUX_TEXEL0, 0, G_CCMUX_SHADE, 0);
    uint32_t alpha = test_alpha_comb(G_ACMUX_TEXEL0, 0, G_ACMUX_SHADE, 0);

    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    uint64_t expected = pack_combine_mode(rgb, alpha);
    EXPECT_EQ(interp->mRdp->combine_mode, expected);
}

TEST_F(ReferenceRDPTest, CombineMode_2Cycle_Packed) {
    // Verify both cycles are packed correctly
    uint32_t rgb1 = test_color_comb(G_CCMUX_TEXEL0, 0, G_CCMUX_SHADE, 0);
    uint32_t alpha1 = test_alpha_comb(G_ACMUX_TEXEL0, 0, G_ACMUX_SHADE, 0);
    uint32_t rgb2 = test_color_comb(G_CCMUX_COMBINED, 0, G_CCMUX_ENVIRONMENT, 0);
    uint32_t alpha2 = test_alpha_comb(G_ACMUX_COMBINED, 0, G_ACMUX_ENVIRONMENT, 0);

    interp->GfxDpSetCombineMode(rgb1, alpha1, rgb2, alpha2);

    uint64_t expected = pack_combine_mode(rgb1, alpha1, rgb2, alpha2);
    EXPECT_EQ(interp->mRdp->combine_mode, expected);
}

// ============================================================
// 6. Color Combiner GenerateCC Verification
//
// Verify that GenerateCC correctly extracts A, B, C, D from the
// packed combine_mode and maps them to shader IDs.
//
// The N64 RDP packs the combiner as:
//   Bits [3:0]   = A (rgb)
//   Bits [7:4]   = B (rgb)
//   Bits [12:8]  = C (rgb, 5 bits)
//   Bits [15:13] = D (rgb, 3 bits)
//   Bits [18:16] = A (alpha)
//   Bits [21:19] = B (alpha)
//   Bits [24:22] = C (alpha)
//   Bits [27:25] = D (alpha)
//   Cycle 2 starts at bit 28.
// ============================================================

TEST_F(ReferenceRDPTest, GenerateCC_ShadeOnly_NoTextures) {
    // (0 - 0) * 0 + SHADE → only uses shade, no textures
    uint32_t rgb = test_color_comb(0, 0, 0, G_CCMUX_SHADE);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_SHADE);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0; // 1-cycle, no options

    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);

    // Shade-only should NOT use textures
    EXPECT_FALSE(comb.usedTextures[0]);
    EXPECT_FALSE(comb.usedTextures[1]);
}

TEST_F(ReferenceRDPTest, GenerateCC_Texel0Only_UsesTexture0) {
    // (0 - 0) * 0 + TEXEL0 → uses texture 0
    uint32_t rgb = test_color_comb(0, 0, 0, G_CCMUX_TEXEL0);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_TEXEL0);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0;

    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);

    EXPECT_TRUE(comb.usedTextures[0]);
    EXPECT_FALSE(comb.usedTextures[1]);
}

TEST_F(ReferenceRDPTest, GenerateCC_Texel1_In1Cycle_RemappedToTexel0) {
    // In 1-cycle mode, TEXEL1 should be remapped to TEXEL0 (both read same tile)
    uint32_t rgb = test_color_comb(0, 0, 0, G_CCMUX_TEXEL1);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_TEXEL1);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0; // 1-cycle (no _2CYC)

    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);

    // After remapping TEXEL1→TEXEL0, texture 0 should be marked used
    EXPECT_TRUE(comb.usedTextures[0]);
}

TEST_F(ReferenceRDPTest, GenerateCC_2Cycle_BothTextures) {
    // 2-cycle mode: cycle 1 uses TEXEL0, cycle 2 uses TEXEL0 (reads texture 1 in 2-cycle)
    uint32_t rgb1 = test_color_comb(G_CCMUX_TEXEL0, 0, G_CCMUX_SHADE, 0);
    uint32_t alpha1 = test_alpha_comb(G_ACMUX_TEXEL0, 0, G_ACMUX_SHADE, 0);
    uint32_t rgb2 = test_color_comb(G_CCMUX_TEXEL0, 0, G_CCMUX_COMBINED, 0);
    uint32_t alpha2 = test_alpha_comb(G_ACMUX_TEXEL0, 0, 0, 0);
    interp->GfxDpSetCombineMode(rgb1, alpha1, rgb2, alpha2);

    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = SHADER_OPT(_2CYC);

    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);

    // In 2-cycle mode, TEXEL0 in cycle 1 → usedTextures[0]
    // TEXEL0 in cycle 2 → usedTextures[1] (swapped for 2-cycle)
    EXPECT_TRUE(comb.usedTextures[0]);
    EXPECT_TRUE(comb.usedTextures[1]);
}

TEST_F(ReferenceRDPTest, GenerateCC_NormalizesIdenticalAB) {
    // When A == B, the formula (A - B) * C + D reduces to just D.
    // The interpreter normalizes this by setting A=B=C=0.
    uint32_t rgb = test_color_comb(G_CCMUX_SHADE, G_CCMUX_SHADE, G_CCMUX_ENVIRONMENT, G_CCMUX_PRIMITIVE);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_SHADE);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0;

    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);

    // After normalization, A=B=0 and C=0 → only D (PRIMITIVE) contributes
    // Verify no textures are used (PRIMITIVE is a non-texture input)
    EXPECT_FALSE(comb.usedTextures[0]);
    EXPECT_FALSE(comb.usedTextures[1]);
}

TEST_F(ReferenceRDPTest, GenerateCC_NormalizesZeroC) {
    // When C == 0, the formula (A - B) * 0 + D = D regardless of A and B.
    // The interpreter normalizes this by setting A=B=0.
    uint32_t rgb = test_color_comb(G_CCMUX_TEXEL0, G_CCMUX_ENVIRONMENT, G_CCMUX_0, G_CCMUX_SHADE);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_SHADE);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0;

    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);

    // TEXEL0 should NOT be used because the (A-B)*C term is zeroed out
    EXPECT_FALSE(comb.usedTextures[0]);
}

// ============================================================
// 7. Color Combiner Input Mapping
//
// When the combiner uses PRIMITIVE, ENVIRONMENT, SHADE, etc., the
// interpreter assigns each a unique "shader input number" (SHADER_INPUT_1,
// SHADER_INPUT_2, ...) and records the mapping in shader_input_mapping.
//
// Reference: each unique CCMUX source gets a sequential input number.
// ============================================================

TEST_F(ReferenceRDPTest, GenerateCC_InputMapping_ShadeAndPrim) {
    // (PRIM - 0) * SHADE + 0 → two inputs: PRIM and SHADE
    uint32_t rgb = test_color_comb(G_CCMUX_PRIMITIVE, 0, G_CCMUX_SHADE, 0);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_SHADE);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0;

    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);

    // PRIMITIVE and SHADE should both appear in shader_input_mapping[0]
    bool hasPrim = false, hasShade = false;
    for (int i = 0; i < 7; i++) {
        if (comb.shader_input_mapping[0][i] == G_CCMUX_PRIMITIVE) hasPrim = true;
        if (comb.shader_input_mapping[0][i] == G_CCMUX_SHADE) hasShade = true;
    }
    EXPECT_TRUE(hasPrim);
    EXPECT_TRUE(hasShade);
}

TEST_F(ReferenceRDPTest, GenerateCC_InputMapping_EnvAlpha) {
    // (TEXEL0 - 0) * ENV_ALPHA + 0
    uint32_t rgb = test_color_comb(G_CCMUX_TEXEL0, 0, G_CCMUX_ENV_ALPHA, 0);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_ENVIRONMENT);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);

    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0;

    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);

    // ENV_ALPHA should appear in the RGB input mapping
    bool hasEnvAlpha = false;
    for (int i = 0; i < 7; i++) {
        if (comb.shader_input_mapping[0][i] == G_CCMUX_ENV_ALPHA) hasEnvAlpha = true;
    }
    EXPECT_TRUE(hasEnvAlpha);
    // TEXEL0 should be used
    EXPECT_TRUE(comb.usedTextures[0]);
}

// ============================================================
// 8. Grayscale Color Setting
//
// Fast3D extension (not in original N64 RDP).
// Verify the color register is set correctly.
// ============================================================

TEST_F(ReferenceRDPTest, GrayscaleColor_Set) {
    interp->GfxDpSetGrayscaleColor(100, 200, 50, 128);
    EXPECT_EQ(interp->mRdp->grayscale_color.r, 100);
    EXPECT_EQ(interp->mRdp->grayscale_color.g, 200);
    EXPECT_EQ(interp->mRdp->grayscale_color.b, 50);
    EXPECT_EQ(interp->mRdp->grayscale_color.a, 128);
}

// ============================================================
// 9. Z Image / Color Image Address Setting
//
// Per RDP spec these just store the RDRAM address pointers.
// ============================================================

TEST_F(ReferenceRDPTest, ZImage_SetAddress) {
    uint8_t dummyAddr[16] = {};
    interp->GfxDpSetZImage(&dummyAddr);
    EXPECT_EQ(interp->mRdp->z_buf_address, (void*)&dummyAddr);
}

TEST_F(ReferenceRDPTest, ColorImage_SetAddress) {
    uint8_t dummyAddr[16] = {};
    interp->GfxDpSetColorImage(0, 0, 320, &dummyAddr);
    EXPECT_EQ(interp->mRdp->color_image_address, (void*)&dummyAddr);
}

TEST_F(ReferenceRDPTest, FillRect_ZBufAddress_SkipsFullScreen) {
    // When color_image_address == z_buf_address and the rect is fullscreen,
    // the fill should be skipped (depth clear is done by glClear).
    uint8_t dummyAddr[16] = {};
    interp->mRdp->z_buf_address = &dummyAddr;
    interp->mRdp->color_image_address = &dummyAddr;

    // Fullscreen fill: 0 to (native_width-1)*4 × (native_height-1)*4
    int32_t lrx = (int32_t)(interp->mNativeDimensions.width - 1) * 4;
    int32_t lry = (int32_t)(interp->mNativeDimensions.height - 1) * 4;

    // This should be a no-op (no draw calls)
    interp->GfxDpFillRectangle(0, 0, lrx, lry);
    // No textures should be uploaded (no drawing happened)
    EXPECT_TRUE(stub->uploads.empty());
}

// ============================================================
// 10. Geometry Mode Setting
//
// Per RSP spec, GfxSpGeometryMode clears then sets bits:
//   geometry_mode = (geometry_mode & ~clear) | set
// ============================================================

TEST_F(ReferenceRDPTest, GeometryMode_SetBit) {
    interp->mRsp->geometry_mode = 0;
    interp->GfxSpGeometryMode(0, G_ZBUFFER);
    EXPECT_EQ(interp->mRsp->geometry_mode & G_ZBUFFER, G_ZBUFFER);
}

TEST_F(ReferenceRDPTest, GeometryMode_ClearBit) {
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->GfxSpGeometryMode(G_ZBUFFER, 0);
    EXPECT_EQ(interp->mRsp->geometry_mode & G_ZBUFFER, 0u);
}

TEST_F(ReferenceRDPTest, GeometryMode_ClearAndSetDifferentBits) {
    interp->mRsp->geometry_mode = G_ZBUFFER | G_SHADE;
    interp->GfxSpGeometryMode(G_ZBUFFER, G_LIGHTING);
    EXPECT_EQ(interp->mRsp->geometry_mode & G_ZBUFFER, 0u);
    EXPECT_NE(interp->mRsp->geometry_mode & G_SHADE, 0u);
    EXPECT_NE(interp->mRsp->geometry_mode & G_LIGHTING, 0u);
}

TEST_F(ReferenceRDPTest, GeometryMode_SetMultipleBits) {
    interp->mRsp->geometry_mode = 0;
    interp->GfxSpGeometryMode(0, G_ZBUFFER | G_SHADE | G_LIGHTING);
    EXPECT_NE(interp->mRsp->geometry_mode & G_ZBUFFER, 0u);
    EXPECT_NE(interp->mRsp->geometry_mode & G_SHADE, 0u);
    EXPECT_NE(interp->mRsp->geometry_mode & G_LIGHTING, 0u);
}

// ============================================================
// 11. Texture Scaling
//
// GfxSpTexture sets the texture scaling factors and tile.
// Reference: sc and tc are Q0.16 fixed-point values.
// ============================================================

TEST_F(ReferenceRDPTest, TextureScaling_FullScale) {
    // sc=tc=0xFFFF means max scale (nearly 1.0 in Q0.16)
    interp->GfxSpTexture(0xFFFF, 0xFFFF, 0, 0, 1);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.s, 0xFFFF);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.t, 0xFFFF);
}

TEST_F(ReferenceRDPTest, TextureScaling_HalfScale) {
    // sc=tc=0x8000 means half scale
    interp->GfxSpTexture(0x8000, 0x8000, 0, 0, 1);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.s, 0x8000);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.t, 0x8000);
}

TEST_F(ReferenceRDPTest, TextureScaling_Zero) {
    interp->GfxSpTexture(0, 0, 0, 0, 0);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.s, 0);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.t, 0);
}

// ============================================================
// 12. SCALE Macro Roundtrip Verification
//
// The N64 RDP uses SCALE_5_8 and SCALE_8_5 for converting between
// 5-bit and 8-bit color components. Verify the roundtrip properties.
// ============================================================

TEST_F(ReferenceRDPTest, Scale5_8_Roundtrip) {
    // For all valid 5-bit values, SCALE_8_5(SCALE_5_8(x)) should
    // approximate the original value (within 1 due to integer division).
    for (int i = 0; i <= 31; i++) {
        uint8_t expanded = SCALE_5_8(i);
        uint8_t compressed = SCALE_8_5(expanded);
        EXPECT_NEAR(compressed, i, 1) << "5->8->5 roundtrip failed for i=" << i;
    }
}

TEST_F(ReferenceRDPTest, Scale4_8_Roundtrip) {
    for (int i = 0; i <= 15; i++) {
        uint8_t expanded = SCALE_4_8(i);
        uint8_t compressed = SCALE_8_4(expanded);
        EXPECT_NEAR(compressed, i, 1) << "4->8->4 roundtrip failed for i=" << i;
    }
}

TEST_F(ReferenceRDPTest, Scale5_8_Endpoints) {
    EXPECT_EQ(SCALE_5_8(0), 0);
    EXPECT_EQ(SCALE_5_8(31), 255);
}

TEST_F(ReferenceRDPTest, Scale4_8_Endpoints) {
    EXPECT_EQ(SCALE_4_8(0), 0);
    EXPECT_EQ(SCALE_4_8(15), 255);
}

// ============================================================
// 13. Fill Color → Fill Rectangle Integration
//
// In FILL cycle mode, FillRectangle uses fill_color for all vertices.
// Verify that setting fill color then filling a rect propagates the
// correct vertex colors through the pipeline.
// ============================================================

TEST_F(ReferenceRDPTest, FillRect_SetsVertexColorsToFillColor) {
    // Set fill color to a known value
    uint16_t packed = (15 << 11) | (20 << 6) | (10 << 1) | 1;
    interp->GfxDpSetFillColor(packed);

    // Set cycle mode to FILL
    interp->mRdp->other_mode_h = G_CYC_FILL;

    // Ensure color_image != z_buf to avoid the depth-clear optimization
    uint8_t colorBuf[16] = {};
    uint8_t zBuf[16] = {};
    interp->mRdp->color_image_address = &colorBuf;
    interp->mRdp->z_buf_address = &zBuf;

    // Need a texture upload buffer for the fill rect pipeline
    interp->mTexUploadBuffer = (uint8_t*)malloc(8192 * 8192 * 4);

    // FillRectangle will set loaded_vertices[MAX_VERTICES..+3].color = fill_color
    // and then call GfxDrawRectangle which draws 2 triangles.
    // U10.2 coords: 10*4=40, 10*4=40, 100*4=400, 100*4=400
    interp->GfxDpFillRectangle(40, 40, 400, 400);

    // Check that the rect vertices got fill_color assigned
    uint8_t expectedR = SCALE_5_8(15);
    uint8_t expectedG = SCALE_5_8(20);
    uint8_t expectedB = SCALE_5_8(10);

    for (int i = MAX_VERTICES; i < MAX_VERTICES + 4; i++) {
        EXPECT_EQ(interp->mRsp->loaded_vertices[i].color.r, expectedR)
            << "Vertex " << i << " red mismatch";
        EXPECT_EQ(interp->mRsp->loaded_vertices[i].color.g, expectedG)
            << "Vertex " << i << " green mismatch";
        EXPECT_EQ(interp->mRsp->loaded_vertices[i].color.b, expectedB)
            << "Vertex " << i << " blue mismatch";
        EXPECT_EQ(interp->mRsp->loaded_vertices[i].color.a, 255)
            << "Vertex " << i << " alpha mismatch";
    }

    free(interp->mTexUploadBuffer);
    interp->mTexUploadBuffer = nullptr;
}

// ============================================================
// 14. Fill Color All 5-bit Channel Combinations
//
// Exhaustively verify a subset of RGBA5551 packed values against
// the reference SCALE_5_8 formula.
// ============================================================

TEST_F(ReferenceRDPTest, FillColor_AllRedValues) {
    for (int r5 = 0; r5 <= 31; r5++) {
        uint16_t packed = (r5 << 11) | (0 << 6) | (0 << 1) | 1;
        interp->GfxDpSetFillColor(packed);
        EXPECT_EQ(interp->mRdp->fill_color.r, SCALE_5_8(r5))
            << "Red 5-bit value " << r5 << " incorrectly converted";
    }
}

TEST_F(ReferenceRDPTest, FillColor_AllGreenValues) {
    for (int g5 = 0; g5 <= 31; g5++) {
        uint16_t packed = (0 << 11) | (g5 << 6) | (0 << 1) | 1;
        interp->GfxDpSetFillColor(packed);
        EXPECT_EQ(interp->mRdp->fill_color.g, SCALE_5_8(g5))
            << "Green 5-bit value " << g5 << " incorrectly converted";
    }
}

TEST_F(ReferenceRDPTest, FillColor_AllBlueValues) {
    for (int b5 = 0; b5 <= 31; b5++) {
        uint16_t packed = (0 << 11) | (0 << 6) | (b5 << 1) | 1;
        interp->GfxDpSetFillColor(packed);
        EXPECT_EQ(interp->mRdp->fill_color.b, SCALE_5_8(b5))
            << "Blue 5-bit value " << b5 << " incorrectly converted";
    }
}
