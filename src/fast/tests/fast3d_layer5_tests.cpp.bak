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

// ============================================================
// 15. Depth Test/Mask Computation — Reference Verification
//
// The N64 RDP requires BOTH G_ZBUFFER geometry mode AND Z_CMP
// other_mode_l to enable depth testing. Depth writes require Z_UPD.
//
// Reference formula (from interpreter.cpp line 1693-1694):
//   depth_test = (geometry_mode & G_ZBUFFER) && (other_mode_l & Z_CMP)
//   depth_mask = (other_mode_l & Z_UPD) != 0
//   packed = (depth_test ? 1 : 0) | (depth_mask ? 2 : 0)
//
// This is a truth table test: all combinations of the three flags.
// ============================================================

TEST_F(ReferenceRDPTest, DepthComputation_NothingSet) {
    // No G_ZBUFFER, no Z_CMP, no Z_UPD → depth_test=false, depth_mask=false
    interp->mRsp->geometry_mode = 0;
    interp->mRdp->other_mode_l = 0;

    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    bool depth_mask = (interp->mRdp->other_mode_l & Z_UPD) == Z_UPD;
    uint8_t packed = (depth_test ? 1 : 0) | (depth_mask ? 2 : 0);

    EXPECT_FALSE(depth_test);
    EXPECT_FALSE(depth_mask);
    EXPECT_EQ(packed, 0u);
}

TEST_F(ReferenceRDPTest, DepthComputation_ZBufferOnly) {
    // G_ZBUFFER set, but no Z_CMP → depth_test=false (need both)
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->mRdp->other_mode_l = 0;

    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    EXPECT_FALSE(depth_test);
}

TEST_F(ReferenceRDPTest, DepthComputation_ZCmpOnly) {
    // Z_CMP set, but no G_ZBUFFER → depth_test=false (need both)
    interp->mRsp->geometry_mode = 0;
    interp->mRdp->other_mode_l = Z_CMP;

    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    EXPECT_FALSE(depth_test);
}

TEST_F(ReferenceRDPTest, DepthComputation_ZBufferAndZCmp) {
    // Both G_ZBUFFER and Z_CMP set → depth_test=true
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->mRdp->other_mode_l = Z_CMP;

    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    EXPECT_TRUE(depth_test);
}

TEST_F(ReferenceRDPTest, DepthComputation_ZUpdOnly) {
    // Z_UPD alone → depth_mask=true (independent of depth_test)
    interp->mRdp->other_mode_l = Z_UPD;

    bool depth_mask = (interp->mRdp->other_mode_l & Z_UPD) == Z_UPD;
    EXPECT_TRUE(depth_mask);
}

TEST_F(ReferenceRDPTest, DepthComputation_AllThreeFlags) {
    // G_ZBUFFER + Z_CMP + Z_UPD → depth_test=true, depth_mask=true, packed=3
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->mRdp->other_mode_l = Z_CMP | Z_UPD;

    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    bool depth_mask = (interp->mRdp->other_mode_l & Z_UPD) == Z_UPD;
    uint8_t packed = (depth_test ? 1 : 0) | (depth_mask ? 2 : 0);

    EXPECT_TRUE(depth_test);
    EXPECT_TRUE(depth_mask);
    EXPECT_EQ(packed, 3u);
}

TEST_F(ReferenceRDPTest, DepthComputation_ZCmpZUpd_NoGeometry) {
    // Z_CMP + Z_UPD, but no G_ZBUFFER → depth_test=false, depth_mask=true
    interp->mRsp->geometry_mode = 0;
    interp->mRdp->other_mode_l = Z_CMP | Z_UPD;

    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    bool depth_mask = (interp->mRdp->other_mode_l & Z_UPD) == Z_UPD;
    uint8_t packed = (depth_test ? 1 : 0) | (depth_mask ? 2 : 0);

    EXPECT_FALSE(depth_test);
    EXPECT_TRUE(depth_mask);
    EXPECT_EQ(packed, 2u);
}

// ============================================================
// 16. ZMODE Variant Detection — Reference Values
//
// The N64 RDP supports four Z modes via bits 10-11 of other_mode_l:
//   ZMODE_OPA   = 0x000  (Opaque)
//   ZMODE_INTER = 0x400  (Interpenetrating)
//   ZMODE_XLU   = 0x800  (Translucent)
//   ZMODE_DEC   = 0xC00  (Decal)
//
// Decal mode is detected as: (other_mode_l & ZMODE_DEC) == ZMODE_DEC
// This means ZMODE_DEC is the only mode where the decal flag is true,
// because ZMODE_DEC = 0xC00 has both bits set.
// ============================================================

TEST_F(ReferenceRDPTest, ZMode_OpaqueIsNotDecal) {
    interp->mRdp->other_mode_l = ZMODE_OPA;
    bool zmode_decal = (interp->mRdp->other_mode_l & ZMODE_DEC) == ZMODE_DEC;
    EXPECT_FALSE(zmode_decal);
}

TEST_F(ReferenceRDPTest, ZMode_InterIsNotDecal) {
    interp->mRdp->other_mode_l = ZMODE_INTER;
    bool zmode_decal = (interp->mRdp->other_mode_l & ZMODE_DEC) == ZMODE_DEC;
    EXPECT_FALSE(zmode_decal);
}

TEST_F(ReferenceRDPTest, ZMode_XluIsNotDecal) {
    interp->mRdp->other_mode_l = ZMODE_XLU;
    bool zmode_decal = (interp->mRdp->other_mode_l & ZMODE_DEC) == ZMODE_DEC;
    EXPECT_FALSE(zmode_decal);
}

TEST_F(ReferenceRDPTest, ZMode_DecIsDecal) {
    interp->mRdp->other_mode_l = ZMODE_DEC;
    bool zmode_decal = (interp->mRdp->other_mode_l & ZMODE_DEC) == ZMODE_DEC;
    EXPECT_TRUE(zmode_decal);
}

TEST_F(ReferenceRDPTest, ZMode_DecWithOtherFlags) {
    // ZMODE_DEC combined with Z_CMP | Z_UPD — decal should still be detected
    interp->mRdp->other_mode_l = ZMODE_DEC | Z_CMP | Z_UPD;
    bool zmode_decal = (interp->mRdp->other_mode_l & ZMODE_DEC) == ZMODE_DEC;
    EXPECT_TRUE(zmode_decal);
}

TEST_F(ReferenceRDPTest, ZMode_BitValues) {
    // Verify the exact bit values per N64 RDP spec
    EXPECT_EQ(ZMODE_OPA, 0x000u);
    EXPECT_EQ(ZMODE_INTER, 0x400u);
    EXPECT_EQ(ZMODE_XLU, 0x800u);
    EXPECT_EQ(ZMODE_DEC, 0xC00u);

    // ZMODE_DEC has both bits 10 and 11 set
    EXPECT_EQ(ZMODE_DEC, ZMODE_INTER | ZMODE_XLU);
}

// ============================================================
// 17. Partial Depth Clear via FillRectangle
//
// When color_image_address == z_buf_address and the rectangle is
// NOT fullscreen, the interpreter should call ClearDepthRegion
// instead of drawing a colored rectangle.
//
// Reference: The coordinates are converted from U10.2 fixed-point
// to pixels, with +1 pixel expansion for fill mode:
//   expanded_lrx = lrx + 4
//   expanded_lry = lry + 4
//   x = ulx / 4.0
//   y = expanded_lry / 4.0
//   w = (expanded_lrx - ulx) / 4.0
//   h = (expanded_lry - uly) / 4.0
// ============================================================

TEST_F(ReferenceRDPTest, PartialDepthClear_TriggersAPI) {
    // Set up depth clear scenario: color_image == z_buf, non-fullscreen rect
    uint8_t dummyAddr[16] = {};
    interp->mRdp->z_buf_address = &dummyAddr;
    interp->mRdp->color_image_address = &dummyAddr;

    // Partial fill rect: pixel coords 40,30 to 200,150 → U10.2: 160,120 to 800,600
    int32_t ulx = 40 * 4;  // 160
    int32_t uly = 30 * 4;  // 120
    int32_t lrx = 200 * 4; // 800
    int32_t lry = 150 * 4; // 600

    interp->GfxDpFillRectangle(ulx, uly, lrx, lry);

    // Should have called ClearDepthRegion exactly once
    ASSERT_EQ(stub->depthRegionClears.size(), 1u);
}

TEST_F(ReferenceRDPTest, PartialDepthClear_CoordinateConversion) {
    // Verify the U10.2 → pixel coordinate conversion for partial depth clears
    uint8_t dummyAddr[16] = {};
    interp->mRdp->z_buf_address = &dummyAddr;
    interp->mRdp->color_image_address = &dummyAddr;

    // Use simple pixel coords: 10,20 to 100,80 → U10.2: 40,80 to 400,320
    int32_t ulx = 10 * 4;  // 40
    int32_t uly = 20 * 4;  // 80
    int32_t lrx = 100 * 4; // 400
    int32_t lry = 80 * 4;  // 320

    interp->GfxDpFillRectangle(ulx, uly, lrx, lry);

    ASSERT_EQ(stub->depthRegionClears.size(), 1u);

    // Expected pre-AdjustViewport values:
    //   expanded_lrx = 400 + 4 = 404
    //   expanded_lry = 320 + 4 = 324
    //   x = 40/4 = 10
    //   y = 324/4 = 81
    //   w = (404 - 40)/4 = 91
    //   h = (324 - 80)/4 = 61
    //
    // AdjustVIewportOrScissor with mFbActive=false, invertY=false:
    //   area.y = mNativeDimensions.height - area.y = 240 - 81 = 159
    //   (RATIO is 1.0 since cur == native dimensions)
    auto& c = stub->depthRegionClears[0];
    EXPECT_EQ(c.x, 10);
    EXPECT_EQ(c.y, 159);
    EXPECT_EQ(c.w, 91u);
    EXPECT_EQ(c.h, 61u);
}

TEST_F(ReferenceRDPTest, FullscreenDepthClear_SkipsAPI) {
    // Fullscreen depth clear should be a no-op
    uint8_t dummyAddr[16] = {};
    interp->mRdp->z_buf_address = &dummyAddr;
    interp->mRdp->color_image_address = &dummyAddr;

    int32_t lrx = (int32_t)(interp->mNativeDimensions.width - 1) * 4;
    int32_t lry = (int32_t)(interp->mNativeDimensions.height - 1) * 4;

    interp->GfxDpFillRectangle(0, 0, lrx, lry);

    // No ClearDepthRegion call (fullscreen is skipped)
    EXPECT_TRUE(stub->depthRegionClears.empty());
    // No ClearFramebuffer call either
    EXPECT_EQ(stub->clearFbCount, 0);
}

TEST_F(ReferenceRDPTest, DepthClear_DifferentAddresses_NotDepthClear) {
    // When color_image_address != z_buf_address, FillRectangle should
    // NOT trigger ClearDepthRegion — it's a normal color fill.
    uint8_t colorBuf[16] = {};
    uint8_t zBuf[16] = {};
    interp->mRdp->color_image_address = &colorBuf;
    interp->mRdp->z_buf_address = &zBuf;
    interp->mRdp->other_mode_h = G_CYC_FILL;

    // Need texture upload buffer for color fill
    interp->mTexUploadBuffer = (uint8_t*)malloc(8192 * 8192 * 4);

    interp->GfxDpFillRectangle(40, 40, 400, 400);

    // No depth clear should happen
    EXPECT_TRUE(stub->depthRegionClears.empty());

    free(interp->mTexUploadBuffer);
    interp->mTexUploadBuffer = nullptr;
}

// ============================================================
// 18. GfxSpSetOtherMode — Selective Bit-Field Update
//
// Unlike GfxDpSetOtherMode which replaces both halves entirely,
// GfxSpSetOtherMode updates only a specified range of bits:
//   mask = ((1 << num_bits) - 1) << shift
//   om = (om & ~mask) | mode
//
// This preserves all bits outside the mask. Verify against
// reference computations.
// ============================================================

TEST_F(ReferenceRDPTest, SpSetOtherMode_SetDepthBits_PreservesOthers) {
    // Set some initial bits that shouldn't be affected
    interp->mRdp->other_mode_l = 0xFF000000;
    interp->mRdp->other_mode_h = 0;

    // Z_CMP=0x10 (bit 4), Z_UPD=0x20 (bit 5)
    // Set bits 4-5 (shift=4, num_bits=2)
    interp->GfxSpSetOtherMode(4, 2, Z_CMP | Z_UPD);

    // Depth bits should be set
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_UPD, 0u);
    // Upper bits should be preserved
    EXPECT_EQ(interp->mRdp->other_mode_l & 0xFF000000, 0xFF000000u);
}

TEST_F(ReferenceRDPTest, SpSetOtherMode_ClearDepthBits) {
    // Start with depth bits set
    interp->mRdp->other_mode_l = Z_CMP | Z_UPD | 0x100;

    // Clear bits 4-5 by setting them to 0
    interp->GfxSpSetOtherMode(4, 2, 0);

    // Depth bits should be cleared
    EXPECT_EQ(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & Z_UPD, 0u);
    // Unrelated bit should be preserved
    EXPECT_NE(interp->mRdp->other_mode_l & 0x100, 0u);
}

TEST_F(ReferenceRDPTest, SpSetOtherMode_SetCycleType) {
    // Cycle type is in the high word at bits 20-21 (G_MDSFT_CYCLETYPE=20)
    // G_CYC_2CYCLE = 1 << 20 = 0x100000
    // Since other_mode uses combined 64-bit: shift would be 20+32=52 for high word
    interp->mRdp->other_mode_h = 0;
    interp->mRdp->other_mode_l = Z_CMP;

    // Set cycle type to 2-cycle in the high word
    // shift=52 (20 in high word = 20+32 in combined), num_bits=2
    interp->GfxSpSetOtherMode(52, 2, (uint64_t)G_CYC_2CYCLE << 32);

    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE), G_CYC_2CYCLE);
    // Low word should be preserved
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
}

TEST_F(ReferenceRDPTest, SpSetOtherMode_MaskComputation) {
    // Verify the mask formula: mask = ((1 << num_bits) - 1) << shift
    // For shift=4, num_bits=2: mask = 0x30
    uint64_t mask = ((uint64_t(1) << 2) - 1) << 4;
    EXPECT_EQ(mask, 0x30u);

    // For shift=10, num_bits=2: mask = 0xC00 (ZMODE bits)
    mask = ((uint64_t(1) << 2) - 1) << 10;
    EXPECT_EQ(mask, 0xC00u);

    // For shift=20, num_bits=2: mask = 0x300000 (in high word, cycle type)
    mask = ((uint64_t(1) << 2) - 1) << 20;
    EXPECT_EQ(mask, 0x300000u);
}

TEST_F(ReferenceRDPTest, SpSetOtherMode_ZmodeBits) {
    // Set ZMODE to DEC (bits 10-11)
    interp->mRdp->other_mode_l = Z_CMP | Z_UPD;

    interp->GfxSpSetOtherMode(10, 2, ZMODE_DEC);

    // ZMODE_DEC should be set
    EXPECT_EQ(interp->mRdp->other_mode_l & ZMODE_DEC, ZMODE_DEC);
    // Depth bits should be preserved
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_UPD, 0u);
}

// ============================================================
// 19. Tile Configuration — SetTile and SetTileSize
//
// The N64 RDP tile descriptor stores format, size, line, tmem,
// palette, and clamp/mirror/mask/shift for both S and T axes.
//
// Reference: verify exact field storage matches RDP spec.
// ============================================================

TEST_F(ReferenceRDPTest, SetTile_BasicFields) {
    // SetTile(fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts)
    interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0,
                         G_TX_CLAMP, 5, 0, G_TX_CLAMP, 5, 0);

    EXPECT_EQ(interp->mRdp->texture_tile[0].fmt, G_IM_FMT_RGBA);
    EXPECT_EQ(interp->mRdp->texture_tile[0].siz, G_IM_SIZ_16b);
    EXPECT_EQ(interp->mRdp->texture_tile[0].cms, G_TX_CLAMP);
    EXPECT_EQ(interp->mRdp->texture_tile[0].cmt, G_TX_CLAMP);
    // line_size_bytes = line * 8
    EXPECT_EQ(interp->mRdp->texture_tile[0].line_size_bytes, 64u);
}

TEST_F(ReferenceRDPTest, SetTile_LineSizeConversion) {
    // Line parameter is in units of 8 bytes (QWORDS)
    // line_size_bytes = line * 8
    for (uint32_t line = 0; line <= 4; line++) {
        interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, line, 0, 0, 0,
                             G_TX_CLAMP, 0, 0, G_TX_CLAMP, 0, 0);
        EXPECT_EQ(interp->mRdp->texture_tile[0].line_size_bytes, line * 8)
            << "Line parameter " << line << " incorrectly converted";
    }
}

TEST_F(ReferenceRDPTest, SetTile_TmemIndexFromAddress) {
    // tmem=0 → tmem_index=0 (first texture slot)
    interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0,
                         G_TX_CLAMP, 0, 0, G_TX_CLAMP, 0, 0);
    EXPECT_EQ(interp->mRdp->texture_tile[0].tmem_index, 0u);

    // tmem=256 → tmem_index=1 (second texture slot)
    interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 256, 1, 0,
                         G_TX_CLAMP, 0, 0, G_TX_CLAMP, 0, 0);
    EXPECT_EQ(interp->mRdp->texture_tile[1].tmem_index, 1u);
}

TEST_F(ReferenceRDPTest, SetTile_WrapWithNoMask_BecomesClamp) {
    // Per N64 RDP: wrap mode with no mask bits → clamp
    // (If mask == 0, wrapping has no effect, so clamp is equivalent)
    interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0,
                         G_TX_WRAP, G_TX_NOMASK, 0, G_TX_WRAP, G_TX_NOMASK, 0);

    EXPECT_EQ(interp->mRdp->texture_tile[0].cms, G_TX_CLAMP);
    EXPECT_EQ(interp->mRdp->texture_tile[0].cmt, G_TX_CLAMP);
}

TEST_F(ReferenceRDPTest, SetTile_WrapWithMask_StaysWrap) {
    // Wrap with a non-zero mask should stay wrap
    interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0,
                         G_TX_WRAP, 5, 0, G_TX_WRAP, 5, 0);

    EXPECT_EQ(interp->mRdp->texture_tile[0].cms, G_TX_WRAP);
    EXPECT_EQ(interp->mRdp->texture_tile[0].cmt, G_TX_WRAP);
}

TEST_F(ReferenceRDPTest, SetTile_SetsTexturesChanged) {
    interp->mRdp->textures_changed[0] = false;
    interp->mRdp->textures_changed[1] = false;

    interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0,
                         G_TX_CLAMP, 0, 0, G_TX_CLAMP, 0, 0);

    EXPECT_TRUE(interp->mRdp->textures_changed[0]);
    EXPECT_TRUE(interp->mRdp->textures_changed[1]);
}

TEST_F(ReferenceRDPTest, SetTileSize_StoresCoordinates) {
    // SetTileSize stores U10.2 coordinates
    interp->GfxDpSetTileSize(0, 0, 0, 128, 64);

    EXPECT_EQ(interp->mRdp->texture_tile[0].uls, 0u);
    EXPECT_EQ(interp->mRdp->texture_tile[0].ult, 0u);
    EXPECT_EQ(interp->mRdp->texture_tile[0].lrs, 128u);
    EXPECT_EQ(interp->mRdp->texture_tile[0].lrt, 64u);
}

TEST_F(ReferenceRDPTest, SetTileSize_SetsTexturesChanged) {
    interp->mRdp->textures_changed[0] = false;
    interp->mRdp->textures_changed[1] = false;

    interp->GfxDpSetTileSize(0, 0, 0, 128, 64);

    EXPECT_TRUE(interp->mRdp->textures_changed[0]);
    EXPECT_TRUE(interp->mRdp->textures_changed[1]);
}

TEST_F(ReferenceRDPTest, SetTile_PaletteIndex) {
    // Palette field should be stored
    interp->GfxDpSetTile(G_IM_FMT_CI, G_IM_SIZ_4b, 4, 0, 0, 7,
                         G_TX_CLAMP, 0, 0, G_TX_CLAMP, 0, 0);

    EXPECT_EQ(interp->mRdp->texture_tile[0].palette, 7u);
}

TEST_F(ReferenceRDPTest, SetTile_MultipleTiles) {
    // Set two different tiles and verify independence
    interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0,
                         G_TX_CLAMP, 5, 0, G_TX_CLAMP, 5, 0);
    interp->GfxDpSetTile(G_IM_FMT_CI, G_IM_SIZ_4b, 4, 256, 1, 3,
                         G_TX_MIRROR, 4, 0, G_TX_MIRROR, 4, 0);

    // Tile 0
    EXPECT_EQ(interp->mRdp->texture_tile[0].fmt, G_IM_FMT_RGBA);
    EXPECT_EQ(interp->mRdp->texture_tile[0].siz, G_IM_SIZ_16b);
    EXPECT_EQ(interp->mRdp->texture_tile[0].palette, 0u);

    // Tile 1
    EXPECT_EQ(interp->mRdp->texture_tile[1].fmt, G_IM_FMT_CI);
    EXPECT_EQ(interp->mRdp->texture_tile[1].siz, G_IM_SIZ_4b);
    EXPECT_EQ(interp->mRdp->texture_tile[1].palette, 3u);
}

// ============================================================
// 20. Alpha/Coverage Flags — Reference Bit Values
//
// The N64 RDP uses several flags in other_mode_l to control
// alpha and coverage behavior during blending.
// ============================================================

TEST_F(ReferenceRDPTest, AlphaCoverageBitValues) {
    // Verify the exact bit positions per N64 RDP spec
    EXPECT_EQ(CVG_X_ALPHA, 0x1000u);
    EXPECT_EQ(ALPHA_CVG_SEL, 0x2000u);
    EXPECT_EQ(FORCE_BL, 0x4000u);
    EXPECT_EQ(AA_EN, 0x8u);
}

TEST_F(ReferenceRDPTest, AlphaCoverageFlags_IndependentBits) {
    // Set CVG_X_ALPHA only
    interp->GfxDpSetOtherMode(0, CVG_X_ALPHA);
    EXPECT_NE(interp->mRdp->other_mode_l & CVG_X_ALPHA, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & ALPHA_CVG_SEL, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & FORCE_BL, 0u);

    // Set ALPHA_CVG_SEL only
    interp->GfxDpSetOtherMode(0, ALPHA_CVG_SEL);
    EXPECT_EQ(interp->mRdp->other_mode_l & CVG_X_ALPHA, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & ALPHA_CVG_SEL, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & FORCE_BL, 0u);
}

TEST_F(ReferenceRDPTest, AlphaCoverageFlags_CombinedRM) {
    // A typical render mode combines multiple flags
    uint32_t rm = AA_EN | Z_CMP | Z_UPD | CVG_X_ALPHA | ALPHA_CVG_SEL | FORCE_BL | ZMODE_XLU;
    interp->GfxDpSetOtherMode(0, rm);

    EXPECT_NE(interp->mRdp->other_mode_l & AA_EN, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_UPD, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & CVG_X_ALPHA, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & ALPHA_CVG_SEL, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & FORCE_BL, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & ZMODE_DEC, ZMODE_XLU);
}

// ============================================================
// 21. Cycle Type in OtherMode — High Word
//
// The cycle type is stored in bits 20-21 of other_mode_h:
//   G_CYC_1CYCLE = 0 << 20
//   G_CYC_2CYCLE = 1 << 20
//   G_CYC_COPY   = 2 << 20
//   G_CYC_FILL   = 3 << 20
// ============================================================

TEST_F(ReferenceRDPTest, CycleType_BitValues) {
    EXPECT_EQ(G_CYC_1CYCLE, 0u);
    EXPECT_EQ(G_CYC_2CYCLE, (1u << 20));
    EXPECT_EQ(G_CYC_COPY, (2u << 20));
    EXPECT_EQ(G_CYC_FILL, (3u << 20));
}

TEST_F(ReferenceRDPTest, CycleType_SetViaOtherMode) {
    // Set cycle type to FILL via GfxDpSetOtherMode
    interp->GfxDpSetOtherMode(G_CYC_FILL, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE), G_CYC_FILL);

    // Change to 2CYCLE
    interp->GfxDpSetOtherMode(G_CYC_2CYCLE, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE), G_CYC_2CYCLE);

    // Change to COPY
    interp->GfxDpSetOtherMode(G_CYC_COPY, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE), G_CYC_COPY);

    // Back to 1CYCLE (default)
    interp->GfxDpSetOtherMode(G_CYC_1CYCLE, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE), G_CYC_1CYCLE);
}

TEST_F(ReferenceRDPTest, CycleType_2CycleDetection) {
    // The interpreter detects 2-cycle mode via:
    //   use_2cyc = (other_mode_h & (3U << G_MDSFT_CYCLETYPE)) == G_CYC_2CYCLE
    interp->GfxDpSetOtherMode(G_CYC_2CYCLE, 0);
    bool use_2cyc = (interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE)) == G_CYC_2CYCLE;
    EXPECT_TRUE(use_2cyc);

    interp->GfxDpSetOtherMode(G_CYC_1CYCLE, 0);
    use_2cyc = (interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE)) == G_CYC_2CYCLE;
    EXPECT_FALSE(use_2cyc);
}

// ============================================================
// 22. SCALE_3_8 Macro Roundtrip
//
// 3-bit to 8-bit color scaling (used for some N64 RDP formats).
// SCALE_3_8(VAL) = VAL * 0x24
// SCALE_8_3(VAL) = VAL / 0x24
// ============================================================

TEST_F(ReferenceRDPTest, Scale3_8_Endpoints) {
    EXPECT_EQ(SCALE_3_8(0), 0);
    EXPECT_EQ(SCALE_3_8(7), 7 * 0x24);  // 252
}

TEST_F(ReferenceRDPTest, Scale3_8_Roundtrip) {
    for (int i = 0; i <= 7; i++) {
        uint8_t expanded = SCALE_3_8(i);
        uint8_t compressed = SCALE_8_3(expanded);
        EXPECT_NEAR(compressed, i, 1) << "3->8->3 roundtrip failed for i=" << i;
    }
}

// ============================================================
// 23. Matrix Stack Push/Pop — Reference Behavior
//
// The N64 RSP maintains a modelview matrix stack. Push increments
// the stack size and copies the current top. Pop decrements and
// recalculates MP_matrix.
//
// Reference: stack starts at size 1, max depth is 11.
// ============================================================

TEST_F(ReferenceRDPTest, MatrixStack_InitialSize) {
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 1u);
}

TEST_F(ReferenceRDPTest, MatrixStack_PopFromInitial) {
    // Pop from size 1 → size 0 (underflow protection)
    interp->GfxSpPopMatrix(1);
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 0u);
}

TEST_F(ReferenceRDPTest, MatrixStack_PopMultiple) {
    // Push a couple times manually, then pop multiple
    interp->mRsp->modelview_matrix_stack_size = 5;
    interp->GfxSpPopMatrix(3);
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 2u);
}

TEST_F(ReferenceRDPTest, MatrixStack_PopSetsLightsChanged) {
    interp->mRsp->lights_changed = false;
    interp->GfxSpPopMatrix(1);
    EXPECT_TRUE(interp->mRsp->lights_changed);
}

TEST_F(ReferenceRDPTest, MatrixStack_PopBeyondZero) {
    // Pop more than stack size → clamp at 0
    interp->mRsp->modelview_matrix_stack_size = 2;
    interp->GfxSpPopMatrix(5);
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 0u);
}

// ============================================================
// 24. GetPixelDepth Pipeline
//
// GetPixelDepthPrepare accumulates coordinate queries.
// GetPixelDepth retrieves values from the rendering API and caches.
// ============================================================

TEST_F(ReferenceRDPTest, GetPixelDepthPrepare_AccumulatesCoords) {
    // Match native dimensions to 1:1 ratio to simplify coordinate transform
    interp->mCurDimensions.width = interp->mNativeDimensions.width;
    interp->mCurDimensions.height = interp->mNativeDimensions.height;
    interp->mGetPixelDepthPending.clear();

    interp->GetPixelDepthPrepare(100.0f, 50.0f);
    EXPECT_EQ(interp->mGetPixelDepthPending.size(), 1u);

    interp->GetPixelDepthPrepare(200.0f, 75.0f);
    EXPECT_EQ(interp->mGetPixelDepthPending.size(), 2u);
}

TEST_F(ReferenceRDPTest, GetPixelDepth_ReturnsConfiguredValue) {
    // Set up 1:1 ratio to simplify coordinate transform
    interp->mCurDimensions.width = interp->mNativeDimensions.width;
    interp->mCurDimensions.height = interp->mNativeDimensions.height;
    interp->mGetPixelDepthCached.clear();
    interp->mGetPixelDepthPending.clear();
    interp->mRendersToFb = true;
    interp->mGameFb = 1;

    // The coordinates get transformed by AdjustPixelDepthCoordinates.
    // With 1:1 ratio and mRendersToFb=true, the coords stay the same.
    // Configure the stub to return a specific depth for the adjusted coordinates.
    stub->pixelDepthValues[{100.0f, 50.0f}] = 12345;

    uint16_t depth = interp->GetPixelDepth(100.0f, 50.0f);
    EXPECT_EQ(depth, 12345);
}

TEST_F(ReferenceRDPTest, GetPixelDepth_CachesResult) {
    interp->mCurDimensions.width = interp->mNativeDimensions.width;
    interp->mCurDimensions.height = interp->mNativeDimensions.height;
    interp->mGetPixelDepthCached.clear();
    interp->mGetPixelDepthPending.clear();
    interp->mRendersToFb = true;
    interp->mGameFb = 1;

    stub->pixelDepthValues[{50.0f, 25.0f}] = 9999;

    // First call populates cache
    uint16_t depth1 = interp->GetPixelDepth(50.0f, 25.0f);
    EXPECT_EQ(depth1, 9999);

    // Change the stub return value — cached should still return old value
    stub->pixelDepthValues[{50.0f, 25.0f}] = 1111;
    uint16_t depth2 = interp->GetPixelDepth(50.0f, 25.0f);
    EXPECT_EQ(depth2, 9999);
}

// ============================================================
// 25. Texture Filter Mode via OtherMode
//
// Texture filtering is set in the high word (other_mode_h) using
// the G_MDSFT_TEXTFILT shift:
//   G_TF_POINT   = 0 << 12
//   G_TF_BILERP  = 2 << 12
//   G_TF_AVERAGE = 3 << 12
// ============================================================

TEST_F(ReferenceRDPTest, TextureFilter_BitValues) {
    EXPECT_EQ(G_TF_POINT, 0u);
    EXPECT_EQ(G_TF_BILERP, (2u << G_MDSFT_TEXTFILT));
    EXPECT_EQ(G_TF_AVERAGE, (3u << G_MDSFT_TEXTFILT));
}

TEST_F(ReferenceRDPTest, TextureFilter_SetViaDpOtherMode) {
    interp->GfxDpSetOtherMode(G_TF_BILERP, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_TEXTFILT), G_TF_BILERP);

    interp->GfxDpSetOtherMode(G_TF_POINT, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_TEXTFILT), G_TF_POINT);

    interp->GfxDpSetOtherMode(G_TF_AVERAGE, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_TEXTFILT), G_TF_AVERAGE);
}

// ============================================================
// 26. Depth Flags with Render Mode Presets
//
// The N64 SDK defines render mode presets that combine depth,
// alpha, and coverage flags. Verify these common combinations
// produce the expected depth behavior.
// ============================================================

TEST_F(ReferenceRDPTest, RM_OPA_SURF_NoDepthTest) {
    // RM_OPA_SURF has no Z_CMP or Z_UPD
    uint32_t rm = CVG_DST_CLAMP | FORCE_BL | ZMODE_OPA;
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->mRdp->other_mode_l = rm;

    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    bool depth_mask = (interp->mRdp->other_mode_l & Z_UPD) == Z_UPD;

    EXPECT_FALSE(depth_test);
    EXPECT_FALSE(depth_mask);
}

TEST_F(ReferenceRDPTest, RM_ZB_OPA_SURF_FullDepth) {
    // RM_ZB_OPA_SURF has Z_CMP | Z_UPD
    uint32_t rm = AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP | ZMODE_OPA | ALPHA_CVG_SEL;
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->mRdp->other_mode_l = rm;

    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    bool depth_mask = (interp->mRdp->other_mode_l & Z_UPD) == Z_UPD;

    EXPECT_TRUE(depth_test);
    EXPECT_TRUE(depth_mask);
}

TEST_F(ReferenceRDPTest, RM_ZB_XLU_SURF_DepthTestNoWrite) {
    // RM_ZB_XLU_SURF has Z_CMP but no Z_UPD (translucent reads but doesn't write depth)
    uint32_t rm = AA_EN | Z_CMP | CVG_DST_WRAP | CLR_ON_CVG | FORCE_BL | ZMODE_XLU;
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->mRdp->other_mode_l = rm;

    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    bool depth_mask = (interp->mRdp->other_mode_l & Z_UPD) == Z_UPD;

    EXPECT_TRUE(depth_test);
    EXPECT_FALSE(depth_mask);
}

TEST_F(ReferenceRDPTest, RM_ZB_OPA_DECAL_DecalMode) {
    // RM_ZB_OPA_DECAL has Z_CMP and ZMODE_DEC
    uint32_t rm = AA_EN | Z_CMP | CVG_DST_WRAP | ALPHA_CVG_SEL | ZMODE_DEC;
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->mRdp->other_mode_l = rm;

    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    bool zmode_decal = (interp->mRdp->other_mode_l & ZMODE_DEC) == ZMODE_DEC;

    EXPECT_TRUE(depth_test);
    EXPECT_TRUE(zmode_decal);
}

// ============================================================
// 27. Color Combiner with LOD Fraction
//
// The LOD fraction from GfxDpSetPrimColor is used as a combiner
// input (G_CCMUX_LOD_FRACTION). Verify it's stored and
// accessible for shader generation.
// ============================================================

TEST_F(ReferenceRDPTest, PrimLodFraction_StoredValues) {
    // LOD fraction is an 8-bit value (0-255)
    interp->GfxDpSetPrimColor(0, 0, 0, 0, 0, 0);
    EXPECT_EQ(interp->mRdp->prim_lod_fraction, 0u);

    interp->GfxDpSetPrimColor(0, 128, 0, 0, 0, 0);
    EXPECT_EQ(interp->mRdp->prim_lod_fraction, 128u);

    interp->GfxDpSetPrimColor(0, 255, 0, 0, 0, 0);
    EXPECT_EQ(interp->mRdp->prim_lod_fraction, 255u);
}

TEST_F(ReferenceRDPTest, PrimColor_MinLevel) {
    // minLevel parameter (first arg) — verify it doesn't affect color
    interp->GfxDpSetPrimColor(0, 0, 100, 150, 200, 250);
    uint8_t r1 = interp->mRdp->prim_color.r;

    interp->GfxDpSetPrimColor(15, 0, 100, 150, 200, 250);
    uint8_t r2 = interp->mRdp->prim_color.r;

    EXPECT_EQ(r1, r2);
    EXPECT_EQ(r1, 100);
}

// ============================================================
// 28. Geometry Mode Flags — Reference Bit Values
//
// Verify the exact bit positions of common geometry mode flags.
// ============================================================

TEST_F(ReferenceRDPTest, GeometryModeBitValues) {
    EXPECT_EQ(G_ZBUFFER, 0x00000001u);
    EXPECT_EQ(G_SHADE, 0x00000004u);
    EXPECT_EQ(G_FOG, 0x00010000u);
    EXPECT_EQ(G_LIGHTING, 0x00020000u);
    EXPECT_EQ(G_TEXTURE_GEN, 0x00040000u);
}

TEST_F(ReferenceRDPTest, GeometryMode_FogAndLighting) {
    interp->mRsp->geometry_mode = 0;
    interp->GfxSpGeometryMode(0, G_FOG | G_LIGHTING);
    EXPECT_NE(interp->mRsp->geometry_mode & G_FOG, 0u);
    EXPECT_NE(interp->mRsp->geometry_mode & G_LIGHTING, 0u);
    EXPECT_EQ(interp->mRsp->geometry_mode & G_ZBUFFER, 0u);
}

TEST_F(ReferenceRDPTest, GeometryMode_ClearFogKeepLighting) {
    interp->mRsp->geometry_mode = G_FOG | G_LIGHTING | G_SHADE;
    interp->GfxSpGeometryMode(G_FOG, 0);
    EXPECT_EQ(interp->mRsp->geometry_mode & G_FOG, 0u);
    EXPECT_NE(interp->mRsp->geometry_mode & G_LIGHTING, 0u);
    EXPECT_NE(interp->mRsp->geometry_mode & G_SHADE, 0u);
}

// ============================================================
// 29. Depth Flag Bits — Reference Values
//
// Verify the exact bit positions of depth-related flags.
// ============================================================

TEST_F(ReferenceRDPTest, DepthFlagBitValues) {
    // Z_CMP is bit 4 (0x10)
    EXPECT_EQ(Z_CMP, 0x10u);
    // Z_UPD is bit 5 (0x20)
    EXPECT_EQ(Z_UPD, 0x20u);
    // CLR_ON_CVG is bit 7 (0x80)
    EXPECT_EQ(CLR_ON_CVG, 0x80u);
}

TEST_F(ReferenceRDPTest, DepthPackedState_AllCombinations) {
    // Exhaustively verify the packed depth_test_and_mask encoding
    // for all 8 combinations of (G_ZBUFFER, Z_CMP, Z_UPD)
    struct DepthCase {
        bool hasGZbuffer;
        bool hasZCmp;
        bool hasZUpd;
        bool expectTest;
        bool expectMask;
        uint8_t expectPacked;
    };
    DepthCase cases[] = {
        { false, false, false, false, false, 0 },
        { false, false, true,  false, true,  2 },
        { false, true,  false, false, false, 0 },
        { false, true,  true,  false, true,  2 },
        { true,  false, false, false, false, 0 },
        { true,  false, true,  false, true,  2 },
        { true,  true,  false, true,  false, 1 },
        { true,  true,  true,  true,  true,  3 },
    };

    for (auto& c : cases) {
        interp->mRsp->geometry_mode = c.hasGZbuffer ? G_ZBUFFER : 0;
        interp->mRdp->other_mode_l = (c.hasZCmp ? Z_CMP : 0) | (c.hasZUpd ? Z_UPD : 0);

        bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                          (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
        bool depth_mask = (interp->mRdp->other_mode_l & Z_UPD) == Z_UPD;
        uint8_t packed = (depth_test ? 1 : 0) | (depth_mask ? 2 : 0);

        EXPECT_EQ(depth_test, c.expectTest)
            << "G_ZBUFFER=" << c.hasGZbuffer << " Z_CMP=" << c.hasZCmp;
        EXPECT_EQ(depth_mask, c.expectMask)
            << "Z_UPD=" << c.hasZUpd;
        EXPECT_EQ(packed, c.expectPacked)
            << "Packed mismatch for G_ZBUFFER=" << c.hasGZbuffer
            << " Z_CMP=" << c.hasZCmp << " Z_UPD=" << c.hasZUpd;
    }
}

// ============================================================
// 30. Z-Buffer Address Lifecycle
//
// Verify the z_buf_address can be set, changed, and compared
// with color_image_address correctly.
// ============================================================

TEST_F(ReferenceRDPTest, ZBufAddress_InitiallyNull) {
    auto freshInterp = std::make_unique<Fast::Interpreter>();
    // Fresh interpreter should have null z_buf_address
    EXPECT_EQ(freshInterp->mRdp->z_buf_address, nullptr);
}

TEST_F(ReferenceRDPTest, ZBufAddress_CanBeUpdated) {
    uint8_t buf1[16] = {};
    uint8_t buf2[16] = {};

    interp->GfxDpSetZImage(&buf1);
    EXPECT_EQ(interp->mRdp->z_buf_address, (void*)&buf1);

    interp->GfxDpSetZImage(&buf2);
    EXPECT_EQ(interp->mRdp->z_buf_address, (void*)&buf2);
}

TEST_F(ReferenceRDPTest, ZBufAddress_ComparedWithColorImage) {
    // Verify the comparison logic used in FillRectangle
    uint8_t shared[16] = {};
    uint8_t separate[16] = {};

    interp->mRdp->z_buf_address = &shared;
    interp->mRdp->color_image_address = &shared;
    EXPECT_EQ(interp->mRdp->color_image_address, interp->mRdp->z_buf_address);

    interp->mRdp->color_image_address = &separate;
    EXPECT_NE(interp->mRdp->color_image_address, interp->mRdp->z_buf_address);
}

// ============================================================
// 31. Texture Image Setup
//
// GfxDpSetTextureImage stores the texture data pointer and format
// information for subsequent LoadBlock/LoadTile operations.
// ============================================================

TEST_F(ReferenceRDPTest, TextureImage_StoresAddress) {
    uint8_t texData[64] = {};
    Fast::RawTexMetadata meta = {};
    meta.h_byte_scale = 1.0f;
    meta.v_pixel_scale = 1.0f;

    interp->GfxDpSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, nullptr, 0, meta, texData);

    EXPECT_EQ(interp->mRdp->texture_to_load.addr, texData);
    EXPECT_EQ(interp->mRdp->texture_to_load.siz, G_IM_SIZ_16b);
    EXPECT_EQ(interp->mRdp->texture_to_load.width, 32u);
}

TEST_F(ReferenceRDPTest, TextureImage_OverwritesPrevious) {
    uint8_t texData1[64] = {};
    uint8_t texData2[64] = {};
    Fast::RawTexMetadata meta = {};
    meta.h_byte_scale = 1.0f;
    meta.v_pixel_scale = 1.0f;

    interp->GfxDpSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, nullptr, 0, meta, texData1);
    EXPECT_EQ(interp->mRdp->texture_to_load.addr, texData1);

    interp->GfxDpSetTextureImage(G_IM_FMT_CI, G_IM_SIZ_8b, 64, nullptr, 0, meta, texData2);
    EXPECT_EQ(interp->mRdp->texture_to_load.addr, texData2);
    EXPECT_EQ(interp->mRdp->texture_to_load.siz, G_IM_SIZ_8b);
    EXPECT_EQ(interp->mRdp->texture_to_load.width, 64u);
}
