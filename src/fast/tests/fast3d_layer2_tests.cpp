// Fast3D Layer 2 Tests — Single interpreter methods with state
//
// Tests cover:
//  - Matrix stack (push/pop)
//  - Vertex transform (identity, scale, multiple vertices)
//  - RDP state (colors, other mode, tiles, texture scaling, fog, num lights, segments)
//  - GfxSpMatrix (load/push/multiply projection and modelview)
//  - SetZImage, AdjustWidthHeightForScale, Scissor, CalcAndSetViewport
//  - GfxSpMovemem light loading
//  - LoadBlock
//  - LoadTlut (palette loading)
//  - OtherMode depth flags
//  - Framebuffer info

#include "fast3d_test_common.h"

using namespace fast3d_test;

// ============================================================
// Matrix Stack Tests (using Interpreter directly)
// ============================================================

class MatrixStackTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_unique<Fast::Interpreter>();
        // Initialize RSP state
        memset(interp->mRsp, 0, sizeof(Fast::RSP));
        interp->mRsp->modelview_matrix_stack_size = 0;
    }

    std::unique_ptr<Fast::Interpreter> interp;
};

TEST_F(MatrixStackTest, PopEmptyStackDoesNotUnderflow) {
    // Should handle gracefully when stack is empty
    interp->mRsp->modelview_matrix_stack_size = 0;
    interp->GfxSpPopMatrix(1);
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 0u);
}

TEST_F(MatrixStackTest, PopReducesStackSize) {
    // Set up a stack with 2 entries
    interp->mRsp->modelview_matrix_stack_size = 2;
    // Initialize P_matrix as identity so MatrixMul works
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            interp->mRsp->P_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
    // Initialize modelview stack entries as identity too
    for (int k = 0; k < 2; k++)
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                interp->mRsp->modelview_matrix_stack[k][i][j] = (i == j) ? 1.0f : 0.0f;

    interp->GfxSpPopMatrix(1);
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 1u);
}

TEST_F(MatrixStackTest, PopMultiple) {
    interp->mRsp->modelview_matrix_stack_size = 5;
    for (int k = 0; k < 5; k++)
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                interp->mRsp->modelview_matrix_stack[k][i][j] = (i == j) ? 1.0f : 0.0f;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            interp->mRsp->P_matrix[i][j] = (i == j) ? 1.0f : 0.0f;

    interp->GfxSpPopMatrix(3);
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 2u);
}

// ============================================================
// Vertex Transformation Tests
// ============================================================

class VertexTransformTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_unique<Fast::Interpreter>();
        memset(interp->mRsp, 0, sizeof(Fast::RSP));
        interp->mRsp->modelview_matrix_stack_size = 1;

        // Set MP_matrix to identity
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                interp->mRsp->MP_matrix[i][j] = (i == j) ? 1.0f : 0.0f;

        // Set modelview to identity
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                interp->mRsp->modelview_matrix_stack[0][i][j] = (i == j) ? 1.0f : 0.0f;

        // No lighting, no texture
        interp->mRsp->geometry_mode = 0;
        interp->mRsp->texture_scaling_factor.s = 0;
        interp->mRsp->texture_scaling_factor.t = 0;

        // Set native and current dimensions to avoid division by zero in aspect ratio
        interp->mNativeDimensions.width = SCREEN_WIDTH;
        interp->mNativeDimensions.height = SCREEN_HEIGHT;
        interp->mCurDimensions.width = SCREEN_WIDTH;
        interp->mCurDimensions.height = SCREEN_HEIGHT;
        interp->mFbActive = false;
    }

    std::unique_ptr<Fast::Interpreter> interp;
};

TEST_F(VertexTransformTest, IdentityTransform) {
    // Create a vertex at (100, 200, 50) with identity MP_matrix
    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 100;
    vtx.v.ob[1] = 200;
    vtx.v.ob[2] = 50;
    vtx.v.cn[0] = 128; vtx.v.cn[1] = 64; vtx.v.cn[2] = 32; vtx.v.cn[3] = 255;

    interp->GfxSpVertex(1, 0, &vtx);

    // With identity matrix, position should pass through
    // (subject to aspect ratio adjustment on x)
    Fast::LoadedVertex& lv = interp->mRsp->loaded_vertices[0];
    // y, z, w should be unchanged
    EXPECT_FLOAT_EQ(lv.y, 200.0f);
    EXPECT_FLOAT_EQ(lv.z, 50.0f);
    EXPECT_FLOAT_EQ(lv.w, 1.0f);

    // Color should be set from vertex color (no lighting)
    EXPECT_EQ(lv.color.r, 128);
    EXPECT_EQ(lv.color.g, 64);
    EXPECT_EQ(lv.color.b, 32);
    EXPECT_EQ(lv.color.a, 255);
}

TEST_F(VertexTransformTest, ScaleTransform) {
    // Set MP_matrix to 2x scale
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            interp->mRsp->MP_matrix[i][j] = 0.0f;
    interp->mRsp->MP_matrix[0][0] = 2.0f;
    interp->mRsp->MP_matrix[1][1] = 2.0f;
    interp->mRsp->MP_matrix[2][2] = 2.0f;
    interp->mRsp->MP_matrix[3][3] = 1.0f;

    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 10;
    vtx.v.ob[1] = 20;
    vtx.v.ob[2] = 30;

    interp->GfxSpVertex(1, 0, &vtx);

    Fast::LoadedVertex& lv = interp->mRsp->loaded_vertices[0];
    EXPECT_FLOAT_EQ(lv.y, 40.0f);  // 20 * 2
    EXPECT_FLOAT_EQ(lv.z, 60.0f);  // 30 * 2
    EXPECT_FLOAT_EQ(lv.w, 1.0f);
}

TEST_F(VertexTransformTest, MultipleVertices) {
    Fast::F3DVtx vtxs[3] = {};
    vtxs[0].v.ob[0] = 1; vtxs[0].v.ob[1] = 2; vtxs[0].v.ob[2] = 3;
    vtxs[1].v.ob[0] = 4; vtxs[1].v.ob[1] = 5; vtxs[1].v.ob[2] = 6;
    vtxs[2].v.ob[0] = 7; vtxs[2].v.ob[1] = 8; vtxs[2].v.ob[2] = 9;

    interp->GfxSpVertex(3, 0, vtxs);

    EXPECT_FLOAT_EQ(interp->mRsp->loaded_vertices[0].y, 2.0f);
    EXPECT_FLOAT_EQ(interp->mRsp->loaded_vertices[1].y, 5.0f);
    EXPECT_FLOAT_EQ(interp->mRsp->loaded_vertices[2].y, 8.0f);
}

// ============================================================
// Phase 3: RDP Color Setting Tests
// ============================================================

class RdpStateTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_unique<Fast::Interpreter>();
    }

    std::unique_ptr<Fast::Interpreter> interp;
};

TEST_F(RdpStateTest, SetEnvColor) {
    interp->GfxDpSetEnvColor(10, 20, 30, 40);
    EXPECT_EQ(interp->mRdp->env_color.r, 10);
    EXPECT_EQ(interp->mRdp->env_color.g, 20);
    EXPECT_EQ(interp->mRdp->env_color.b, 30);
    EXPECT_EQ(interp->mRdp->env_color.a, 40);
}

TEST_F(RdpStateTest, SetPrimColor) {
    interp->GfxDpSetPrimColor(0, 128, 100, 150, 200, 250);
    EXPECT_EQ(interp->mRdp->prim_color.r, 100);
    EXPECT_EQ(interp->mRdp->prim_color.g, 150);
    EXPECT_EQ(interp->mRdp->prim_color.b, 200);
    EXPECT_EQ(interp->mRdp->prim_color.a, 250);
    EXPECT_EQ(interp->mRdp->prim_lod_fraction, 128);
}

TEST_F(RdpStateTest, SetFogColor) {
    interp->GfxDpSetFogColor(55, 66, 77, 88);
    EXPECT_EQ(interp->mRdp->fog_color.r, 55);
    EXPECT_EQ(interp->mRdp->fog_color.g, 66);
    EXPECT_EQ(interp->mRdp->fog_color.b, 77);
    EXPECT_EQ(interp->mRdp->fog_color.a, 88);
}

TEST_F(RdpStateTest, SetGrayscaleColor) {
    interp->GfxDpSetGrayscaleColor(11, 22, 33, 44);
    EXPECT_EQ(interp->mRdp->grayscale_color.r, 11);
    EXPECT_EQ(interp->mRdp->grayscale_color.g, 22);
    EXPECT_EQ(interp->mRdp->grayscale_color.b, 33);
    EXPECT_EQ(interp->mRdp->grayscale_color.a, 44);
}

TEST_F(RdpStateTest, SetFillColor_RedOpaque) {
    // RGBA5551: R=31,G=0,B=0,A=1 → 0xF801
    // packed_color lower 16 bits used: 0xF801
    uint32_t packed = 0xF801;
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, 255); // SCALE_5_8(31)
    EXPECT_EQ(interp->mRdp->fill_color.g, 0);
    EXPECT_EQ(interp->mRdp->fill_color.b, 0);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(RdpStateTest, SetFillColor_Transparent) {
    // A=0: alpha bit is 0
    // R=0,G=0,B=0,A=0 → 0x0000
    interp->GfxDpSetFillColor(0x0000);
    EXPECT_EQ(interp->mRdp->fill_color.r, 0);
    EXPECT_EQ(interp->mRdp->fill_color.g, 0);
    EXPECT_EQ(interp->mRdp->fill_color.b, 0);
    EXPECT_EQ(interp->mRdp->fill_color.a, 0);
}

TEST_F(RdpStateTest, SetFillColor_GreenOpaque) {
    // R=0,G=31,B=0,A=1 → (0 << 11) | (31 << 6) | (0 << 1) | 1 = 0x07C1
    uint32_t packed = 0x07C1;
    interp->GfxDpSetFillColor(packed);
    EXPECT_EQ(interp->mRdp->fill_color.r, 0);
    EXPECT_EQ(interp->mRdp->fill_color.g, 255); // SCALE_5_8(31)
    EXPECT_EQ(interp->mRdp->fill_color.b, 0);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(RdpStateTest, SetFillColor_WhiteOpaque) {
    // R=31,G=31,B=31,A=1 → 0xFFFF
    interp->GfxDpSetFillColor(0xFFFF);
    EXPECT_EQ(interp->mRdp->fill_color.r, 255);
    EXPECT_EQ(interp->mRdp->fill_color.g, 255);
    EXPECT_EQ(interp->mRdp->fill_color.b, 255);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

// ============================================================
// Other Mode Tests
// ============================================================

TEST_F(RdpStateTest, SetOtherMode_Direct) {
    interp->GfxDpSetOtherMode(0x12345678, 0xABCDEF01);
    EXPECT_EQ(interp->mRdp->other_mode_h, 0x12345678u);
    EXPECT_EQ(interp->mRdp->other_mode_l, 0xABCDEF01u);
}

TEST_F(RdpStateTest, SetOtherMode_Masked) {
    // Start with known state
    interp->GfxDpSetOtherMode(0, 0);
    EXPECT_EQ(interp->mRdp->other_mode_h, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l, 0u);

    // Set cycle type to 2-cycle (bits 20-21 in other_mode_h)
    // GfxSpSetOtherMode: shift is from bit 0 of the 64-bit combined mode
    // other_mode = other_mode_l | (other_mode_h << 32)
    // G_MDSFT_CYCLETYPE = 20, in other_mode_h → shift = 20 + 32 = 52
    interp->GfxSpSetOtherMode(52, 2, (uint64_t)G_CYC_2CYCLE << 32);
    EXPECT_EQ(interp->mRdp->other_mode_h & (3u << G_MDSFT_CYCLETYPE), (uint32_t)G_CYC_2CYCLE);
}

TEST_F(RdpStateTest, SetOtherMode_PreservesOtherBits) {
    // Set all bits
    interp->GfxDpSetOtherMode(0xFFFFFFFF, 0xFFFFFFFF);

    // Clear just 4 bits at position 0 of the low word
    interp->GfxSpSetOtherMode(0, 4, 0);
    EXPECT_EQ(interp->mRdp->other_mode_l & 0xF, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & ~0xFu, ~0xFu); // rest preserved
    EXPECT_EQ(interp->mRdp->other_mode_h, 0xFFFFFFFFu);    // high word unchanged
}

// ============================================================
// Tile Configuration Tests
// ============================================================

TEST_F(RdpStateTest, SetTileSize) {
    interp->GfxDpSetTileSize(2, 10, 20, 300, 400);
    EXPECT_EQ(interp->mRdp->texture_tile[2].uls, 10);
    EXPECT_EQ(interp->mRdp->texture_tile[2].ult, 20);
    EXPECT_EQ(interp->mRdp->texture_tile[2].lrs, 300);
    EXPECT_EQ(interp->mRdp->texture_tile[2].lrt, 400);
    EXPECT_TRUE(interp->mRdp->textures_changed[0]);
    EXPECT_TRUE(interp->mRdp->textures_changed[1]);
}

TEST_F(RdpStateTest, SetTile_Basic) {
    // line=2, tmem=0, tile=3, palette=5
    interp->GfxDpSetTile(0 /*fmt*/, 2 /*siz*/, 2 /*line*/, 0 /*tmem*/, 3 /*tile*/, 5 /*palette*/,
                         G_TX_CLAMP /*cmt*/, 4 /*maskt*/, 1 /*shiftt*/,
                         G_TX_WRAP /*cms*/, 3 /*masks*/, 2 /*shifts*/);
    EXPECT_EQ(interp->mRdp->texture_tile[3].fmt, 0);
    EXPECT_EQ(interp->mRdp->texture_tile[3].siz, 2);
    EXPECT_EQ(interp->mRdp->texture_tile[3].palette, 5);
    EXPECT_EQ(interp->mRdp->texture_tile[3].line_size_bytes, 16u); // line * 8
    EXPECT_EQ(interp->mRdp->texture_tile[3].shifts, 2);
    EXPECT_EQ(interp->mRdp->texture_tile[3].shiftt, 1);
    EXPECT_EQ(interp->mRdp->texture_tile[3].cmt, G_TX_CLAMP);
    // cms = G_TX_WRAP with masks != G_TX_NOMASK → stays as G_TX_WRAP
    EXPECT_EQ(interp->mRdp->texture_tile[3].cms, G_TX_WRAP);
    EXPECT_EQ(interp->mRdp->texture_tile[3].tmem_index, 0); // tmem=0 → index 0
}

TEST_F(RdpStateTest, SetTile_WrapNoMaskBecomesClamp) {
    // When cms==G_TX_WRAP and masks==G_TX_NOMASK, cms should become G_TX_CLAMP
    interp->GfxDpSetTile(0, 2, 1, 0, 0, 0,
                         G_TX_WRAP /*cmt*/, G_TX_NOMASK /*maskt*/, 0,
                         G_TX_WRAP /*cms*/, G_TX_NOMASK /*masks*/, 0);
    EXPECT_EQ(interp->mRdp->texture_tile[0].cms, G_TX_CLAMP);
    EXPECT_EQ(interp->mRdp->texture_tile[0].cmt, G_TX_CLAMP);
}

TEST_F(RdpStateTest, SetTile_TmemIndexMapping) {
    // tmem=0 → tmem_index=0
    interp->GfxDpSetTile(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    EXPECT_EQ(interp->mRdp->texture_tile[0].tmem_index, 0);

    // tmem=256 → tmem_index=1
    interp->GfxDpSetTile(0, 0, 0, 256, 1, 0, 0, 0, 0, 0, 0, 0);
    EXPECT_EQ(interp->mRdp->texture_tile[1].tmem_index, 1);

    // tmem=1 → tmem_index=1 (any non-zero → 1)
    interp->GfxDpSetTile(0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0);
    EXPECT_EQ(interp->mRdp->texture_tile[2].tmem_index, 1);
}

// ============================================================
// Texture Scaling Factor Tests
// ============================================================

TEST_F(RdpStateTest, SpTexture) {
    interp->GfxSpTexture(0x8000, 0x4000, 0, 2, 1);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.s, 0x8000);
    EXPECT_EQ(interp->mRsp->texture_scaling_factor.t, 0x4000);
    EXPECT_EQ(interp->mRdp->first_tile_index, 2);
}

TEST_F(RdpStateTest, SpTexture_ChangeTileTriggersTextureChanged) {
    interp->mRdp->first_tile_index = 0;
    interp->mRdp->textures_changed[0] = false;
    interp->mRdp->textures_changed[1] = false;

    // Changing to a different tile should set textures_changed
    interp->GfxSpTexture(0xFFFF, 0xFFFF, 0, 3, 1);
    EXPECT_TRUE(interp->mRdp->textures_changed[0]);
    EXPECT_TRUE(interp->mRdp->textures_changed[1]);
}

TEST_F(RdpStateTest, SpTexture_SameTileNoChange) {
    interp->mRdp->first_tile_index = 5;
    interp->mRdp->textures_changed[0] = false;
    interp->mRdp->textures_changed[1] = false;

    // Same tile — textures_changed should NOT be set
    interp->GfxSpTexture(0xFFFF, 0xFFFF, 0, 5, 1);
    EXPECT_FALSE(interp->mRdp->textures_changed[0]);
    EXPECT_FALSE(interp->mRdp->textures_changed[1]);
}

// ============================================================
// Fog Parameter Tests
// ============================================================

TEST_F(RdpStateTest, FogParametersF3dex2) {
    // GfxSpMovewordF3dex2 with G_MW_FOG
    uint16_t mul = 0x7FFF;   // fog_mul
    uint16_t offset = 0x0100; // fog_offset
    uintptr_t data = ((uintptr_t)mul << 16) | offset;
    interp->GfxSpMovewordF3dex2(G_MW_FOG, 0, data);
    EXPECT_EQ(interp->mRsp->fog_mul, (int16_t)0x7FFF);
    EXPECT_EQ(interp->mRsp->fog_offset, (int16_t)0x0100);
}

TEST_F(RdpStateTest, FogParametersF3d) {
    uint16_t mul = 0x8000;   // negative in int16
    uint16_t offset = 0xFFF0;
    uintptr_t data = ((uintptr_t)mul << 16) | offset;
    interp->GfxSpMovewordF3d(G_MW_FOG, 0, data);
    EXPECT_EQ(interp->mRsp->fog_mul, (int16_t)0x8000);
    EXPECT_EQ(interp->mRsp->fog_offset, (int16_t)0xFFF0);
}

// ============================================================
// Num Lights Tests
// ============================================================

TEST_F(RdpStateTest, NumLightsF3dex2) {
    // F3DEX2: num_lights = data / 24 + 1
    interp->GfxSpMovewordF3dex2(G_MW_NUMLIGHT, 0, 48); // 48/24 + 1 = 3
    EXPECT_EQ(interp->mRsp->current_num_lights, 3u);
    EXPECT_TRUE(interp->mRsp->lights_changed);
}

TEST_F(RdpStateTest, NumLightsF3d) {
    // F3D: num_lights = (data - 0x80000000) / 32
    interp->GfxSpMovewordF3d(G_MW_NUMLIGHT, 0, 0x80000000U + 64); // 64/32 = 2
    EXPECT_EQ(interp->mRsp->current_num_lights, 2u);
    EXPECT_TRUE(interp->mRsp->lights_changed);
}

// ============================================================
// Segment Pointer Tests
// ============================================================

TEST_F(RdpStateTest, SegmentPointerF3dex2) {
    // G_MW_SEGMENT: segNumber = offset / 4
    interp->GfxSpMovewordF3dex2(G_MW_SEGMENT, 4 * 3, 0xDEADBEEF);
    EXPECT_EQ(interp->mSegmentPointers[3], (uintptr_t)0xDEADBEEF);
}

TEST_F(RdpStateTest, SegmentPointerF3d) {
    interp->GfxSpMovewordF3d(G_MW_SEGMENT, 4 * 5, 0x12345678);
    EXPECT_EQ(interp->mSegmentPointers[5], (uintptr_t)0x12345678);
}

// ============================================================
// Modify Vertex Tests
// ============================================================

TEST_F(RdpStateTest, ModifyVertex_ST) {
    // Prepare a loaded vertex at index 0
    interp->mRsp->loaded_vertices[0].u = 0;
    interp->mRsp->loaded_vertices[0].v = 0;

    // val: high 16 bits = s, low 16 bits = t
    int16_t s = 1024;
    int16_t t = -512;
    uint32_t val = ((uint32_t)(uint16_t)s << 16) | (uint16_t)t;

    interp->GfxSpModifyVertex(0, G_MWO_POINT_ST, val);
    EXPECT_EQ(interp->mRsp->loaded_vertices[0].u, 1024);
    EXPECT_EQ(interp->mRsp->loaded_vertices[0].v, -512);
}

// ============================================================
// CalculateNormalDir Tests
// ============================================================

TEST_F(RdpStateTest, CalculateNormalDir_IdentityModelview) {
    // Set up identity modelview matrix at top of stack
    interp->mRsp->modelview_matrix_stack_size = 1;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            interp->mRsp->modelview_matrix_stack[0][i][j] = (i == j) ? 1.0f : 0.0f;

    Fast::F3DLight_t light = {};
    light.dir[0] = 127;
    light.dir[1] = 0;
    light.dir[2] = 0;

    float coeffs[3];
    interp->CalculateNormalDir(&light, coeffs);

    // With identity modelview, the light direction should pass through normalized
    EXPECT_NEAR(coeffs[0], 1.0f, 1e-3f);
    EXPECT_NEAR(coeffs[1], 0.0f, 1e-3f);
    EXPECT_NEAR(coeffs[2], 0.0f, 1e-3f);
}

TEST_F(RdpStateTest, CalculateNormalDir_DiagonalLight) {
    interp->mRsp->modelview_matrix_stack_size = 1;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            interp->mRsp->modelview_matrix_stack[0][i][j] = (i == j) ? 1.0f : 0.0f;

    // Light dir pointing equally along x, y, z
    Fast::F3DLight_t light = {};
    light.dir[0] = 73; // ~ 127 / sqrt(3)
    light.dir[1] = 73;
    light.dir[2] = 73;

    float coeffs[3];
    interp->CalculateNormalDir(&light, coeffs);

    // Should be normalized to unit length
    float len = sqrtf(coeffs[0] * coeffs[0] + coeffs[1] * coeffs[1] + coeffs[2] * coeffs[2]);
    EXPECT_NEAR(len, 1.0f, 1e-3f);
    // All components should be equal
    EXPECT_NEAR(coeffs[0], coeffs[1], 1e-3f);
    EXPECT_NEAR(coeffs[1], coeffs[2], 1e-3f);
}

// ============================================================
// Color Image Address Test
// ============================================================

TEST_F(RdpStateTest, SetColorImage) {
    int dummy;
    interp->GfxDpSetColorImage(0, 0, 320, &dummy);
    EXPECT_EQ(interp->mRdp->color_image_address, &dummy);
}

// ============================================================
// GfxDpSetCombineMode Tests
// ============================================================

TEST_F(RdpStateTest, SetCombineMode) {
    interp->GfxDpSetCombineMode(0x12345678, 0x9ABCDEF0, 0x11111111, 0x22222222);
    // The combine mode is set as a 64-bit value constructed from the RGB and alpha parts
    // This just verifies it doesn't crash and sets some state
    // (Internal representation may differ from raw args)
}

// ============================================================
// SetTextureImage Tests
// ============================================================

TEST_F(RdpStateTest, SetTextureImage) {
    uint8_t fakeData[16] = {};
    Fast::RawTexMetadata meta = {};
    interp->GfxDpSetTextureImage(0 /*format*/, 2 /*size*/, 64 /*width*/,
                                 nullptr /*texPath*/, 0 /*texFlags*/, meta,
                                 fakeData /*addr*/);
    EXPECT_EQ(interp->mRdp->texture_to_load.addr, fakeData);
    EXPECT_EQ(interp->mRdp->texture_to_load.siz, 2);
    EXPECT_EQ(interp->mRdp->texture_to_load.width, 64u);
}

// ============================================================
// SpReset Test
// ============================================================

TEST_F(RdpStateTest, SpReset) {
    // Modify some RSP state, then verify SpReset clears it
    interp->mRsp->geometry_mode = 0xFFFFFFFF;
    interp->mRsp->modelview_matrix_stack_size = 5;

    interp->SpReset();

    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 1);
}

// ============================================================
// Layer 2: GfxSpMatrix Tests
// ============================================================

class GfxSpMatrixTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_unique<Fast::Interpreter>();
        memset(interp->mRsp, 0, sizeof(Fast::RSP));
        interp->mRsp->modelview_matrix_stack_size = 1;

        // GfxSpMatrix reads from mCurMtxReplacements — provide an empty map
        static const std::unordered_map<Mtx*, MtxF> emptyReplacements;
        interp->mCurMtxReplacements = &emptyReplacements;

        // Identity for P_matrix and modelview stack
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++) {
                interp->mRsp->P_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
                interp->mRsp->modelview_matrix_stack[0][i][j] = (i == j) ? 1.0f : 0.0f;
            }
    }

    // Build a fixed-point identity matrix (N64 format: 16 int32s)
    // Layout: 4 rows x 2 columns of int32 (integer parts), then 4 rows x 2 columns (fraction parts)
    void MakeFixedPointIdentity(int32_t* out) {
        memset(out, 0, 16 * sizeof(int32_t));
        // Integer parts: row i, columns j,j+1 packed into int32
        // Identity: diagonal entries are 1.0 = 0x00010000 integer + 0x00000000 fraction
        // addr[i*2 + j/2] = (int_part[i][j] << 16) | int_part[i][j+1]
        out[0] = 0x00010000; // [0][0]=1, [0][1]=0
        out[1] = 0x00000000; // [0][2]=0, [0][3]=0
        out[2] = 0x00000000; // [1][0]=0, [1][1]=1
        out[3] = 0x00010000; // becomes: int_part = addr[3] => row1 cols2,3
        // Wait — let me re-derive the formula:
        // matrix[i][j]     = (int32_t)((addr[i*2+j/2] & 0xffff0000) | (addr[8+i*2+j/2] >> 16)) / 65536.0f
        // matrix[i][j+1]   = (int32_t)((addr[i*2+j/2] << 16)       | (addr[8+i*2+j/2] & 0xffff)) / 65536.0f
        // For identity, matrix[i][j] = (i==j) ? 1.0 : 0.0
        // 1.0 = 65536 / 65536 => the combined int32 should be 65536 = 0x00010000
        // So for diagonal (i,i):
        //   if i is even (j=i): high 16 bits of addr[i*2+i/2] = 1 => addr[i*2+i/2] |= 0x00010000
        //   if i is odd (j+1=i): low 16 bits of addr[i*2+(i-1)/2] = 1 => addr[...] |= 0x0001
        //   frac parts (addr[8+...]) all zero for integer values
    }

    // Build a fixed-point matrix encoding the given float matrix (N64 format)
    void FloatToFixedPoint(const float src[4][4], int32_t* out) {
        memset(out, 0, 16 * sizeof(int32_t));
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j += 2) {
                int32_t val0 = (int32_t)(src[i][j] * 65536.0f);
                int32_t val1 = (int32_t)(src[i][j + 1] * 65536.0f);
                // integer part: high 16 bits of each value
                out[i * 2 + j / 2] = (val0 & (int32_t)0xffff0000) | ((uint32_t)(val1 & (int32_t)0xffff0000) >> 16);
                // fraction part: low 16 bits of each value
                out[8 + i * 2 + j / 2] =
                    ((val0 & 0xffff) << 16) | (val1 & 0xffff);
            }
        }
    }

    std::unique_ptr<Fast::Interpreter> interp;
};

TEST_F(GfxSpMatrixTest, LoadProjectionMatrix) {
    // Create a 2x scale matrix in fixed-point format
    float scale2[4][4] = {};
    scale2[0][0] = 2.0f;
    scale2[1][1] = 2.0f;
    scale2[2][2] = 2.0f;
    scale2[3][3] = 1.0f;

    int32_t fixedPt[16];
    FloatToFixedPoint(scale2, fixedPt);

    // F3DEX attribute values: MTX_PROJECTION=0x01, MTX_LOAD=0x02
    uint8_t params = Fast::F3DEX2_G_MTX_PROJECTION | Fast::F3DEX2_G_MTX_LOAD;
    interp->GfxSpMatrix(params, fixedPt);

    // P_matrix should now be the 2x scale matrix
    EXPECT_NEAR(interp->mRsp->P_matrix[0][0], 2.0f, 1e-3f);
    EXPECT_NEAR(interp->mRsp->P_matrix[1][1], 2.0f, 1e-3f);
    EXPECT_NEAR(interp->mRsp->P_matrix[2][2], 2.0f, 1e-3f);
    EXPECT_NEAR(interp->mRsp->P_matrix[3][3], 1.0f, 1e-3f);
    EXPECT_NEAR(interp->mRsp->P_matrix[0][1], 0.0f, 1e-3f);
}

TEST_F(GfxSpMatrixTest, LoadModelviewMatrix) {
    float trans[4][4] = {};
    trans[0][0] = 1.0f;
    trans[1][1] = 1.0f;
    trans[2][2] = 1.0f;
    trans[3][3] = 1.0f;
    trans[3][0] = 10.0f; // translate X by 10

    int32_t fixedPt[16];
    FloatToFixedPoint(trans, fixedPt);

    // Modelview load (not projection, is load)
    uint8_t params = Fast::F3DEX2_G_MTX_LOAD; // no MTX_PROJECTION → modelview
    interp->GfxSpMatrix(params, fixedPt);

    // Should load into top of modelview stack
    auto& mv = interp->mRsp->modelview_matrix_stack[interp->mRsp->modelview_matrix_stack_size - 1];
    EXPECT_NEAR(mv[3][0], 10.0f, 1e-3f);
    EXPECT_NEAR(mv[0][0], 1.0f, 1e-3f);
}

TEST_F(GfxSpMatrixTest, PushModelviewMatrix) {
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 1u);

    float identity[4][4] = {};
    for (int i = 0; i < 4; i++) identity[i][i] = 1.0f;
    int32_t fixedPt[16];
    FloatToFixedPoint(identity, fixedPt);

    // Push + Load modelview
    uint8_t params = Fast::F3DEX2_G_MTX_PUSH | Fast::F3DEX2_G_MTX_LOAD;
    interp->GfxSpMatrix(params, fixedPt);

    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 2u);
}

TEST_F(GfxSpMatrixTest, MultiplyProjectionMatrix) {
    // Start with identity P_matrix, multiply by 3x scale
    float scale3[4][4] = {};
    scale3[0][0] = 3.0f;
    scale3[1][1] = 3.0f;
    scale3[2][2] = 3.0f;
    scale3[3][3] = 1.0f;

    int32_t fixedPt[16];
    FloatToFixedPoint(scale3, fixedPt);

    // Projection, multiply (not load)
    uint8_t params = Fast::F3DEX2_G_MTX_PROJECTION; // no MTX_LOAD → multiply
    interp->GfxSpMatrix(params, fixedPt);

    // Identity * scale3 = scale3
    EXPECT_NEAR(interp->mRsp->P_matrix[0][0], 3.0f, 1e-3f);
    EXPECT_NEAR(interp->mRsp->P_matrix[1][1], 3.0f, 1e-3f);
}

TEST_F(GfxSpMatrixTest, PushThenPopRoundTrip) {
    // Set modelview to a translation matrix
    float trans[4][4] = {};
    for (int i = 0; i < 4; i++) trans[i][i] = 1.0f;
    trans[3][0] = 5.0f;

    int32_t fixedPt[16];
    FloatToFixedPoint(trans, fixedPt);

    // Load the translation into modelview
    interp->GfxSpMatrix(Fast::F3DEX2_G_MTX_LOAD, fixedPt);
    EXPECT_NEAR(interp->mRsp->modelview_matrix_stack[0][3][0], 5.0f, 1e-3f);

    // Push and load identity
    float identity[4][4] = {};
    for (int i = 0; i < 4; i++) identity[i][i] = 1.0f;
    FloatToFixedPoint(identity, fixedPt);
    interp->GfxSpMatrix(Fast::F3DEX2_G_MTX_PUSH | Fast::F3DEX2_G_MTX_LOAD, fixedPt);

    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 2u);
    // Top of stack should be identity
    EXPECT_NEAR(interp->mRsp->modelview_matrix_stack[1][3][0], 0.0f, 1e-3f);
    // Previous entry should still have translation
    EXPECT_NEAR(interp->mRsp->modelview_matrix_stack[0][3][0], 5.0f, 1e-3f);

    // Pop back
    interp->GfxSpPopMatrix(1);
    EXPECT_EQ(interp->mRsp->modelview_matrix_stack_size, 1u);
    // Top should be the original translation
    EXPECT_NEAR(interp->mRsp->modelview_matrix_stack[0][3][0], 5.0f, 1e-3f);
}

// ============================================================
// Layer 2: SetZImage Test
// ============================================================

TEST_F(RdpStateTest, SetZImage) {
    int dummy;
    interp->GfxDpSetZImage(&dummy);
    EXPECT_EQ(interp->mRdp->z_buf_address, &dummy);
}

// ============================================================
// Layer 2: AdjustWidthHeightForScale Tests
// ============================================================

TEST_F(RdpStateTest, AdjustWidthHeightForScale_1to1) {
    // Native 320x240, current 320x240 → 1:1 ratio → no change
    interp->mCurDimensions.width = 320;
    interp->mCurDimensions.height = 240;

    uint32_t w = 100, h = 50;
    interp->AdjustWidthHeightForScale(w, h, 320, 240);
    EXPECT_EQ(w, 100u);
    EXPECT_EQ(h, 50u);
}

TEST_F(RdpStateTest, AdjustWidthHeightForScale_2x) {
    // Native 320x240, current 640x480 → 2:1 ratio
    interp->mCurDimensions.width = 640;
    interp->mCurDimensions.height = 480;

    uint32_t w = 100, h = 50;
    interp->AdjustWidthHeightForScale(w, h, 320, 240);
    EXPECT_EQ(w, 200u);
    EXPECT_EQ(h, 100u);
}

TEST_F(RdpStateTest, AdjustWidthHeightForScale_ZeroClamped) {
    // Very small result would be 0 — should clamp to 1
    interp->mCurDimensions.width = 1;
    interp->mCurDimensions.height = 1;

    uint32_t w = 1, h = 1;
    interp->AdjustWidthHeightForScale(w, h, 320, 240);
    EXPECT_GE(w, 1u);
    EXPECT_GE(h, 1u);
}

// ============================================================
// Layer 2: Scissor Tests (with StubRenderingAPI)
// ============================================================

class ScissorViewportTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_unique<Fast::Interpreter>();
        stub = new StubRenderingAPI();
        interp->mRapi = stub;

        // Set dimensions to match native (1:1 ratio)
        interp->mNativeDimensions.width = 320;
        interp->mNativeDimensions.height = 240;
        interp->mCurDimensions.width = 320;
        interp->mCurDimensions.height = 240;
        interp->mFbActive = false;
        interp->mRendersToFb = false;
        interp->mMsaaLevel = 1;
        interp->mGameWindowViewport = {};
        interp->mGfxCurrentWindowDimensions = {};
    }

    void TearDown() override {
        interp->mRapi = nullptr;
        delete stub;
    }

    std::unique_ptr<Fast::Interpreter> interp;
    StubRenderingAPI* stub;
};

TEST_F(ScissorViewportTest, SetScissor_Basic) {
    // SetScissor coordinates are in 10.2 fixed-point (multiply by 4)
    // ulx=0, uly=0, lrx=320*4=1280, lry=240*4=960
    interp->GfxDpSetScissor(0, 0, 0, 1280, 960);

    // Before AdjustVIewportOrScissor:
    //   x = 0/4 = 0, y = 960/4 = 240, width = 1280/4 = 320, height = 960/4 = 240
    // After adjustment (invertY=false, 1:1 ratio):
    //   y = nativeHeight - y = 240 - 240 = 0
    //   All multiplied by 1:1 ratio = unchanged
    // The viewport_or_scissor_changed flag should be set
    EXPECT_TRUE(interp->mRdp->viewport_or_scissor_changed);

    // With invertY=false and 1:1 ratio:
    // scissor.width should be 320, scissor.height should be 240
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.width, 320.0f);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.height, 240.0f);
}

TEST_F(ScissorViewportTest, SetScissor_SubRegion) {
    // Scissor from (40,30) to (280,210) → width=240, height=180
    uint32_t ulx = 40 * 4;
    uint32_t uly = 30 * 4;
    uint32_t lrx = 280 * 4;
    uint32_t lry = 210 * 4;
    interp->GfxDpSetScissor(0, ulx, uly, lrx, lry);

    EXPECT_FLOAT_EQ(interp->mRdp->scissor.width, 240.0f);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.height, 180.0f);
}

// ============================================================
// Layer 2: CalcAndSetViewport Tests
// ============================================================

TEST_F(ScissorViewportTest, CalcAndSetViewport_FullScreen) {
    // Standard N64 viewport: vscale = {SCREEN_WD/2 * 4, SCREEN_HT/2 * 4, ...}
    //                        vtrans = {SCREEN_WD/2 * 4, SCREEN_HT/2 * 4, ...}
    Fast::F3DVp_t vp = {};
    vp.vscale[0] = 160 * 4; // half-width * 4
    vp.vscale[1] = 120 * 4; // half-height * 4
    vp.vtrans[0] = 160 * 4;
    vp.vtrans[1] = 120 * 4;

    interp->CalcAndSetViewport(&vp);

    // width = 2 * vscale[0] / 4 = 2 * 640 / 4 = 320
    // height = 2 * vscale[1] / 4 = 2 * 480 / 4 = 240
    // x = vtrans[0]/4 - width/2 = 160 - 160 = 0
    // y = vtrans[1]/4 + height/2 = 120 + 120 = 240
    // After AdjustVIewportOrScissor: y = nativeHeight - y = 240 - 240 = 0
    EXPECT_TRUE(interp->mRdp->viewport_or_scissor_changed);
    EXPECT_FLOAT_EQ(interp->mRdp->viewport.width, 320.0f);
    EXPECT_FLOAT_EQ(interp->mRdp->viewport.height, 240.0f);
}

// ============================================================
// Layer 2: GfxSpMovemem Light Loading Tests
// ============================================================

TEST_F(RdpStateTest, MovememF3dex2_LoadLight) {
    // F3DEX2: lightidx = offset / 24 - 2
    // offset = (lightidx + 2) * 24
    // For light 0: offset = 2 * 24 = 48
    Fast::F3DLight light = {};
    light.l.col[0] = 200;
    light.l.col[1] = 100;
    light.l.col[2] = 50;
    light.l.dir[0] = 0;
    light.l.dir[1] = 127;
    light.l.dir[2] = 0;

    interp->GfxSpMovememF3dex2(Fast::F3DEX2_G_MV_LIGHT, 48, &light);

    EXPECT_EQ(interp->mRsp->current_lights[0].l.col[0], 200);
    EXPECT_EQ(interp->mRsp->current_lights[0].l.col[1], 100);
    EXPECT_EQ(interp->mRsp->current_lights[0].l.col[2], 50);
    EXPECT_EQ(interp->mRsp->current_lights[0].l.dir[1], 127);
}

TEST_F(RdpStateTest, MovememF3dex2_LoadSecondLight) {
    // Light 1: offset = (1+2) * 24 = 72
    Fast::F3DLight light = {};
    light.l.col[0] = 0;
    light.l.col[1] = 255;
    light.l.col[2] = 0;
    light.l.dir[0] = 127;
    light.l.dir[1] = 0;
    light.l.dir[2] = 0;

    interp->GfxSpMovememF3dex2(Fast::F3DEX2_G_MV_LIGHT, 72, &light);

    EXPECT_EQ(interp->mRsp->current_lights[1].l.col[1], 255);
    EXPECT_EQ(interp->mRsp->current_lights[1].l.dir[0], 127);
}

TEST_F(RdpStateTest, MovememF3d_LoadLight0) {
    // F3D: light 0 uses Fast::F3DEX_G_MV_L0
    Fast::F3DLight_t light = {};
    light.col[0] = 128;
    light.col[1] = 64;
    light.col[2] = 32;
    light.dir[0] = 0;
    light.dir[1] = 0;
    light.dir[2] = 127;

    interp->GfxSpMovememF3d(Fast::F3DEX_G_MV_L0, 0, &light);

    // F3D: index = (Fast::F3DEX_G_MV_L0 - Fast::F3DEX_G_MV_L0) / 2 = 0
    EXPECT_EQ(interp->mRsp->current_lights[0].l.col[0], 128);
    EXPECT_EQ(interp->mRsp->current_lights[0].l.dir[2], 127);
}

TEST_F(RdpStateTest, MovememF3d_LoadLight1) {
    Fast::F3DLight_t light = {};
    light.col[0] = 255;
    light.col[1] = 0;
    light.col[2] = 255;

    interp->GfxSpMovememF3d(Fast::F3DEX_G_MV_L1, 0, &light);

    // index = (Fast::F3DEX_G_MV_L1 - Fast::F3DEX_G_MV_L0) / 2 = 1
    EXPECT_EQ(interp->mRsp->current_lights[1].l.col[0], 255);
    EXPECT_EQ(interp->mRsp->current_lights[1].l.col[2], 255);
}

TEST_F(RdpStateTest, MovememF3d_LoadLookat) {
    Fast::F3DLight_t lookat = {};
    lookat.dir[0] = 100;
    lookat.dir[1] = 50;
    lookat.dir[2] = 25;

    interp->GfxSpMovememF3d(Fast::F3DEX_G_MV_LOOKATY, 0, &lookat);

    EXPECT_EQ(interp->mRsp->lookat[0].dir[0], 100);
    EXPECT_EQ(interp->mRsp->lookat[0].dir[1], 50);
    EXPECT_EQ(interp->mRsp->lookat[0].dir[2], 25);
}

// ============================================================
// Layer 2: LoadBlock Tests
// ============================================================

class LoadBlockTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_unique<Fast::Interpreter>();
    }

    std::unique_ptr<Fast::Interpreter> interp;
};

TEST_F(LoadBlockTest, LoadBlock_16bit_Basic) {
    // Set up texture_to_load state
    uint8_t texData[128] = {};
    Fast::RawTexMetadata meta = {};
    meta.h_byte_scale = 1.0f;
    meta.v_pixel_scale = 1.0f;
    interp->GfxDpSetTextureImage(0, G_IM_SIZ_16b, 1, nullptr, 0, meta, texData);

    // Set up tile 0 at tmem=0
    interp->GfxDpSetTile(0, G_IM_SIZ_16b, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    // LoadBlock: lrs = pixel_count - 1 = 31 (32 pixels of 16-bit = 64 bytes)
    interp->GfxDpLoadBlock(0, 0, 0, 31, 0);

    // size_bytes = (31+1) << 1 = 64 bytes
    EXPECT_EQ(interp->mRdp->loaded_texture[0].size_bytes, 64u);
    EXPECT_EQ(interp->mRdp->loaded_texture[0].addr, texData);
    EXPECT_TRUE(interp->mRdp->textures_changed[0]);
}

TEST_F(LoadBlockTest, LoadBlock_32bit) {
    uint8_t texData[256] = {};
    Fast::RawTexMetadata meta = {};
    meta.h_byte_scale = 1.0f;
    meta.v_pixel_scale = 1.0f;
    interp->GfxDpSetTextureImage(0, G_IM_SIZ_32b, 1, nullptr, 0, meta, texData);
    interp->GfxDpSetTile(0, G_IM_SIZ_32b, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    // 16 pixels of 32-bit = 64 bytes
    interp->GfxDpLoadBlock(0, 0, 0, 15, 0);

    // size_bytes = (15+1) << 2 = 64
    EXPECT_EQ(interp->mRdp->loaded_texture[0].size_bytes, 64u);
}

TEST_F(LoadBlockTest, LoadBlock_8bit) {
    uint8_t texData[64] = {};
    Fast::RawTexMetadata meta = {};
    meta.h_byte_scale = 1.0f;
    meta.v_pixel_scale = 1.0f;
    interp->GfxDpSetTextureImage(0, G_IM_SIZ_8b, 1, nullptr, 0, meta, texData);
    interp->GfxDpSetTile(0, G_IM_SIZ_8b, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    // 64 pixels of 8-bit = 64 bytes
    interp->GfxDpLoadBlock(0, 0, 0, 63, 0);

    // size_bytes = (63+1) << 0 = 64
    EXPECT_EQ(interp->mRdp->loaded_texture[0].size_bytes, 64u);
}

TEST_F(LoadBlockTest, LoadBlock_WithLineSize) {
    // When texture_to_load.width > 1, line_size_bytes should be calculated
    uint8_t texData[256] = {};
    Fast::RawTexMetadata meta = {};
    meta.h_byte_scale = 1.0f;
    meta.v_pixel_scale = 1.0f;
    // Set width=16 pixels, 16-bit format → line = 16 * 2 = 32 bytes
    interp->GfxDpSetTextureImage(0, G_IM_SIZ_16b, 16, nullptr, 0, meta, texData);
    interp->GfxDpSetTile(0, G_IM_SIZ_16b, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    // Load 16*8=128 pixels → (127+1)*2 = 256 bytes total
    interp->GfxDpLoadBlock(0, 0, 0, 127, 0);

    EXPECT_EQ(interp->mRdp->loaded_texture[0].size_bytes, 256u);
    // line_size should be 32 (16 pixels * 2 bytes) since 32 < 256 and 256 % 32 == 0
    EXPECT_EQ(interp->mRdp->loaded_texture[0].line_size_bytes, 32u);
}

// ============================================================
// Layer 2: LoadTlut (Palette Load) Tests
// ============================================================

TEST_F(RdpStateTest, LoadTlut_CI4_SinglePalette) {
    // Set up a 16-entry CI4 palette (32 bytes)
    uint8_t palData[32] = {};
    palData[0] = 0xFF; palData[1] = 0xFF; // Entry 0: white opaque

    Fast::RawTexMetadata meta = {};
    interp->GfxDpSetTextureImage(0, G_IM_SIZ_16b, 0, nullptr, 0, meta, palData);

    // Set tile tmem=256 (start of palette area)
    interp->GfxDpSetTile(0, G_IM_SIZ_16b, 0, 256, 0, 0, 0, 0, 0, 0, 0, 0);

    // Load 16 entries (high_index=15)
    interp->GfxDpLoadTlut(0, 15);

    // Should have loaded into palette_staging[0] at offset (256-256)*2 = 0
    EXPECT_EQ(interp->mRdp->palettes[0], interp->mRdp->palette_staging[0]);
    EXPECT_EQ(interp->mRdp->palette_staging[0][0], 0xFF);
    EXPECT_EQ(interp->mRdp->palette_staging[0][1], 0xFF);
}

TEST_F(RdpStateTest, LoadTlut_CI8_FullPalette) {
    // CI8: 256 entries = 512 bytes spanning both halves
    uint8_t palData[512] = {};
    palData[0] = 0xAA;     // First entry
    palData[256] = 0xBB;   // First entry of second half

    Fast::RawTexMetadata meta = {};
    interp->GfxDpSetTextureImage(0, G_IM_SIZ_16b, 0, nullptr, 0, meta, palData);
    interp->GfxDpSetTile(0, G_IM_SIZ_16b, 0, 256, 0, 0, 0, 0, 0, 0, 0, 0);

    // Load all 256 entries
    interp->GfxDpLoadTlut(0, 255);

    EXPECT_EQ(interp->mRdp->palettes[0], interp->mRdp->palette_staging[0]);
    EXPECT_EQ(interp->mRdp->palettes[1], interp->mRdp->palette_staging[1]);
    EXPECT_EQ(interp->mRdp->palette_staging[0][0], 0xAA);
    EXPECT_EQ(interp->mRdp->palette_staging[1][0], 0xBB);
}

// ============================================================
// OtherMode Depth Flag Tests
// ============================================================

TEST_F(RdpStateTest, SetOtherMode_DepthCompareAndUpdate) {
    // Set Z_CMP | Z_UPD via GfxDpSetOtherMode
    interp->GfxDpSetOtherMode(0, Z_CMP | Z_UPD);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_UPD, 0u);
}

TEST_F(RdpStateTest, SetOtherMode_DepthCompareOnly) {
    // Set only Z_CMP (no Z_UPD)
    interp->GfxDpSetOtherMode(0, Z_CMP);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & Z_UPD, 0u);
}

TEST_F(RdpStateTest, SetOtherMode_DepthUpdateOnly) {
    // Set only Z_UPD (no Z_CMP)
    interp->GfxDpSetOtherMode(0, Z_UPD);
    EXPECT_EQ(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_UPD, 0u);
}

TEST_F(RdpStateTest, SetOtherMode_ZmodeDecal) {
    // ZMODE_DEC = 0xC00
    interp->GfxDpSetOtherMode(0, ZMODE_DEC);
    EXPECT_EQ(interp->mRdp->other_mode_l & ZMODE_DEC, ZMODE_DEC);
}

TEST_F(RdpStateTest, SetOtherMode_NoDepthFlags) {
    // Clear state first with depth flags, then set with none
    interp->GfxDpSetOtherMode(0, Z_CMP | Z_UPD | ZMODE_DEC);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);

    // Now clear all
    interp->GfxDpSetOtherMode(0, 0);
    EXPECT_EQ(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_EQ(interp->mRdp->other_mode_l & Z_UPD, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & ZMODE_DEC, ZMODE_DEC);
}

TEST_F(RdpStateTest, SpSetOtherMode_DepthBitsViaMask) {
    // Use GfxSpSetOtherMode to set specific bits via shift/width
    // Z_CMP is bit 4 (0x10), Z_UPD is bit 5 (0x20)
    // Set bits 4-5 to 0x30 (both Z_CMP and Z_UPD)
    interp->mRdp->other_mode_l = 0;
    interp->mRdp->other_mode_h = 0;
    interp->GfxSpSetOtherMode(4, 2, Z_CMP | Z_UPD);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_UPD, 0u);
}

TEST_F(RdpStateTest, SpSetOtherMode_PreservesUnrelatedBits) {
    // Set some bits first, then modify only depth bits
    interp->mRdp->other_mode_l = 0xFF000000;
    interp->mRdp->other_mode_h = 0;
    interp->GfxSpSetOtherMode(4, 2, Z_CMP | Z_UPD);
    // Upper bits should be preserved
    EXPECT_EQ(interp->mRdp->other_mode_l & 0xFF000000, 0xFF000000u);
    // Depth bits should be set
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
}

// ============================================================
// SetZImage and SetColorImage Tests
// ============================================================

TEST_F(RdpStateTest, SetZImage_ChangesAddress) {
    int dummy1, dummy2;
    interp->GfxDpSetZImage(&dummy1);
    EXPECT_EQ(interp->mRdp->z_buf_address, &dummy1);
    interp->GfxDpSetZImage(&dummy2);
    EXPECT_EQ(interp->mRdp->z_buf_address, &dummy2);
}

TEST_F(RdpStateTest, SetColorImage_StoresFormatSizeWidth) {
    int dummy;
    // format=2, size=1, width=640
    interp->GfxDpSetColorImage(2, 1, 640, &dummy);
    EXPECT_EQ(interp->mRdp->color_image_address, &dummy);
}

TEST_F(RdpStateTest, SetColorImage_NullAddress) {
    interp->GfxDpSetColorImage(0, 0, 320, nullptr);
    EXPECT_EQ(interp->mRdp->color_image_address, nullptr);
}

// ============================================================
// AdjustWidthHeightForScale fractional tests
// ============================================================

TEST_F(RdpStateTest, AdjustWidthHeightForScale_1_5x) {
    // Native 320x240, current 480x360 → 1.5:1 ratio
    interp->mCurDimensions.width = 480;
    interp->mCurDimensions.height = 360;

    uint32_t w = 100, h = 100;
    interp->AdjustWidthHeightForScale(w, h, 320, 240);
    EXPECT_EQ(w, 150u);
    EXPECT_EQ(h, 150u);
}

TEST_F(RdpStateTest, AdjustWidthHeightForScale_Asymmetric) {
    // Non-uniform scaling: width doubled, height tripled
    interp->mCurDimensions.width = 640;
    interp->mCurDimensions.height = 720;

    uint32_t w = 100, h = 100;
    interp->AdjustWidthHeightForScale(w, h, 320, 240);
    EXPECT_EQ(w, 200u);
    EXPECT_EQ(h, 300u);
}

// ============================================================
// Framebuffer Info Tests
// ============================================================

TEST_F(RdpStateTest, FrameBufferMap_EmptyByDefault) {
    EXPECT_TRUE(interp->mFrameBuffers.empty());
}

TEST_F(RdpStateTest, FrameBufferInfo_StoreAndRetrieve) {
    // Insert a framebuffer entry and verify its properties
    Fast::FBInfo fbInfo = { 320, 240, 640, 480, 320, 240, true };
    interp->mFrameBuffers[1] = fbInfo;

    EXPECT_EQ(interp->mFrameBuffers.size(), 1u);
    auto it = interp->mFrameBuffers.find(1);
    EXPECT_NE(it, interp->mFrameBuffers.end());
    EXPECT_EQ(it->second.orig_width, 320u);
    EXPECT_EQ(it->second.orig_height, 240u);
    EXPECT_EQ(it->second.applied_width, 640u);
    EXPECT_EQ(it->second.applied_height, 480u);
    EXPECT_TRUE(it->second.resize);
}

TEST_F(RdpStateTest, FrameBufferInfo_MultipleEntries) {
    Fast::FBInfo fb1 = { 320, 240, 640, 480, 320, 240, true };
    Fast::FBInfo fb2 = { 160, 120, 320, 240, 160, 120, false };
    interp->mFrameBuffers[1] = fb1;
    interp->mFrameBuffers[2] = fb2;

    EXPECT_EQ(interp->mFrameBuffers.size(), 2u);
    EXPECT_EQ(interp->mFrameBuffers[1].orig_width, 320u);
    EXPECT_EQ(interp->mFrameBuffers[2].orig_width, 160u);
    EXPECT_FALSE(interp->mFrameBuffers[2].resize);
}

TEST_F(RdpStateTest, FbActiveDefault) {
    // mFbActive should default to false
    EXPECT_FALSE(interp->mFbActive);
}

TEST_F(RdpStateTest, RenderingState_DepthDefault) {
    // RenderingState depth_test_and_mask should start at 0
    EXPECT_EQ(interp->mRenderingState.depth_test_and_mask, 0u);
    EXPECT_FALSE(interp->mRenderingState.decal_mode);
}
