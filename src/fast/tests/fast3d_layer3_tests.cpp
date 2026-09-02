// Fast3D Layer 3 Tests — Multi-step pipelines
//
// Tests cover:
//  - Vertex transform with lighting
//  - Vertex texture coordinates
//  - Vertex clip rejection
//  - Vertex fog calculation
//  - Dest index offset in GfxSpVertex
//  - Combined LoadBlock → ImportTexture pipeline
//  - Display list command dispatch
//  - Display list depth tests

#include "fast3d_test_common.h"

using namespace fast3d_test;

// ============================================================
// Vertex Lighting Tests
// ============================================================

class VertexLightingTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_unique<Fast::Interpreter>();
        stub = new StubRenderingAPI();
        interp->mRapi = stub;
        memset(interp->mRsp, 0, sizeof(Fast::RSP));
        interp->mRsp->modelview_matrix_stack_size = 1;

        // Identity matrices
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++) {
                interp->mRsp->MP_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
                interp->mRsp->modelview_matrix_stack[0][i][j] = (i == j) ? 1.0f : 0.0f;
                interp->mRsp->P_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
            }

        // Set native dimensions
        interp->mNativeDimensions.width = SCREEN_WIDTH;
        interp->mNativeDimensions.height = SCREEN_HEIGHT;
        interp->mCurDimensions.width = SCREEN_WIDTH;
        interp->mCurDimensions.height = SCREEN_HEIGHT;
        interp->mFbActive = false;

        // Enable lighting
        interp->mRsp->geometry_mode = G_LIGHTING;

        // Set up 1 directional light + ambient
        interp->mRsp->current_num_lights = 2; // 1 directional + 1 ambient
        interp->mRsp->lights_changed = true;

        // Directional light 0: white, pointing along +Y
        interp->mRsp->current_lights[0].l.col[0] = 255;
        interp->mRsp->current_lights[0].l.col[1] = 255;
        interp->mRsp->current_lights[0].l.col[2] = 255;
        interp->mRsp->current_lights[0].l.dir[0] = 0;
        interp->mRsp->current_lights[0].l.dir[1] = 127;
        interp->mRsp->current_lights[0].l.dir[2] = 0;

        // Ambient light (last light = index 1)
        interp->mRsp->current_lights[1].l.col[0] = 40;
        interp->mRsp->current_lights[1].l.col[1] = 40;
        interp->mRsp->current_lights[1].l.col[2] = 40;

        // Initialize lookat directions
        interp->mRsp->lookat[0].dir[0] = 0;
        interp->mRsp->lookat[0].dir[1] = 127;
        interp->mRsp->lookat[0].dir[2] = 0;
        interp->mRsp->lookat[1].dir[0] = 127;
        interp->mRsp->lookat[1].dir[1] = 0;
        interp->mRsp->lookat[1].dir[2] = 0;
    }

    void TearDown() override {
        interp->mRapi = nullptr;
        delete stub;
    }

    std::unique_ptr<Fast::Interpreter> interp;
    StubRenderingAPI* stub;
};

TEST_F(VertexLightingTest, DirectionalLight_NormalFacingLight) {
    // Vertex with normal pointing along +Y (same as light direction)
    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 0;
    vtx.v.ob[1] = 0;
    vtx.v.ob[2] = 0;
    // Normal (as signed bytes): pointing +Y
    vtx.n.n[0] = 0;
    vtx.n.n[1] = 127;
    vtx.n.n[2] = 0;
    vtx.v.cn[3] = 255; // alpha

    interp->GfxSpVertex(1, 0, &vtx);

    Fast::LoadedVertex& lv = interp->mRsp->loaded_vertices[0];
    // With normal facing directly toward light:
    // dot product = 1.0 → full directional contribution = 255
    // total = ambient(40) + directional(255) = 295 → clamped to 255
    EXPECT_GE(lv.color.r, 200); // Should be near 255
    EXPECT_GE(lv.color.g, 200);
    EXPECT_GE(lv.color.b, 200);
}

TEST_F(VertexLightingTest, DirectionalLight_NormalAwayFromLight) {
    // Normal pointing -Y (away from light direction +Y)
    Fast::F3DVtx vtx = {};
    vtx.n.n[0] = 0;
    vtx.n.n[1] = -127;
    vtx.n.n[2] = 0;
    vtx.v.cn[3] = 255;

    interp->GfxSpVertex(1, 0, &vtx);

    Fast::LoadedVertex& lv = interp->mRsp->loaded_vertices[0];
    // dot product = -1.0 → no directional contribution (clamped to 0)
    // total = ambient only = 40
    EXPECT_EQ(lv.color.r, 40);
    EXPECT_EQ(lv.color.g, 40);
    EXPECT_EQ(lv.color.b, 40);
}

TEST_F(VertexLightingTest, DirectionalLight_NormalPerpendicular) {
    // Normal pointing +X (perpendicular to light +Y)
    Fast::F3DVtx vtx = {};
    vtx.n.n[0] = 127;
    vtx.n.n[1] = 0;
    vtx.n.n[2] = 0;
    vtx.v.cn[3] = 255;

    interp->GfxSpVertex(1, 0, &vtx);

    Fast::LoadedVertex& lv = interp->mRsp->loaded_vertices[0];
    // dot product = 0 → no directional contribution
    // total = ambient only = 40
    EXPECT_EQ(lv.color.r, 40);
    EXPECT_EQ(lv.color.g, 40);
    EXPECT_EQ(lv.color.b, 40);
}

// ============================================================
// Vertex Transform Tests (texture coordinates, clip rejection,
// dest index offset)
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

// ============================================================
// Vertex Texture Coordinate Tests
// ============================================================

TEST_F(VertexTransformTest, TextureCoordinates) {
    // Set texture scaling factor (U0.16 format)
    interp->mRsp->texture_scaling_factor.s = 0x8000; // 0.5 in U0.16
    interp->mRsp->texture_scaling_factor.t = 0x4000; // 0.25 in U0.16

    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 0;
    vtx.v.ob[1] = 0;
    vtx.v.ob[2] = 0;
    vtx.v.tc[0] = 1024; // texture coordinate S
    vtx.v.tc[1] = 2048; // texture coordinate T

    interp->GfxSpVertex(1, 0, &vtx);

    Fast::LoadedVertex& lv = interp->mRsp->loaded_vertices[0];
    // U = tc[0] * s >> 16 = 1024 * 0x8000 >> 16 = 1024 * 32768 >> 16 = 512
    EXPECT_EQ(lv.u, 512);
    // V = tc[1] * t >> 16 = 2048 * 0x4000 >> 16 = 2048 * 16384 >> 16 = 512
    EXPECT_EQ(lv.v, 512);
}

TEST_F(VertexTransformTest, TextureCoordinates_NoScaling) {
    // Zero scaling = no texture coords
    interp->mRsp->texture_scaling_factor.s = 0;
    interp->mRsp->texture_scaling_factor.t = 0;

    Fast::F3DVtx vtx = {};
    vtx.v.tc[0] = 1024;
    vtx.v.tc[1] = 2048;

    interp->GfxSpVertex(1, 0, &vtx);

    EXPECT_EQ(interp->mRsp->loaded_vertices[0].u, 0);
    EXPECT_EQ(interp->mRsp->loaded_vertices[0].v, 0);
}

// ============================================================
// Vertex Clip Rejection Tests
// ============================================================

TEST_F(VertexTransformTest, ClipRejection_InsideFrustum) {
    // Vertex at origin with identity matrix → x=0,y=0,z=0,w=1
    // All within [-w, w] → no clip flags
    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 0;
    vtx.v.ob[1] = 0;
    vtx.v.ob[2] = 0;

    interp->GfxSpVertex(1, 0, &vtx);

    EXPECT_EQ(interp->mRsp->loaded_vertices[0].clip_rej, 0);
}

TEST_F(VertexTransformTest, ClipRejection_FarClip) {
    // Set up matrix that will produce z > w (far clip)
    // With identity matrix, w = 1.0. z > w when z > 1
    // z = ob[2] * MP[2][2] + MP[3][2] = ob[2] * 1 = ob[2]
    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 0;
    vtx.v.ob[1] = 0;
    vtx.v.ob[2] = 2; // z=2 > w=1

    interp->GfxSpVertex(1, 0, &vtx);

    // clip_rej should have CLIP_FAR (bit 5 = 32)
    EXPECT_NE(interp->mRsp->loaded_vertices[0].clip_rej & 32, 0);
}

TEST_F(VertexTransformTest, ClipRejection_LeftClip) {
    // With identity: x = ob[0] * 1 = ob[0], w = 1
    // x < -w when x < -1
    // But AdjXForAspectRatio adjusts x, so use a large negative value
    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = -100; // will be < -w after aspect ratio adjustment
    vtx.v.ob[1] = 0;
    vtx.v.ob[2] = 0;

    interp->GfxSpVertex(1, 0, &vtx);

    // Should have CLIP_LEFT (bit 0 = 1)
    EXPECT_NE(interp->mRsp->loaded_vertices[0].clip_rej & 1, 0);
}

TEST_F(VertexTransformTest, ClipRejection_RightClip) {
    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 100; // will be > w after aspect ratio adjustment
    vtx.v.ob[1] = 0;
    vtx.v.ob[2] = 0;

    interp->GfxSpVertex(1, 0, &vtx);

    // Should have CLIP_RIGHT (bit 1 = 2)
    EXPECT_NE(interp->mRsp->loaded_vertices[0].clip_rej & 2, 0);
}

TEST_F(VertexTransformTest, ClipRejection_TopClip) {
    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 0;
    vtx.v.ob[1] = 100; // y > w=1
    vtx.v.ob[2] = 0;

    interp->GfxSpVertex(1, 0, &vtx);

    // Should have CLIP_TOP (bit 3 = 8)
    EXPECT_NE(interp->mRsp->loaded_vertices[0].clip_rej & 8, 0);
}

TEST_F(VertexTransformTest, ClipRejection_BottomClip) {
    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 0;
    vtx.v.ob[1] = -100; // y < -w=−1
    vtx.v.ob[2] = 0;

    interp->GfxSpVertex(1, 0, &vtx);

    // Should have CLIP_BOTTOM (bit 2 = 4)
    EXPECT_NE(interp->mRsp->loaded_vertices[0].clip_rej & 4, 0);
}

// ============================================================
// Vertex Fog Calculation Tests
// ============================================================

class VertexFogTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_unique<Fast::Interpreter>();
        memset(interp->mRsp, 0, sizeof(Fast::RSP));
        interp->mRsp->modelview_matrix_stack_size = 1;

        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++) {
                interp->mRsp->MP_matrix[i][j] = (i == j) ? 1.0f : 0.0f;
                interp->mRsp->modelview_matrix_stack[0][i][j] = (i == j) ? 1.0f : 0.0f;
            }

        interp->mNativeDimensions.width = SCREEN_WIDTH;
        interp->mNativeDimensions.height = SCREEN_HEIGHT;
        interp->mCurDimensions.width = SCREEN_WIDTH;
        interp->mCurDimensions.height = SCREEN_HEIGHT;
        interp->mFbActive = false;

        // Enable fog
        interp->mRsp->geometry_mode = G_FOG;
        interp->mRsp->fog_mul = 256;
        interp->mRsp->fog_offset = 0;
    }

    std::unique_ptr<Fast::Interpreter> interp;
};

TEST_F(VertexFogTest, FogAtOrigin) {
    // Vertex at origin: z=0, w=1 → fog_z = 0*1*256 + 0 = 0 → clamped to 0
    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 0;
    vtx.v.ob[1] = 0;
    vtx.v.ob[2] = 0;
    vtx.v.cn[3] = 200; // original alpha (overwritten by fog)

    interp->GfxSpVertex(1, 0, &vtx);

    // Fog should replace alpha
    EXPECT_EQ(interp->mRsp->loaded_vertices[0].color.a, 0);
}

TEST_F(VertexFogTest, FogWithOffset) {
    // Set fog_offset to push fog value up
    interp->mRsp->fog_mul = 0;
    interp->mRsp->fog_offset = 128;

    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 0;
    vtx.v.ob[1] = 0;
    vtx.v.ob[2] = 0;

    interp->GfxSpVertex(1, 0, &vtx);

    // fog_z = z * winv * fog_mul + fog_offset = 0 * 1 * 0 + 128 = 128
    EXPECT_EQ(interp->mRsp->loaded_vertices[0].color.a, 128);
}

TEST_F(VertexFogTest, FogClampedTo255) {
    interp->mRsp->fog_mul = 0;
    interp->mRsp->fog_offset = 500; // above 255

    Fast::F3DVtx vtx = {};
    interp->GfxSpVertex(1, 0, &vtx);

    // Should be clamped to 255
    EXPECT_EQ(interp->mRsp->loaded_vertices[0].color.a, 255);
}

// ============================================================
// Dest Index Offset in GfxSpVertex
// ============================================================

TEST_F(VertexTransformTest, DestIndexOffset) {
    // Load vertex at dest_index=5
    Fast::F3DVtx vtx = {};
    vtx.v.ob[0] = 42;
    vtx.v.ob[1] = 0;
    vtx.v.ob[2] = 0;

    interp->GfxSpVertex(1, 5, &vtx);

    // Verify vertex was stored at index 5
    EXPECT_FLOAT_EQ(interp->mRsp->loaded_vertices[5].z, 0.0f);
    EXPECT_FLOAT_EQ(interp->mRsp->loaded_vertices[5].w, 1.0f);
}

// ============================================================
// Combined LoadBlock → ImportTexture Pipeline
// ============================================================

TEST_F(TextureTestFixture, LoadBlock_ThenImportRgba16) {
    // Simulate a realistic pipeline: SetTextureImage → SetTile → LoadBlock → ImportTexture
    uint8_t texData[8] = {};
    // Pixel 0: white opaque (R=31,G=31,B=31,A=1 → 0xFFFF)
    texData[0] = 0xFF; texData[1] = 0xFF;
    // Pixel 1: red opaque (R=31,G=0,B=0,A=1 → 0xF801)
    texData[2] = 0xF8; texData[3] = 0x01;
    // Pixel 2: black opaque (R=0,G=0,B=0,A=1 → 0x0001)
    texData[4] = 0x00; texData[5] = 0x01;
    // Pixel 3: transparent (0x0000)
    texData[6] = 0x00; texData[7] = 0x00;

    // Step 1: SetTextureImage
    Fast::RawTexMetadata meta = {};
    meta.h_byte_scale = 1.0f;
    meta.v_pixel_scale = 1.0f;
    interp->GfxDpSetTextureImage(0, G_IM_SIZ_16b, 2 /*width=2 pixels*/, nullptr, 0, meta, texData);

    // Step 2: SetTile (tile=0, tmem=0, line=1 means 8 bytes per row)
    interp->GfxDpSetTile(0, G_IM_SIZ_16b, 1 /*line*/, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    // Step 3: LoadBlock (4 pixels of 16-bit = 8 bytes)
    interp->GfxDpLoadBlock(0, 0, 0, 3, 0);

    // Verify loaded_texture state
    EXPECT_EQ(interp->mRdp->loaded_texture[0].size_bytes, 8u);
    EXPECT_EQ(interp->mRdp->loaded_texture[0].addr, texData);

    // Step 4: Set tile size for a 2x2 texture
    interp->GfxDpSetTileSize(0, 0, 0, (2 - 1) * 4, (2 - 1) * 4);

    // Step 5: Import the texture
    interp->ImportTextureRgba16(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 2u);
    EXPECT_EQ(stub->uploads[0].height, 2u);

    // Pixel 0: white opaque
    EXPECT_EQ(stub->uploads[0].data[0], 255);
    EXPECT_EQ(stub->uploads[0].data[1], 255);
    EXPECT_EQ(stub->uploads[0].data[2], 255);
    EXPECT_EQ(stub->uploads[0].data[3], 255);

    // Pixel 1: red opaque
    EXPECT_EQ(stub->uploads[0].data[4], 255);
    EXPECT_EQ(stub->uploads[0].data[5], 0);
    EXPECT_EQ(stub->uploads[0].data[6], 0);
    EXPECT_EQ(stub->uploads[0].data[7], 255);
}

// ============================================================
// Display List Command Dispatch Tests
// These tests build F3DGfx command arrays and process them
// through the interpreter's display list execution loop.
// ============================================================

class DisplayListTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_shared<Fast::Interpreter>();
        stub = new StubRenderingAPI();
        interp->mRapi = stub;

        // Register as the global instance (required by gfx_step handlers)
        Fast::GfxSetInstance(interp);

        // Set dimensions
        interp->mNativeDimensions.width = SCREEN_WIDTH;
        interp->mNativeDimensions.height = SCREEN_HEIGHT;
        interp->mCurDimensions.width = SCREEN_WIDTH;
        interp->mCurDimensions.height = SCREEN_HEIGHT;
        interp->mFbActive = false;
    }

    void TearDown() override {
        Fast::GfxSetInstance(nullptr);
        interp->mRapi = nullptr;
        delete stub;
        interp.reset();
    }

    std::shared_ptr<Fast::Interpreter> interp;
    StubRenderingAPI* stub;
    std::unordered_map<Mtx*, MtxF> emptyMtxReplacements;
};

TEST_F(DisplayListTest, SetEnvColor_ViaDisplayList) {
    // Build a display list: G_SETENVCOLOR(r=255, g=128, b=64, a=32), G_ENDDL
    Fast::F3DGfx dl[2];
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_SETENVCOLOR << 24,
                    (255u << 24) | (128u << 16) | (64u << 8) | 32u);
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    EXPECT_EQ(interp->mRdp->env_color.r, 255);
    EXPECT_EQ(interp->mRdp->env_color.g, 128);
    EXPECT_EQ(interp->mRdp->env_color.b, 64);
    EXPECT_EQ(interp->mRdp->env_color.a, 32);
}

TEST_F(DisplayListTest, SetFogColor_ViaDisplayList) {
    Fast::F3DGfx dl[2];
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_SETFOGCOLOR << 24,
                    (100u << 24) | (200u << 16) | (50u << 8) | 255u);
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    EXPECT_EQ(interp->mRdp->fog_color.r, 100);
    EXPECT_EQ(interp->mRdp->fog_color.g, 200);
    EXPECT_EQ(interp->mRdp->fog_color.b, 50);
    EXPECT_EQ(interp->mRdp->fog_color.a, 255);
}

TEST_F(DisplayListTest, SetPrimColor_ViaDisplayList) {
    // G_SETPRIMCOLOR encodes minlevel and primlevel in w0, rgba in w1
    Fast::F3DGfx dl[2];
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_SETPRIMCOLOR << 24 | (5u << 8) | 10u,
                    (200u << 24) | (150u << 16) | (100u << 8) | 50u);
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    EXPECT_EQ(interp->mRdp->prim_color.r, 200);
    EXPECT_EQ(interp->mRdp->prim_color.g, 150);
    EXPECT_EQ(interp->mRdp->prim_color.b, 100);
    EXPECT_EQ(interp->mRdp->prim_color.a, 50);
}

TEST_F(DisplayListTest, SetFillColor_ViaDisplayList) {
    // G_SETFILLCOLOR: w1 is the packed color
    // For RGBA16: red opaque = 0xF801F801
    Fast::F3DGfx dl[2];
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_SETFILLCOLOR << 24, 0xF801F801u);
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    // SetFillColor decodes RGBA16: R=0xF8>>3=31, scaled to 255
    EXPECT_EQ(interp->mRdp->fill_color.r, 255);
    EXPECT_EQ(interp->mRdp->fill_color.g, 0);
    EXPECT_EQ(interp->mRdp->fill_color.b, 0);
    EXPECT_EQ(interp->mRdp->fill_color.a, 255);
}

TEST_F(DisplayListTest, GeometryMode_ViaDisplayList) {
    // F3DEX2 G_GEOMETRYMODE: w0 = (opcode<<24) | ~clear_bits (24 bits), w1 = set_bits
    // Clear nothing (~0 = 0xFFFFFF in 24 bits), set G_LIGHTING (0x00020000)
    uint32_t clearBits = 0; // ~clearBits & 0xFFFFFF = 0xFFFFFF (clear nothing)
    uint32_t setBits = G_LIGHTING;
    Fast::F3DGfx dl[2];
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_GEOMETRYMODE << 24 | (~clearBits & 0xFFFFFF),
                    setBits);
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    EXPECT_NE(interp->mRsp->geometry_mode & G_LIGHTING, 0u);
}

TEST_F(DisplayListTest, MultipleCommands_ViaDisplayList) {
    // Test a sequence of multiple RDP commands in one display list
    Fast::F3DGfx dl[4];
    // Set env color to red
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_SETENVCOLOR << 24,
                    (255u << 24) | (0u << 16) | (0u << 8) | 255u);
    // Set fog color to blue
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_SETFOGCOLOR << 24,
                    (0u << 24) | (0u << 16) | (255u << 8) | 255u);
    // Set prim color to green
    dl[2] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_SETPRIMCOLOR << 24,
                    (0u << 24) | (255u << 16) | (0u << 8) | 255u);
    dl[3] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    EXPECT_EQ(interp->mRdp->env_color.r, 255);
    EXPECT_EQ(interp->mRdp->env_color.g, 0);
    EXPECT_EQ(interp->mRdp->env_color.b, 0);

    EXPECT_EQ(interp->mRdp->fog_color.r, 0);
    EXPECT_EQ(interp->mRdp->fog_color.g, 0);
    EXPECT_EQ(interp->mRdp->fog_color.b, 255);

    EXPECT_EQ(interp->mRdp->prim_color.r, 0);
    EXPECT_EQ(interp->mRdp->prim_color.g, 255);
    EXPECT_EQ(interp->mRdp->prim_color.b, 0);
}

TEST_F(DisplayListTest, SetScissor_ViaDisplayList) {
    // G_SETSCISSOR: w0 = (opcode<<24) | (ulx<<12) | uly, w1 = (mode<<24) | (lrx<<12) | lry
    // Full screen scissor: ulx=0, uly=0, lrx=320*4=1280, lry=240*4=960
    uint32_t ulx = 0, uly = 0;
    uint32_t lrx = 320 * 4, lry = 240 * 4;
    Fast::F3DGfx dl[2];
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_SETSCISSOR << 24 | (ulx << 12) | uly,
                    (0u << 24) | (lrx << 12) | lry);
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    EXPECT_TRUE(interp->mRdp->viewport_or_scissor_changed);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.width, 320.0f);
    EXPECT_FLOAT_EQ(interp->mRdp->scissor.height, 240.0f);
}

// ============================================================
// Display List Depth Tests
// ============================================================

TEST_F(DisplayListTest, SetZImage_ViaDisplayList) {
    // G_SETZIMG: w0 = (opcode<<24), w1 = address
    int dummyZBuf;
    Fast::F3DGfx dl[2];
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_SETZIMG << 24, (uintptr_t)&dummyZBuf);
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    EXPECT_EQ(interp->mRdp->z_buf_address, &dummyZBuf);
}

TEST_F(DisplayListTest, SetColorImage_ViaDisplayList) {
    // G_SETCIMG: w0 = (opcode<<24) | (format<<21) | (size<<19) | width, w1 = address
    int dummyCBuf;
    uint32_t format = 0; // RGBA
    uint32_t size = 2;   // 16-bit
    uint32_t width = 319; // width-1 encoded
    Fast::F3DGfx dl[2];
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_SETCIMG << 24 | (format << 21) | (size << 19) | (width & 0x7FF),
                    (uintptr_t)&dummyCBuf);
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    EXPECT_EQ(interp->mRdp->color_image_address, &dummyCBuf);
}

TEST_F(DisplayListTest, DepthMode_ViaDisplayList_SetOtherModeRDP) {
    // RDP_G_RDPSETOTHERMODE: w0 = (opcode<<24) | other_mode_h (low 24), w1 = other_mode_l
    Fast::F3DGfx dl[2];
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_RDPSETOTHERMODE << 24,
                    Z_CMP | Z_UPD);
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_UPD, 0u);
}

TEST_F(DisplayListTest, ZmodeDecal_ViaDisplayList) {
    Fast::F3DGfx dl[2];
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_RDPSETOTHERMODE << 24,
                    ZMODE_DEC);
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    EXPECT_EQ(interp->mRdp->other_mode_l & ZMODE_DEC, ZMODE_DEC);
}

TEST_F(DisplayListTest, DepthAndGeometryZBuffer_ViaDisplayList) {
    // Set G_ZBUFFER in geometry mode and depth flags in other_mode
    // This combination is what triggers depth_test in the triangle pipeline
    Fast::F3DGfx dl[3];
    // Set geometry mode with G_ZBUFFER
    uint32_t clearBits = 0;
    dl[0] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_GEOMETRYMODE << 24 | (~clearBits & 0xFFFFFF),
                    G_ZBUFFER);
    // Set other_mode_l with Z_CMP | Z_UPD
    dl[1] = MakeCmd((uintptr_t)(uint8_t)Fast::RDP_G_RDPSETOTHERMODE << 24,
                    Z_CMP | Z_UPD);
    dl[2] = MakeCmd((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL << 24, 0);

    interp->RunDisplayListForTest((Gfx*)dl, emptyMtxReplacements);

    // Verify both conditions for depth test are met
    EXPECT_NE(interp->mRsp->geometry_mode & G_ZBUFFER, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_CMP, 0u);
    EXPECT_NE(interp->mRdp->other_mode_l & Z_UPD, 0u);

    // Compute expected depth flags the same way the interpreter does
    bool depth_test = (interp->mRsp->geometry_mode & G_ZBUFFER) && (interp->mRdp->other_mode_l & Z_CMP);
    bool depth_mask = (interp->mRdp->other_mode_l & Z_UPD) != 0;
    EXPECT_TRUE(depth_test);
    EXPECT_TRUE(depth_mask);
}
