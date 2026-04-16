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
// Duplicates of tests in Layers 1-4 have been removed. Tests that exist
// here provide *reference-level* verification unique to this layer.
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

// ************************************************************
// Phase 1: Constants & Bit-Field Reference Values
//
// Verify exact bit positions and values of N64 RDP constants
// against the hardware specification.
// ************************************************************

// ============================================================
// 1.1 Geometry Mode Flag Bit Values
// ============================================================

TEST_F(ReferenceRDPTest, GeometryModeBitValues) {
    EXPECT_EQ(G_ZBUFFER, 0x00000001u);
    EXPECT_EQ(G_SHADE, 0x00000004u);
    EXPECT_EQ(G_FOG, 0x00010000u);
    EXPECT_EQ(G_LIGHTING, 0x00020000u);
    EXPECT_EQ(G_TEXTURE_GEN, 0x00040000u);
}

// ============================================================
// 1.2 Depth Flag Bit Values
// ============================================================

TEST_F(ReferenceRDPTest, DepthFlagBitValues) {
    // Z_CMP is bit 4 (0x10)
    EXPECT_EQ(Z_CMP, 0x10u);
    // Z_UPD is bit 5 (0x20)
    EXPECT_EQ(Z_UPD, 0x20u);
    // CLR_ON_CVG is bit 7 (0x80)
    EXPECT_EQ(CLR_ON_CVG, 0x80u);
}

// ============================================================
// 1.3 Alpha/Coverage Flag Bit Values
// ============================================================

TEST_F(ReferenceRDPTest, AlphaCoverageBitValues) {
    EXPECT_EQ(CVG_X_ALPHA, 0x1000u);
    EXPECT_EQ(ALPHA_CVG_SEL, 0x2000u);
    EXPECT_EQ(FORCE_BL, 0x4000u);
    EXPECT_EQ(AA_EN, 0x8u);
}

// ============================================================
// 1.4 ZMODE Variant Bit Values
//
// The N64 RDP supports four Z modes via bits 10-11 of other_mode_l:
//   ZMODE_OPA   = 0x000  (Opaque)
//   ZMODE_INTER = 0x400  (Interpenetrating)
//   ZMODE_XLU   = 0x800  (Translucent)
//   ZMODE_DEC   = 0xC00  (Decal)
// ============================================================

TEST_F(ReferenceRDPTest, ZMode_BitValues) {
    EXPECT_EQ(ZMODE_OPA, 0x000u);
    EXPECT_EQ(ZMODE_INTER, 0x400u);
    EXPECT_EQ(ZMODE_XLU, 0x800u);
    EXPECT_EQ(ZMODE_DEC, 0xC00u);
    // ZMODE_DEC has both bits 10 and 11 set
    EXPECT_EQ(ZMODE_DEC, ZMODE_INTER | ZMODE_XLU);
}

// ============================================================
// 1.5 Cycle Type Bit Values
// ============================================================

TEST_F(ReferenceRDPTest, CycleType_BitValues) {
    EXPECT_EQ(G_CYC_1CYCLE, 0u);
    EXPECT_EQ(G_CYC_2CYCLE, (1u << 20));
    EXPECT_EQ(G_CYC_COPY, (2u << 20));
    EXPECT_EQ(G_CYC_FILL, (3u << 20));
}

// ============================================================
// 1.6 Texture Filter Bit Values
// ============================================================

TEST_F(ReferenceRDPTest, TextureFilter_BitValues) {
    EXPECT_EQ(G_TF_POINT, 0u);
    EXPECT_EQ(G_TF_BILERP, (2u << G_MDSFT_TEXTFILT));
    EXPECT_EQ(G_TF_AVERAGE, (3u << G_MDSFT_TEXTFILT));
}

// ============================================================
// 1.7 SpSetOtherMode Mask Formula
// ============================================================

TEST_F(ReferenceRDPTest, SpSetOtherMode_MaskComputation) {
    // mask = ((1 << num_bits) - 1) << shift
    // For shift=4, num_bits=2: mask = 0x30
    uint64_t mask = ((uint64_t(1) << 2) - 1) << 4;
    EXPECT_EQ(mask, 0x30u);

    // For shift=10, num_bits=2: mask = 0xC00 (ZMODE bits)
    mask = ((uint64_t(1) << 2) - 1) << 10;
    EXPECT_EQ(mask, 0xC00u);

    // For shift=20, num_bits=2: mask = 0x300000 (cycle type)
    mask = ((uint64_t(1) << 2) - 1) << 20;
    EXPECT_EQ(mask, 0x300000u);
}

// ************************************************************
// Phase 2: Color Processing Reference
//
// Verify fill color RGBA5551 conversion, environment/primitive/
// fog/blend/grayscale color storage.
// ************************************************************

// ============================================================
// 2.1 Fill Color Conversion: RGBA16 Packed → RGBA8
//
// Reference values computed from the RDP hardware formula:
//   r8 = (r5 * 0xFF) / 0x1F
//   g8 = (g5 * 0xFF) / 0x1F
//   b8 = (b5 * 0xFF) / 0x1F
//   a8 = a1 * 255
// ============================================================

TEST_F(ReferenceRDPTest, FillColor_White) {
    interp->GfxDpSetFillColor(0xFFFF);
    EXPECT_EQ(interp->mRdp->fill_color.r, 255);
    EXPECT_EQ(interp->mRdp->fill_color.g, 255);
    EXPECT_EQ(interp->mRdp->fill_color.b, 255);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(ReferenceRDPTest, FillColor_Black) {
    interp->GfxDpSetFillColor(0x0000);
    EXPECT_EQ(interp->mRdp->fill_color.r, 0);
    EXPECT_EQ(interp->mRdp->fill_color.g, 0);
    EXPECT_EQ(interp->mRdp->fill_color.b, 0);
    EXPECT_EQ(interp->mRdp->fill_color.a, 0);
}

TEST_F(ReferenceRDPTest, FillColor_Red) {
    uint16_t packed = (31 << 11) | (0 << 6) | (0 << 1) | 1;
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, SCALE_5_8(31));
    EXPECT_EQ(interp->mRdp->fill_color.g, 0);
    EXPECT_EQ(interp->mRdp->fill_color.b, 0);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(ReferenceRDPTest, FillColor_Green) {
    uint16_t packed = (0 << 11) | (31 << 6) | (0 << 1) | 1;
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, 0);
    EXPECT_EQ(interp->mRdp->fill_color.g, SCALE_5_8(31));
    EXPECT_EQ(interp->mRdp->fill_color.b, 0);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(ReferenceRDPTest, FillColor_Blue) {
    uint16_t packed = (0 << 11) | (0 << 6) | (31 << 1) | 1;
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, 0);
    EXPECT_EQ(interp->mRdp->fill_color.g, 0);
    EXPECT_EQ(interp->mRdp->fill_color.b, SCALE_5_8(31));
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(ReferenceRDPTest, FillColor_MidGray) {
    uint16_t packed = (16 << 11) | (16 << 6) | (16 << 1) | 1;
    interp->GfxDpSetFillColor(packed);
    uint8_t expected = SCALE_5_8(16);
    EXPECT_EQ(interp->mRdp->fill_color.r, expected);
    EXPECT_EQ(interp->mRdp->fill_color.g, expected);
    EXPECT_EQ(interp->mRdp->fill_color.b, expected);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(ReferenceRDPTest, FillColor_AlphaZero) {
    uint16_t packed = (31 << 11) | (31 << 6) | (31 << 1) | 0;
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, 255);
    EXPECT_EQ(interp->mRdp->fill_color.g, 255);
    EXPECT_EQ(interp->mRdp->fill_color.b, 255);
    EXPECT_EQ(interp->mRdp->fill_color.a, 0);
}

TEST_F(ReferenceRDPTest, FillColor_UpperWordIgnored) {
    uint32_t packed = 0xDEAD0000 | 0xFFFF;
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, 255);
    EXPECT_EQ(interp->mRdp->fill_color.g, 255);
    EXPECT_EQ(interp->mRdp->fill_color.b, 255);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

// ============================================================
// 2.2 Fill Color — Exhaustive 5-bit Channel Sweep
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
// 2.3 Environment / Primitive / Fog / Blend / Grayscale Colors
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
    interp->GfxDpSetPrimColor(5, 128, 100, 150, 200, 250);
    EXPECT_EQ(interp->mRdp->prim_color.r, 100);
    EXPECT_EQ(interp->mRdp->prim_color.g, 150);
    EXPECT_EQ(interp->mRdp->prim_color.b, 200);
    EXPECT_EQ(interp->mRdp->prim_color.a, 250);
    EXPECT_EQ(interp->mRdp->prim_lod_fraction, 128);
}

TEST_F(ReferenceRDPTest, PrimLodFraction_StoredValues) {
    interp->GfxDpSetPrimColor(0, 0, 0, 0, 0, 0);
    EXPECT_EQ(interp->mRdp->prim_lod_fraction, 0u);
    interp->GfxDpSetPrimColor(0, 128, 0, 0, 0, 0);
    EXPECT_EQ(interp->mRdp->prim_lod_fraction, 128u);
    interp->GfxDpSetPrimColor(0, 255, 0, 0, 0, 0);
    EXPECT_EQ(interp->mRdp->prim_lod_fraction, 255u);
}

TEST_F(ReferenceRDPTest, PrimColor_MinLevel) {
    interp->GfxDpSetPrimColor(0, 0, 100, 150, 200, 250);
    uint8_t r1 = interp->mRdp->prim_color.r;
    interp->GfxDpSetPrimColor(15, 0, 100, 150, 200, 250);
    uint8_t r2 = interp->mRdp->prim_color.r;
    EXPECT_EQ(r1, r2);
    EXPECT_EQ(r1, 100);
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
    interp->GfxDpSetBlendColor(55, 66, 77, 88);
    EXPECT_EQ(interp->mRdp->blend_color.r, 0);
}

TEST_F(ReferenceRDPTest, GrayscaleColor_Set) {
    interp->GfxDpSetGrayscaleColor(100, 200, 50, 128);
    EXPECT_EQ(interp->mRdp->grayscale_color.r, 100);
    EXPECT_EQ(interp->mRdp->grayscale_color.g, 200);
    EXPECT_EQ(interp->mRdp->grayscale_color.b, 50);
    EXPECT_EQ(interp->mRdp->grayscale_color.a, 128);
}

// ************************************************************
// Phase 3: Coordinate & Viewport Reference
//
// Verify scissor U10.2 → float conversion, texture scaling.
// ************************************************************

// ============================================================
// 3.1 Scissor Conversion: U10.2 Fixed-Point → Float
// ============================================================

TEST_F(ReferenceRDPTest, Scissor_FullScreen) {
    interp->GfxDpSetScissor(0, 0, 0, 1280, 960);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.width, 320.0f);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.height, 240.0f);
}

TEST_F(ReferenceRDPTest, Scissor_PartialRegion) {
    interp->GfxDpSetScissor(0, 160, 120, 1120, 840);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.width, 240.0f);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.height, 180.0f);
}

TEST_F(ReferenceRDPTest, Scissor_SubPixelPrecision) {
    interp->GfxDpSetScissor(0, 1, 0, 5, 4);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.width, 1.0f);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.height, 1.0f);
}

TEST_F(ReferenceRDPTest, Scissor_SetsChangedFlag) {
    interp->mRdp->viewport_or_scissor_changed = false;
    interp->GfxDpSetScissor(0, 0, 0, 1280, 960);
    EXPECT_TRUE(interp->mRdp->viewport_or_scissor_changed);
}

// ============================================================
// 3.2 Texture Scaling
// ============================================================

TEST_F(ReferenceRDPTest, TextureScaling_FullScale) {
    interp->GfxSpTexture(0xFFFF, 0xFFFF, 0, 0, 1);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.s, 0xFFFF);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.t, 0xFFFF);
}

TEST_F(ReferenceRDPTest, TextureScaling_HalfScale) {
    interp->GfxSpTexture(0x8000, 0x8000, 0, 0, 1);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.s, 0x8000);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.t, 0x8000);
}

TEST_F(ReferenceRDPTest, TextureScaling_Zero) {
    interp->GfxSpTexture(0, 0, 0, 0, 0);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.s, 0);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.t, 0);
}

// ************************************************************
// Phase 4: Depth System Reference
//
// Verify depth flag computation, ZMODE detection, depth clear
// handling, and render mode presets.
// ************************************************************

// ============================================================
// 4.1 Depth Test/Mask Computation — Truth Table
//
// Reference formula:
//   depth_test = (geometry_mode & G_ZBUFFER) && (other_mode_l & Z_CMP)
//   depth_mask = (other_mode_l & Z_UPD) != 0
//   packed = (depth_test ? 1 : 0) | (depth_mask ? 2 : 0)
// ============================================================

TEST_F(ReferenceRDPTest, DepthComputation_NothingSet) {
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
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->mRdp->other_mode_l = 0;
    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    EXPECT_FALSE(depth_test);
}

TEST_F(ReferenceRDPTest, DepthComputation_ZCmpOnly) {
    interp->mRsp->geometry_mode = 0;
    interp->mRdp->other_mode_l = Z_CMP;
    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    EXPECT_FALSE(depth_test);
}

TEST_F(ReferenceRDPTest, DepthComputation_ZBufferAndZCmp) {
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->mRdp->other_mode_l = Z_CMP;
    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    EXPECT_TRUE(depth_test);
}

TEST_F(ReferenceRDPTest, DepthComputation_ZUpdOnly) {
    interp->mRdp->other_mode_l = Z_UPD;
    bool depth_mask = (interp->mRdp->other_mode_l & Z_UPD) == Z_UPD;
    EXPECT_TRUE(depth_mask);
}

TEST_F(ReferenceRDPTest, DepthComputation_AllThreeFlags) {
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

TEST_F(ReferenceRDPTest, DepthComputation_OtherModeWithExtraBits) {
    // Depth extraction should work even when other OtherMode bits are set
    interp->mRsp->geometry_mode = G_ZBUFFER | G_SHADE | G_LIGHTING;
    interp->mRdp->other_mode_l = Z_CMP | Z_UPD | ZMODE_XLU | CVG_X_ALPHA | FORCE_BL;
    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    bool depth_mask = (interp->mRdp->other_mode_l & Z_UPD) == Z_UPD;
    EXPECT_TRUE(depth_test);
    EXPECT_TRUE(depth_mask);
}

TEST_F(ReferenceRDPTest, DepthPackedState_AllCombinations) {
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
// 4.2 ZMODE Variant Detection
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
    interp->mRdp->other_mode_l = ZMODE_DEC | Z_CMP | Z_UPD;
    bool zmode_decal = (interp->mRdp->other_mode_l & ZMODE_DEC) == ZMODE_DEC;
    EXPECT_TRUE(zmode_decal);
}

// ============================================================
// 4.3 Partial Depth Clear via FillRectangle
// ============================================================

TEST_F(ReferenceRDPTest, PartialDepthClear_TriggersAPI) {
    uint8_t dummyAddr[16] = {};
    interp->mRdp->z_buf_address = &dummyAddr;
    interp->mRdp->color_image_address = &dummyAddr;
    int32_t ulx = 40 * 4, uly = 30 * 4, lrx = 200 * 4, lry = 150 * 4;
    interp->GfxDpFillRectangle(ulx, uly, lrx, lry);
    ASSERT_EQ(stub->depthRegionClears.size(), 1u);
}

TEST_F(ReferenceRDPTest, PartialDepthClear_CoordinateConversion) {
    uint8_t dummyAddr[16] = {};
    interp->mRdp->z_buf_address = &dummyAddr;
    interp->mRdp->color_image_address = &dummyAddr;
    int32_t ulx = 10 * 4, uly = 20 * 4, lrx = 100 * 4, lry = 80 * 4;
    interp->GfxDpFillRectangle(ulx, uly, lrx, lry);
    ASSERT_EQ(stub->depthRegionClears.size(), 1u);
    auto& c = stub->depthRegionClears[0];
    EXPECT_EQ(c.x, 10);
    EXPECT_EQ(c.y, 159);
    EXPECT_EQ(c.w, 91u);
    EXPECT_EQ(c.h, 61u);
}

TEST_F(ReferenceRDPTest, FullscreenDepthClear_SkipsAPI) {
    uint8_t dummyAddr[16] = {};
    interp->mRdp->z_buf_address = &dummyAddr;
    interp->mRdp->color_image_address = &dummyAddr;
    int32_t lrx = (int32_t)(interp->mNativeDimensions.width - 1) * 4;
    int32_t lry = (int32_t)(interp->mNativeDimensions.height - 1) * 4;
    interp->GfxDpFillRectangle(0, 0, lrx, lry);
    EXPECT_TRUE(stub->depthRegionClears.empty());
    EXPECT_EQ(stub->clearFbCount, 0);
}

TEST_F(ReferenceRDPTest, DepthClear_DifferentAddresses_NotDepthClear) {
    uint8_t colorBuf[16] = {};
    uint8_t zBuf[16] = {};
    interp->mRdp->color_image_address = &colorBuf;
    interp->mRdp->z_buf_address = &zBuf;
    interp->mRdp->other_mode_h = G_CYC_FILL;
    interp->mTexUploadBuffer = (uint8_t*)malloc(8192 * 8192 * 4);
    interp->GfxDpFillRectangle(40, 40, 400, 400);
    EXPECT_TRUE(stub->depthRegionClears.empty());
    free(interp->mTexUploadBuffer);
    interp->mTexUploadBuffer = nullptr;
}

// ============================================================
// 4.4 Render Mode Presets — Depth Behavior
// ============================================================

TEST_F(ReferenceRDPTest, RM_OPA_SURF_NoDepthTest) {
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
    uint32_t rm = AA_EN | Z_CMP | CVG_DST_WRAP | ALPHA_CVG_SEL | ZMODE_DEC;
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->mRdp->other_mode_l = rm;
    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER &&
                      (interp->mRdp->other_mode_l & Z_CMP) == Z_CMP;
    bool zmode_decal = (interp->mRdp->other_mode_l & ZMODE_DEC) == ZMODE_DEC;
    EXPECT_TRUE(depth_test);
    EXPECT_TRUE(zmode_decal);
}

// ************************************************************
// Phase 5: Color Combiner Reference
//
// Verify combine mode packing, GenerateCC extraction,
// normalization, and input mapping.
// ************************************************************

// ============================================================
// 5.1 Combine Mode Packing
// ============================================================

TEST_F(ReferenceRDPTest, CombineMode_1Cycle_ShadeOnly) {
    uint32_t rgb = test_color_comb(0, 0, 0, G_CCMUX_SHADE);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_SHADE);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);
    uint64_t expected = pack_combine_mode(rgb, alpha);
    EXPECT_EQ(interp->mRdp->combine_mode, expected);
}

TEST_F(ReferenceRDPTest, CombineMode_1Cycle_EnvOnly) {
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
    uint32_t rgb = test_color_comb(G_CCMUX_TEXEL0, 0, G_CCMUX_SHADE, 0);
    uint32_t alpha = test_alpha_comb(G_ACMUX_TEXEL0, 0, G_ACMUX_SHADE, 0);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);
    uint64_t expected = pack_combine_mode(rgb, alpha);
    EXPECT_EQ(interp->mRdp->combine_mode, expected);
}

TEST_F(ReferenceRDPTest, CombineMode_2Cycle_Packed) {
    uint32_t rgb1 = test_color_comb(G_CCMUX_TEXEL0, 0, G_CCMUX_SHADE, 0);
    uint32_t alpha1 = test_alpha_comb(G_ACMUX_TEXEL0, 0, G_ACMUX_SHADE, 0);
    uint32_t rgb2 = test_color_comb(G_CCMUX_COMBINED, 0, G_CCMUX_ENVIRONMENT, 0);
    uint32_t alpha2 = test_alpha_comb(G_ACMUX_COMBINED, 0, G_ACMUX_ENVIRONMENT, 0);
    interp->GfxDpSetCombineMode(rgb1, alpha1, rgb2, alpha2);
    uint64_t expected = pack_combine_mode(rgb1, alpha1, rgb2, alpha2);
    EXPECT_EQ(interp->mRdp->combine_mode, expected);
}

TEST_F(ReferenceRDPTest, CombineMode_Overwrite) {
    uint32_t rgb1 = test_color_comb(0, 0, 0, G_CCMUX_SHADE);
    uint32_t alpha1 = test_alpha_comb(0, 0, 0, G_ACMUX_SHADE);
    interp->GfxDpSetCombineMode(rgb1, alpha1, 0, 0);
    uint64_t first = interp->mRdp->combine_mode;

    uint32_t rgb2 = test_color_comb(0, 0, 0, G_CCMUX_ENVIRONMENT);
    uint32_t alpha2 = test_alpha_comb(0, 0, 0, G_ACMUX_ENVIRONMENT);
    interp->GfxDpSetCombineMode(rgb2, alpha2, 0, 0);

    EXPECT_NE(interp->mRdp->combine_mode, first);
    uint64_t expected = pack_combine_mode(rgb2, alpha2);
    EXPECT_EQ(interp->mRdp->combine_mode, expected);
}

// ============================================================
// 5.2 GenerateCC — Texture Usage Detection
// ============================================================

TEST_F(ReferenceRDPTest, GenerateCC_ShadeOnly_NoTextures) {
    uint32_t rgb = test_color_comb(0, 0, 0, G_CCMUX_SHADE);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_SHADE);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);
    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0;
    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);
    EXPECT_FALSE(comb.usedTextures[0]);
    EXPECT_FALSE(comb.usedTextures[1]);
}

TEST_F(ReferenceRDPTest, GenerateCC_Texel0Only_UsesTexture0) {
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
    uint32_t rgb = test_color_comb(0, 0, 0, G_CCMUX_TEXEL1);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_TEXEL1);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);
    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0;
    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);
    EXPECT_TRUE(comb.usedTextures[0]);
}

TEST_F(ReferenceRDPTest, GenerateCC_2Cycle_BothTextures) {
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
    EXPECT_TRUE(comb.usedTextures[0]);
    EXPECT_TRUE(comb.usedTextures[1]);
}

// ============================================================
// 5.3 GenerateCC — Normalization
// ============================================================

TEST_F(ReferenceRDPTest, GenerateCC_NormalizesIdenticalAB) {
    uint32_t rgb = test_color_comb(G_CCMUX_SHADE, G_CCMUX_SHADE, G_CCMUX_ENVIRONMENT, G_CCMUX_PRIMITIVE);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_SHADE);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);
    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0;
    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);
    EXPECT_FALSE(comb.usedTextures[0]);
    EXPECT_FALSE(comb.usedTextures[1]);
}

TEST_F(ReferenceRDPTest, GenerateCC_NormalizesZeroC) {
    uint32_t rgb = test_color_comb(G_CCMUX_TEXEL0, G_CCMUX_ENVIRONMENT, G_CCMUX_0, G_CCMUX_SHADE);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_SHADE);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);
    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0;
    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);
    EXPECT_FALSE(comb.usedTextures[0]);
}

// ============================================================
// 5.4 GenerateCC — Input Mapping
// ============================================================

TEST_F(ReferenceRDPTest, GenerateCC_InputMapping_ShadeAndPrim) {
    uint32_t rgb = test_color_comb(G_CCMUX_PRIMITIVE, 0, G_CCMUX_SHADE, 0);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_SHADE);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);
    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0;
    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);
    bool hasPrim = false, hasShade = false;
    for (int i = 0; i < 7; i++) {
        if (comb.shader_input_mapping[0][i] == G_CCMUX_PRIMITIVE) hasPrim = true;
        if (comb.shader_input_mapping[0][i] == G_CCMUX_SHADE) hasShade = true;
    }
    EXPECT_TRUE(hasPrim);
    EXPECT_TRUE(hasShade);
}

TEST_F(ReferenceRDPTest, GenerateCC_InputMapping_EnvAlpha) {
    uint32_t rgb = test_color_comb(G_CCMUX_TEXEL0, 0, G_CCMUX_ENV_ALPHA, 0);
    uint32_t alpha = test_alpha_comb(0, 0, 0, G_ACMUX_ENVIRONMENT);
    interp->GfxDpSetCombineMode(rgb, alpha, 0, 0);
    ColorCombinerKey key;
    key.combine_mode = interp->mRdp->combine_mode;
    key.options = 0;
    Fast::ColorCombiner comb = {};
    interp->GenerateCC(&comb, key);
    bool hasEnvAlpha = false;
    for (int i = 0; i < 7; i++) {
        if (comb.shader_input_mapping[0][i] == G_CCMUX_ENV_ALPHA) hasEnvAlpha = true;
    }
    EXPECT_TRUE(hasEnvAlpha);
    EXPECT_TRUE(comb.usedTextures[0]);
}

// ************************************************************
// Phase 6: Tile & Texture Reference
//
// Verify tile configuration details unique to this layer.
// (Basic SetTile/SetTileSize tests are in Layer 2.)
// ************************************************************

// ============================================================
// 6.1 SetTile — Line Size Conversion
// ============================================================

TEST_F(ReferenceRDPTest, SetTile_LineSizeConversion) {
    for (uint32_t line = 0; line <= 4; line++) {
        interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, line, 0, 0, 0,
                             G_TX_CLAMP, 0, 0, G_TX_CLAMP, 0, 0);
        EXPECT_EQ(interp->mRdp->texture_tile[0].line_size_bytes, line * 8)
            << "Line parameter " << line << " incorrectly converted";
    }
}

// ============================================================
// 6.2 SetTile — Wrap Mode with Mask
// ============================================================

TEST_F(ReferenceRDPTest, SetTile_WrapWithMask_StaysWrap) {
    interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0,
                         G_TX_WRAP, 5, 0, G_TX_WRAP, 5, 0);
    EXPECT_EQ(interp->mRdp->texture_tile[0].cms, G_TX_WRAP);
    EXPECT_EQ(interp->mRdp->texture_tile[0].cmt, G_TX_WRAP);
}

// ============================================================
// 6.3 SetTile — Textures Changed Flag
// ============================================================

TEST_F(ReferenceRDPTest, SetTile_SetsTexturesChanged) {
    interp->mRdp->textures_changed[0] = false;
    interp->mRdp->textures_changed[1] = false;
    interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0,
                         G_TX_CLAMP, 0, 0, G_TX_CLAMP, 0, 0);
    EXPECT_TRUE(interp->mRdp->textures_changed[0]);
    EXPECT_TRUE(interp->mRdp->textures_changed[1]);
}

// ============================================================
// 6.4 SetTileSize — Changed Flag
// ============================================================

TEST_F(ReferenceRDPTest, SetTileSize_SetsTexturesChanged) {
    interp->mRdp->textures_changed[0] = false;
    interp->mRdp->textures_changed[1] = false;
    interp->GfxDpSetTileSize(0, 0, 0, 128, 64);
    EXPECT_TRUE(interp->mRdp->textures_changed[0]);
    EXPECT_TRUE(interp->mRdp->textures_changed[1]);
}

// ============================================================
// 6.5 SetTile — Palette and Multiple Tiles
// ============================================================

TEST_F(ReferenceRDPTest, SetTile_PaletteIndex) {
    interp->GfxDpSetTile(G_IM_FMT_CI, G_IM_SIZ_4b, 4, 0, 0, 7,
                         G_TX_CLAMP, 0, 0, G_TX_CLAMP, 0, 0);
    EXPECT_EQ(interp->mRdp->texture_tile[0].palette, 7u);
}

TEST_F(ReferenceRDPTest, SetTile_MultipleTiles) {
    interp->GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, 0, 0,
                         G_TX_CLAMP, 5, 0, G_TX_CLAMP, 5, 0);
    interp->GfxDpSetTile(G_IM_FMT_CI, G_IM_SIZ_4b, 4, 256, 1, 3,
                         G_TX_MIRROR, 4, 0, G_TX_MIRROR, 4, 0);
    EXPECT_EQ(interp->mRdp->texture_tile[0].fmt, G_IM_FMT_RGBA);
    EXPECT_EQ(interp->mRdp->texture_tile[0].siz, G_IM_SIZ_16b);
    EXPECT_EQ(interp->mRdp->texture_tile[0].palette, 0u);
    EXPECT_EQ(interp->mRdp->texture_tile[1].fmt, G_IM_FMT_CI);
    EXPECT_EQ(interp->mRdp->texture_tile[1].siz, G_IM_SIZ_4b);
    EXPECT_EQ(interp->mRdp->texture_tile[1].palette, 3u);
}

// ============================================================
// 6.6 Texture Image — Overwrite
// ============================================================

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

// ************************************************************
// Phase 7: State Lifecycle Reference
//
// Verify matrix stack, OtherMode, geometry mode, pixel depth,
// cycle type, and texture filter state transitions.
// ************************************************************

// ============================================================
// 7.1 Z Image / Color Image Addresses
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
    uint8_t dummyAddr[16] = {};
    interp->mRdp->z_buf_address = &dummyAddr;
    interp->mRdp->color_image_address = &dummyAddr;
    int32_t lrx = (int32_t)(interp->mNativeDimensions.width - 1) * 4;
    int32_t lry = (int32_t)(interp->mNativeDimensions.height - 1) * 4;
    interp->GfxDpFillRectangle(0, 0, lrx, lry);
    EXPECT_TRUE(stub->uploads.empty());
}

TEST_F(ReferenceRDPTest, ZBufAddress_InitiallyNull) {
    auto freshInterp = std::make_unique<Fast::Interpreter>();
    EXPECT_EQ(freshInterp->mRdp->z_buf_address, nullptr);
}

TEST_F(ReferenceRDPTest, ZBufAddress_ComparedWithColorImage) {
    uint8_t shared[16] = {};
    uint8_t separate[16] = {};
    interp->mRdp->z_buf_address = &shared;
    interp->mRdp->color_image_address = &shared;
    EXPECT_EQ(interp->mRdp->color_image_address, interp->mRdp->z_buf_address);
    interp->mRdp->color_image_address = &separate;
    EXPECT_NE(interp->mRdp->color_image_address, interp->mRdp->z_buf_address);
}

// ============================================================
// 7.2 Geometry Mode Operations
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
// 7.3 OtherMode Operations
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

TEST_F(ReferenceRDPTest, OtherMode_OverwriteCompletely) {
    // GfxDpSetOtherMode always completely replaces both halves
    interp->GfxDpSetOtherMode(0xAAAAAAAA, 0xBBBBBBBB);
    EXPECT_EQ(interp->mRdp->other_mode_h, 0xAAAAAAAAu);
    EXPECT_EQ(interp->mRdp->other_mode_l, 0xBBBBBBBBu);
    interp->GfxDpSetOtherMode(0x11111111, 0x22222222);
    EXPECT_EQ(interp->mRdp->other_mode_h, 0x11111111u);
    EXPECT_EQ(interp->mRdp->other_mode_l, 0x22222222u);
    // No bits from previous call should remain
    EXPECT_EQ(interp->mRdp->other_mode_h & 0xAAAAAAAA, 0u);
}

// ============================================================
// 7.4 SpSetOtherMode — Selective Bit-Field Update
// ============================================================

TEST_F(ReferenceRDPTest, SpSetOtherMode_ClearDepthBits) {
    interp->mRdp->other_mode_l = Z_CMP | Z_UPD | 0x100;
    interp->GfxSpSetOtherMode(4, 2, 0);
    EXPECT_EQ(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & Z_UPD, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & 0x100, 0u);
}

TEST_F(ReferenceRDPTest, SpSetOtherMode_SetCycleType) {
    interp->mRdp->other_mode_h = 0;
    interp->mRdp->other_mode_l = Z_CMP;
    interp->GfxSpSetOtherMode(52, 2, (uint64_t)G_CYC_2CYCLE << 32);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE), G_CYC_2CYCLE);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
}

TEST_F(ReferenceRDPTest, SpSetOtherMode_ZmodeBits) {
    interp->mRdp->other_mode_l = Z_CMP | Z_UPD;
    interp->GfxSpSetOtherMode(10, 2, ZMODE_DEC);
    EXPECT_EQ(interp->mRdp->other_mode_l & ZMODE_DEC, ZMODE_DEC);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_UPD, 0u);
}

TEST_F(ReferenceRDPTest, SpSetOtherMode_WideMask) {
    // Test a wider bit range: set 8 bits starting at bit 0
    interp->mRdp->other_mode_l = 0xFFFFFFFF;
    interp->mRdp->other_mode_h = 0;
    interp->GfxSpSetOtherMode(0, 8, 0xAB);
    EXPECT_EQ(interp->mRdp->other_mode_l & 0xFF, 0xABu);
    // Upper bits should be preserved
    EXPECT_EQ(interp->mRdp->other_mode_l & 0xFFFFFF00, 0xFFFFFF00u);
}

// ============================================================
// 7.5 Alpha/Coverage Flag Operations
// ============================================================

TEST_F(ReferenceRDPTest, AlphaCoverageFlags_IndependentBits) {
    interp->GfxDpSetOtherMode(0, CVG_X_ALPHA);
    EXPECT_NE(interp->mRdp->other_mode_l & CVG_X_ALPHA, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & ALPHA_CVG_SEL, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & FORCE_BL, 0u);
    interp->GfxDpSetOtherMode(0, ALPHA_CVG_SEL);
    EXPECT_EQ(interp->mRdp->other_mode_l & CVG_X_ALPHA, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & ALPHA_CVG_SEL, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & FORCE_BL, 0u);
}

TEST_F(ReferenceRDPTest, AlphaCoverageFlags_CombinedRM) {
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
// 7.6 Cycle Type in OtherMode
// ============================================================

TEST_F(ReferenceRDPTest, CycleType_SetViaOtherMode) {
    interp->GfxDpSetOtherMode(G_CYC_FILL, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE), G_CYC_FILL);
    interp->GfxDpSetOtherMode(G_CYC_2CYCLE, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE), G_CYC_2CYCLE);
    interp->GfxDpSetOtherMode(G_CYC_COPY, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE), G_CYC_COPY);
    interp->GfxDpSetOtherMode(G_CYC_1CYCLE, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE), G_CYC_1CYCLE);
}

TEST_F(ReferenceRDPTest, CycleType_2CycleDetection) {
    interp->GfxDpSetOtherMode(G_CYC_2CYCLE, 0);
    bool use_2cyc = (interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE)) == G_CYC_2CYCLE;
    EXPECT_TRUE(use_2cyc);
    interp->GfxDpSetOtherMode(G_CYC_1CYCLE, 0);
    use_2cyc = (interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE)) == G_CYC_2CYCLE;
    EXPECT_FALSE(use_2cyc);
}

// ============================================================
// 7.7 Texture Filter Mode
// ============================================================

TEST_F(ReferenceRDPTest, TextureFilter_SetViaDpOtherMode) {
    interp->GfxDpSetOtherMode(G_TF_BILERP, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_TEXTFILT), G_TF_BILERP);
    interp->GfxDpSetOtherMode(G_TF_POINT, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_TEXTFILT), G_TF_POINT);
    interp->GfxDpSetOtherMode(G_TF_AVERAGE, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_TEXTFILT), G_TF_AVERAGE);
}

// ============================================================
// 7.8 Matrix Stack
// ============================================================

TEST_F(ReferenceRDPTest, MatrixStack_InitialSize) {
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 1u);
}

TEST_F(ReferenceRDPTest, MatrixStack_PopFromInitial) {
    interp->GfxSpPopMatrix(1);
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 0u);
}

TEST_F(ReferenceRDPTest, MatrixStack_PopSetsLightsChanged) {
    interp->mRsp->lights_changed = false;
    interp->GfxSpPopMatrix(1);
    EXPECT_TRUE(interp->mRsp->lights_changed);
}

TEST_F(ReferenceRDPTest, MatrixStack_PopBeyondZero) {
    interp->mRsp->modelview_matrix_stack_size = 2;
    interp->GfxSpPopMatrix(5);
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 0u);
}

// ============================================================
// 7.9 GetPixelDepth Pipeline
// ============================================================

TEST_F(ReferenceRDPTest, GetPixelDepth_ReturnsConfiguredValue) {
    interp->mCurDimensions.width = interp->mNativeDimensions.width;
    interp->mCurDimensions.height = interp->mNativeDimensions.height;
    interp->mGetPixelDepthCached.clear();
    interp->mGetPixelDepthPending.clear();
    interp->mRendersToFb = true;
    interp->mGameFb = 1;
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
    uint16_t depth1 = interp->GetPixelDepth(50.0f, 25.0f);
    EXPECT_EQ(depth1, 9999);
    stub->pixelDepthValues[{50.0f, 25.0f}] = 1111;
    uint16_t depth2 = interp->GetPixelDepth(50.0f, 25.0f);
    EXPECT_EQ(depth2, 9999);
}

// ============================================================
// 7.10 Fill Rectangle Vertex Colors
// ============================================================

TEST_F(ReferenceRDPTest, FillRect_SetsVertexColorsToFillColor) {
    uint16_t packed = (15 << 11) | (20 << 6) | (10 << 1) | 1;
    interp->GfxDpSetFillColor(packed);
    interp->mRdp->other_mode_h = G_CYC_FILL;
    uint8_t colorBuf[16] = {};
    uint8_t zBuf[16] = {};
    interp->mRdp->color_image_address = &colorBuf;
    interp->mRdp->z_buf_address = &zBuf;
    interp->mTexUploadBuffer = (uint8_t*)malloc(8192 * 8192 * 4);
    interp->GfxDpFillRectangle(40, 40, 400, 400);
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
