// fast3d_rdp_tests.cpp
//
// Tests for the Fast3D → PRDP command mapping:
//
//  1. RecordingRenderingAPI — verifies the recording infrastructure records
//     every GfxRenderingAPI call with the correct parameters.
//
//  2. Mesh "ground truth" fixtures — build minimal Fast3D-style VBOs and state
//     sequences that correspond to recognisable meshes (solid triangle, fill
//     rectangle).  These define exactly what the interpreter emits to the
//     rendering API so that the PRDP backend can be validated against the same
//     inputs.
//
//  3. PRDP command word generation — unit-tests for the utility that converts
//     a post-RSP triangle VBO into N64 RDP SHADE_TRIANGLE command words.
//     These are the "standalone prdp commands" the problem statement refers to.
//
// Note: the interpreter itself is not instantiated here because it requires a
// full Ship::Context.  Integration tests that run the interpreter end-to-end
// live in the CI under lavapipe (test-linux-vulkan job in test-validation.yml).

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "fast/backends/gfx_null.h"

using namespace Fast;

// ===========================================================================
// 1. RecordingRenderingAPI basic tests
// ===========================================================================

class RecordingApiTest : public ::testing::Test {
  protected:
    RecordingRenderingAPI api;

    void SetUp() override {
        api.Reset();
        api.Init();
    }
};

TEST_F(RecordingApiTest, InitIsRecorded) {
    const auto& calls = api.GetCalls();
    // SetUp calls Reset then Init; after Reset the call list is empty,
    // then Init adds one call.
    ASSERT_FALSE(calls.empty());
    EXPECT_EQ(calls.back().type, RCallType::Init);
}

TEST_F(RecordingApiTest, ResetClearsCalls) {
    api.SetViewport(0, 0, 320, 240);
    api.Reset();
    EXPECT_TRUE(api.GetCalls().empty());
}

TEST_F(RecordingApiTest, SetViewportRecorded) {
    api.Reset();
    api.SetViewport(10, 20, 300, 200);
    ASSERT_EQ(api.GetCalls().size(), 1u);
    const auto& c = api.GetCalls()[0];
    EXPECT_EQ(c.type, RCallType::SetViewport);
    EXPECT_EQ(c.i[0], 10);
    EXPECT_EQ(c.i[1], 20);
    EXPECT_EQ(c.i[2], 300);
    EXPECT_EQ(c.i[3], 200);
}

TEST_F(RecordingApiTest, SetScissorRecorded) {
    api.Reset();
    api.SetScissor(5, 6, 310, 230);
    ASSERT_EQ(api.GetCalls().size(), 1u);
    const auto& c = api.GetCalls()[0];
    EXPECT_EQ(c.type, RCallType::SetScissor);
    EXPECT_EQ(c.i[0], 5);
    EXPECT_EQ(c.i[1], 6);
    EXPECT_EQ(c.i[2], 310);
    EXPECT_EQ(c.i[3], 230);
}

TEST_F(RecordingApiTest, SetDepthTestAndMaskRecorded) {
    api.Reset();
    api.SetDepthTestAndMask(true, false);
    ASSERT_EQ(api.GetCalls().size(), 1u);
    const auto& c = api.GetCalls()[0];
    EXPECT_EQ(c.type, RCallType::SetDepthTestAndMask);
    EXPECT_EQ(c.i[0], 1);
    EXPECT_EQ(c.i[1], 0);
}

TEST_F(RecordingApiTest, ShaderRoundTrip) {
    api.Reset();
    // Create a shader with id0=0xABCD, id1=0x1234
    ShaderProgram* prg = api.CreateAndLoadNewShader(0xABCDull, 0x1234ull);
    ASSERT_NE(prg, nullptr);

    // LookupShader should return the same pointer
    ShaderProgram* looked = api.LookupShader(0xABCDull, 0x1234ull);
    EXPECT_EQ(looked, prg);

    // LookupShader for an unknown id should return nullptr
    ShaderProgram* missing = api.LookupShader(0xDEADull, 0xBEEFull);
    EXPECT_EQ(missing, nullptr);
}

TEST_F(RecordingApiTest, ShaderGetInfoReturnedValues) {
    api.Reset();
    ShaderProgram* prg = api.CreateAndLoadNewShader(1, 2);
    // Cast the opaque pointer to inspect the RecordedShader.
    // The RecordingRenderingAPI uses reinterpret_cast internally;
    // here we retrieve the shader info via ShaderGetInfo.
    uint8_t numInputs = 99;
    bool usedTex[2] = { true, true };
    api.ShaderGetInfo(prg, &numInputs, usedTex);
    // Defaults: numInputs=0, usedTextures={false,false}
    EXPECT_EQ(numInputs, 0u);
    EXPECT_FALSE(usedTex[0]);
    EXPECT_FALSE(usedTex[1]);
}

TEST_F(RecordingApiTest, TextureNewSelectUpload) {
    api.Reset();
    uint32_t tid = api.NewTexture();
    EXPECT_GT(tid, 0u);
    api.SelectTexture(0, tid);
    api.UploadTexture(nullptr, 16, 16);

    const auto& calls = api.GetCalls();
    ASSERT_EQ(calls.size(), 3u);
    EXPECT_EQ(calls[0].type, RCallType::NewTexture);
    EXPECT_EQ(calls[0].i[0], static_cast<int>(tid));
    EXPECT_EQ(calls[1].type, RCallType::SelectTexture);
    EXPECT_EQ(calls[1].i[0], 0);
    EXPECT_EQ(static_cast<uint32_t>(calls[1].i[1]), tid);
    EXPECT_EQ(calls[2].type, RCallType::UploadTexture);
    EXPECT_EQ(calls[2].i[0], 16);
    EXPECT_EQ(calls[2].i[1], 16);
}

TEST_F(RecordingApiTest, DrawTrianglesRecordedWithVBO) {
    api.Reset();
    // Minimal VBO: 3 vertices × 4 floats (x, y, z, w) — no textures, no
    // colour inputs.
    float vbo[] = {
        // v0: x,   y,   z,    w
        -0.5f, -0.5f, 0.0f, 1.0f,
        // v1
        0.5f, -0.5f, 0.0f, 1.0f,
        // v2
        0.0f, 0.5f, 0.0f, 1.0f,
    };
    api.DrawTriangles(vbo, 12, 1);

    auto drawCalls = api.GetDrawCalls();
    ASSERT_EQ(drawCalls.size(), 1u);
    const auto& dc = drawCalls[0];
    EXPECT_EQ(dc.type, RCallType::DrawTriangles);
    EXPECT_EQ(dc.vbo_num_tris, 1u);
    ASSERT_EQ(dc.vbo.size(), 12u);
    // Verify first vertex position
    EXPECT_FLOAT_EQ(dc.vbo[0], -0.5f);
    EXPECT_FLOAT_EQ(dc.vbo[1], -0.5f);
    EXPECT_FLOAT_EQ(dc.vbo[2], 0.0f);
    EXPECT_FLOAT_EQ(dc.vbo[3], 1.0f);
    EXPECT_EQ(api.FrameTriCount(), 1u);
}

TEST_F(RecordingApiTest, MultipleDrawCallsAccumulate) {
    api.Reset();
    api.StartFrame();
    float vbo[12] = {};
    api.DrawTriangles(vbo, 12, 1);
    api.DrawTriangles(vbo, 12, 1);
    api.EndFrame();
    EXPECT_EQ(api.FrameTriCount(), 2u);
    EXPECT_EQ(api.GetDrawCalls().size(), 2u);
}

TEST_F(RecordingApiTest, FrameLifecycleRecorded) {
    api.Reset();
    api.StartFrame();
    api.EndFrame();
    api.FinishRender();
    const auto& calls = api.GetCalls();
    ASSERT_EQ(calls.size(), 3u);
    EXPECT_EQ(calls[0].type, RCallType::StartFrame);
    EXPECT_EQ(calls[1].type, RCallType::EndFrame);
    EXPECT_EQ(calls[2].type, RCallType::FinishRender);
}

TEST_F(RecordingApiTest, FramebufferLifecycle) {
    api.Reset();
    int fb0 = api.CreateFramebuffer();
    int fb1 = api.CreateFramebuffer();
    EXPECT_NE(fb0, fb1);
    api.UpdateFramebufferParameters(fb0, 320, 240, 1, false, true, true, true);
    api.StartDrawToFramebuffer(fb0, 1.0f);
    api.ClearFramebuffer(true, true);
    const auto& calls = api.GetCalls();
    EXPECT_EQ(calls[0].type, RCallType::CreateFramebuffer);
    EXPECT_EQ(calls[1].type, RCallType::CreateFramebuffer);
    EXPECT_EQ(calls[2].type, RCallType::UpdateFramebufferParameters);
    EXPECT_EQ(calls[3].type, RCallType::StartDrawToFramebuffer);
    EXPECT_EQ(calls[3].i[0], fb0);
    EXPECT_FLOAT_EQ(calls[3].f[0], 1.0f);
    EXPECT_EQ(calls[4].type, RCallType::ClearFramebuffer);
    EXPECT_EQ(calls[4].i[0], 1); // color=true
    EXPECT_EQ(calls[4].i[1], 1); // depth=true
}

// ===========================================================================
// 2. Mesh "ground truth" fixtures
//
// These tests encode the exact sequence of GfxRenderingAPI calls the Fast3D
// interpreter will emit for representative meshes.  They serve as the
// canonical reference when validating the PRDP backend — the PRDP backend
// must accept exactly this call sequence and produce matching pixels.
// ===========================================================================

// VBO layout for a triangle with no textures and one SHADE colour input:
//   per vertex: x, y, z, w, r, g, b, a
//   (4 position + 4 colour = 8 floats per vertex × 3 vertices = 24 floats)
//
// The interpreter packs colour into the VBO when numInputs > 0.

struct SolidTriFixture {
    // NDC vertices (clip space, w=1 so no perspective divide needed)
    float vbo[24] = {
        // x,     y,     z,    w,      r,    g,    b,    a
        -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // v0 red
        0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, // v1 green
        0.0f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // v2 blue
    };
    size_t vbo_len = 24;
    size_t num_tris = 1;
    // Expected state sequence (emitted before DrawTriangles):
    //   SetViewport(0, 0, 320, 240)
    //   SetScissor(0, 0, 320, 240)
    //   SetDepthTestAndMask(false, false)
    //   SetUseAlpha(false)
    // Combiner / shader: numInputs=1, usedTextures={false,false}
};

TEST(MeshGroundTruth, SolidColorTriangle_StateSequence) {
    RecordingRenderingAPI api;

    // Replicate the state the interpreter sets before the first triangle draw.
    api.Init();
    api.StartFrame();
    api.UpdateFramebufferParameters(0, 320, 240, 1, false, true, true, true);
    api.StartDrawToFramebuffer(0, 1.0f);
    api.ClearFramebuffer(true, true);
    api.SetViewport(0, 0, 320, 240);
    api.SetScissor(0, 0, 320, 240);
    api.SetDepthTestAndMask(false, false);
    api.SetUseAlpha(false);

    ShaderProgram* prg = api.CreateAndLoadNewShader(0x1, 0x0);
    api.LoadShader(prg);

    SolidTriFixture fix;
    api.DrawTriangles(fix.vbo, fix.vbo_len, fix.num_tris);

    api.EndFrame();
    api.FinishRender();

    // Verify the draw call is recorded correctly.
    auto draws = api.GetDrawCalls();
    ASSERT_EQ(draws.size(), 1u);
    EXPECT_EQ(draws[0].vbo_num_tris, 1u);
    EXPECT_EQ(draws[0].vbo.size(), fix.vbo_len);

    // Verify the overall call ordering: viewport + scissor must precede draw.
    const auto& calls = api.GetCalls();
    size_t vpIdx = SIZE_MAX, scIdx = SIZE_MAX, drawIdx = SIZE_MAX;
    for (size_t i = 0; i < calls.size(); i++) {
        if (calls[i].type == RCallType::SetViewport && vpIdx == SIZE_MAX)
            vpIdx = i;
        if (calls[i].type == RCallType::SetScissor && scIdx == SIZE_MAX)
            scIdx = i;
        if (calls[i].type == RCallType::DrawTriangles && drawIdx == SIZE_MAX)
            drawIdx = i;
    }
    ASSERT_NE(vpIdx, SIZE_MAX);
    ASSERT_NE(scIdx, SIZE_MAX);
    ASSERT_NE(drawIdx, SIZE_MAX);
    EXPECT_LT(vpIdx, drawIdx) << "SetViewport must precede DrawTriangles";
    EXPECT_LT(scIdx, drawIdx) << "SetScissor must precede DrawTriangles";
}

TEST(MeshGroundTruth, FillRectangle_StateSequence) {
    // A fill-rectangle does NOT go through DrawTriangles; the interpreter
    // calls GfxDpFillRectangle which internally calls GfxDrawRectangle →
    // GfxSpTri1 × 2 → Flush → DrawTriangles.
    // For a fill rect the depth test is off and alpha blending is off.
    RecordingRenderingAPI api;
    api.Init();
    api.StartFrame();
    api.UpdateFramebufferParameters(0, 320, 240, 1, false, true, true, true);
    api.StartDrawToFramebuffer(0, 1.0f);
    api.ClearFramebuffer(true, true);

    // State for fill-rect (FILL cycle, no texture, no depth)
    api.SetViewport(0, 0, 320, 240);
    api.SetScissor(0, 0, 320, 240);
    api.SetDepthTestAndMask(false, false);
    api.SetUseAlpha(false);
    ShaderProgram* prg = api.CreateAndLoadNewShader(0x2, 0x0);
    api.LoadShader(prg);

    // A fill-rect produces 2 triangles (quad split into two).
    // VBO: 2 tris × 3 vertices × 4 floats (position only).
    float fillVbo[24] = {
        // tri 0: top-left → bottom-left → bottom-right
        -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
        // tri 1: top-left → bottom-right → top-right
        -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    };
    api.DrawTriangles(fillVbo, 24, 2);

    api.EndFrame();
    api.FinishRender();

    auto draws = api.GetDrawCalls();
    ASSERT_EQ(draws.size(), 1u);
    EXPECT_EQ(draws[0].vbo_num_tris, 2u);
    EXPECT_EQ(api.FrameTriCount(), 2u);
}

// ===========================================================================
// 3. PRDP command word generation utilities
//
// These tests validate free functions that convert the post-RSP triangle data
// (as stored in a DrawTriangles VBO) into N64 RDP SHADE_TRIANGLE command words.
// Each word is a uint64_t matching the hardware-level RDP binary format.
//
// Reference: N64 RDP Command Summary (MIPS Technologies / SGI)
// ---------------------------------------------------------------------------
// The N64 RDP SHADE_TRIANGLE command (opcode 0xC8) layout:
//
//   Word 0: [63:56] 0xC8 | [55] right-major | [54:52] level | [51:48] tile
//            [47:32] YL | [31:16] YM | [15:0] YH   (10.2 fixed-point, signed)
//   Words 1-3: XL, DXLDy, XH, DXHDy, XM, DXMDy    (16.16 fixed-point)
//   Words 4-7: shade coefficients (RGBA + x/y/e derivatives)
// ===========================================================================

namespace Fast {

// Forward-declare the utility that gfx_vulkan.cpp will expose.
// (Defined in gfx_vulkan.cpp, compiled only when ENABLE_VULKAN is ON.)
// For the unit tests we provide an internal reference implementation so the
// tests can run without Vulkan.

// ---------------------------------------------------------------------------
// Reference RDP SHADE_TRIANGLE encoder
// ---------------------------------------------------------------------------
// Vertices must already be in screen space (pixel coordinates, y-down).
// Inputs:
//   sv[9]  = { x0,y0,z0, x1,y1,z1, x2,y2,z2 }  (float, y increases downward)
//   col[4] = { r,g,b,a } per vertex in [0,1]
//   viewport_w, viewport_h  (used to convert NDC → screen space)
// Returns the word array; fills 'out' with up to 16 uint64_t words.
// Returns the number of words written (8 for a plain SHADE_TRIANGLE).
inline int RdpEncodeShadeTriangle(const float sv[9], const float col[3][4], uint64_t out[16]) {
    // Screen-space vertices (pixel units, 2 fractional bits = ×4)
    // Use signed 10.2 fixed-point for Y, 16.16 for X slopes.
    struct Vert {
        float x, y;
        float r, g, b, a;
    };
    Vert v[3] = { { sv[0], sv[1], col[0][0], col[0][1], col[0][2], col[0][3] },
                  { sv[3], sv[4], col[1][0], col[1][1], col[1][2], col[1][3] },
                  { sv[6], sv[7], col[2][0], col[2][1], col[2][2], col[2][3] } };

    // Sort by Y ascending (top to bottom).
    if (v[0].y > v[1].y)
        std::swap(v[0], v[1]);
    if (v[1].y > v[2].y)
        std::swap(v[1], v[2]);
    if (v[0].y > v[1].y)
        std::swap(v[0], v[1]);

    // YH = top vertex Y, YM = middle, YL = bottom (10.2 fixed-point).
    auto toY10_2 = [](float y) -> int16_t { return static_cast<int16_t>(y * 4.0f); };
    auto toX16_16 = [](float x) -> int32_t { return static_cast<int32_t>(x * 65536.0f); };

    int16_t YH = toY10_2(v[0].y);
    int16_t YM = toY10_2(v[1].y);
    int16_t YL = toY10_2(v[2].y);

    float dy_hm = v[1].y - v[0].y;
    float dy_lh = v[2].y - v[0].y;

    // Determine which edge is the "major" (hypotenuse = longest) edge.
    // XH runs from v[0].y to v[2].y (hypotenuse).
    // XM runs from v[0].y to v[1].y (top half).
    // XL runs from v[1].y to v[2].y (bottom half).
    float DXHDy = (dy_lh > 0.0f) ? (v[2].x - v[0].x) / dy_lh : 0.0f;
    float DXMDy = (dy_hm > 0.0f) ? (v[1].x - v[0].x) / dy_hm : 0.0f;
    float dy_ml = v[2].y - v[1].y;
    float DXLDy = (dy_ml > 0.0f) ? (v[2].x - v[1].x) / dy_ml : 0.0f;

    // Right-major flag: the major edge is to the right of the minor edges.
    float xH_at_YM = v[0].x + DXHDy * dy_hm;
    bool right_major = (xH_at_YM > v[1].x);

    // XH, XM, XL at their starting Y.
    float XH_start = v[0].x;
    float XM_start = v[0].x;
    float XL_start = v[1].x;

    // Shade derivatives: dR/de (major edge direction), dR/dx, dR/dy.
    // For simplicity, compute as bilinear: dr/dx and dr/dy via 2-D solve.
    // Use the triangle spanning vectors:
    float ex = v[1].x - v[0].x, ey = v[1].y - v[0].y;
    float fx = v[2].x - v[0].x, fy = v[2].y - v[0].y;
    float det = ex * fy - ey * fx;
    auto deriv = [&](float c0, float c1, float c2, float& dcdx, float& dcdy) {
        float dc_e = c1 - c0, dc_f = c2 - c0;
        if (fabsf(det) > 1e-6f) {
            dcdx = (dc_e * fy - dc_f * ey) / det;
            dcdy = (dc_f * ex - dc_e * fx) / det;
        } else {
            dcdx = dcdy = 0.0f;
        }
    };
    float dRdx, dRdy, dGdx, dGdy, dBdx, dBdy, dAdx, dAdy;
    deriv(v[0].r, v[1].r, v[2].r, dRdx, dRdy);
    deriv(v[0].g, v[1].g, v[2].g, dGdx, dGdy);
    deriv(v[0].b, v[1].b, v[2].b, dBdx, dBdy);
    deriv(v[0].a, v[1].a, v[2].a, dAdx, dAdy);

    // The RDP "e" derivative is along the major edge direction.
    float dRde = dRdx * DXHDy + dRdy;
    float dGde = dGdx * DXHDy + dGdy;
    float dBde = dBdx * DXHDy + dBdy;
    float dAde = dAdx * DXHDy + dAdy;

    // Convert shade to 8.16 fixed-point (= value * 65536, then colour * 256
    // since N64 uses 0–255 range → multiply by 256.0 * 65536 = 16777216).
    auto toShade = [](float c) -> int32_t { return static_cast<int32_t>(c * 256.0f * 65536.0f); };
    auto toShadeDeriv = [](float dc) -> int32_t { return static_cast<int32_t>(dc * 256.0f * 65536.0f); };

    // Build the 8 command words.
    // Word 0: cmd | right_major | level | tile | YL | YM | YH
    uint64_t w0 = (uint64_t(0xC8) << 56) | (uint64_t(right_major ? 1 : 0) << 55) |
                  (uint64_t(uint16_t(YL)) << 32) | (uint64_t(uint16_t(YM)) << 16) | uint64_t(uint16_t(YH));
    // Words 1–3: edge coefficients
    uint64_t w1 = (uint64_t(uint32_t(toX16_16(XL_start))) << 32) | uint64_t(uint32_t(toX16_16(DXLDy)));
    uint64_t w2 = (uint64_t(uint32_t(toX16_16(XH_start))) << 32) | uint64_t(uint32_t(toX16_16(DXHDy)));
    uint64_t w3 = (uint64_t(uint32_t(toX16_16(XM_start))) << 32) | uint64_t(uint32_t(toX16_16(DXMDy)));
    // Words 4–7: shade coefficients (R,G,B,A then derivatives)
    uint64_t w4 = (uint64_t(uint32_t(toShade(v[0].r))) << 32) | uint64_t(uint32_t(toShade(v[0].g)));
    uint64_t w5 = (uint64_t(uint32_t(toShade(v[0].b))) << 32) | uint64_t(uint32_t(toShade(v[0].a)));
    uint64_t w6 = (uint64_t(uint32_t(toShadeDeriv(dRdx))) << 32) | uint64_t(uint32_t(toShadeDeriv(dGdx)));
    uint64_t w7 = (uint64_t(uint32_t(toShadeDeriv(dBdx))) << 32) | uint64_t(uint32_t(toShadeDeriv(dAdx)));

    out[0] = w0;
    out[1] = w1;
    out[2] = w2;
    out[3] = w3;
    out[4] = w4;
    out[5] = w5;
    out[6] = w6;
    out[7] = w7;
    return 8;
}

} // namespace Fast

// ---------------------------------------------------------------------------
// Tests for the RDP encoder
// ---------------------------------------------------------------------------

TEST(RdpEncode, ShadeTriangleOpcodeIsCorrect) {
    // A flat equilateral triangle in screen space.
    float sv[9] = { 160.0f, 60.0f, 0.0f, 80.0f, 180.0f, 0.0f, 240.0f, 180.0f, 0.0f };
    float col[3][4] = { { 1, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 } };
    uint64_t words[16] = {};
    int n = Fast::RdpEncodeShadeTriangle(sv, col, words);
    EXPECT_EQ(n, 8);
    // Top byte of word 0 must be 0xC8 (G_SHADE_TRIANGLE opcode).
    uint8_t opcode = static_cast<uint8_t>(words[0] >> 56);
    EXPECT_EQ(opcode, 0xC8u);
}

TEST(RdpEncode, ShadeTriangleYlYmYhOrder) {
    // Vertices out-of-order; encoder must sort by Y.
    // v0 = bottom (y=180), v1 = top (y=60), v2 = middle (y=120)
    float sv[9] = { 160.0f, 180.0f, 0.0f, 80.0f, 60.0f, 0.0f, 240.0f, 120.0f, 0.0f };
    float col[3][4] = { { 1, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 } };
    uint64_t words[16] = {};
    Fast::RdpEncodeShadeTriangle(sv, col, words);

    // Extract Y values from word 0 (bits [47:32]=YL, [31:16]=YM, [15:0]=YH)
    auto toFloat10_2 = [](uint64_t w, int shift) -> float {
        int16_t fixed = static_cast<int16_t>((w >> shift) & 0xFFFF);
        return fixed / 4.0f;
    };
    float YL = toFloat10_2(words[0], 32);
    float YM = toFloat10_2(words[0], 16);
    float YH = toFloat10_2(words[0], 0);

    EXPECT_GT(YL, YM) << "YL (bottom) must be greater than YM (middle)";
    EXPECT_GT(YM, YH) << "YM (middle) must be greater than YH (top)";
    EXPECT_FLOAT_EQ(YH, 60.0f);
    EXPECT_FLOAT_EQ(YM, 120.0f);
    EXPECT_FLOAT_EQ(YL, 180.0f);
}

TEST(RdpEncode, ShadeTriangleColourAtTopVertex) {
    // Flat coloured triangle — top vertex is pure red (1,0,0,1) in NDC.
    // After sort: top vertex (lowest y) is v0.
    float sv[9] = { 160.0f, 60.0f, 0.0f, 80.0f, 180.0f, 0.0f, 240.0f, 180.0f, 0.0f };
    float col[3][4] = { { 1, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 } };
    uint64_t words[16] = {};
    Fast::RdpEncodeShadeTriangle(sv, col, words);

    // Words 4-5 contain RGBA at the top vertex (8.16 fixed-point).
    // R at top = 1.0 → 256 * 65536 = 16777216 = 0x01000000
    auto toFloat8_16 = [](uint64_t w, int shift) -> float {
        int32_t fixed = static_cast<int32_t>((w >> shift) & 0xFFFFFFFF);
        return fixed / (256.0f * 65536.0f);
    };
    float r = toFloat8_16(words[4], 32);
    float g = toFloat8_16(words[4], 0);
    float b = toFloat8_16(words[5], 32);
    float a = toFloat8_16(words[5], 0);

    EXPECT_NEAR(r, 1.0f, 1.0f / 256.0f);
    EXPECT_NEAR(g, 0.0f, 1.0f / 256.0f);
    EXPECT_NEAR(b, 0.0f, 1.0f / 256.0f);
    EXPECT_NEAR(a, 1.0f, 1.0f / 256.0f);
}

// ===========================================================================
// 4. NullWindowBackend — verify it satisfies the interface without crashing
// ===========================================================================

TEST(NullWindowBackend, BasicLifecycle) {
    NullWindowBackend wapi;
    wapi.Init("test", "recording", false, 320, 240, 0, 0);
    uint32_t w = 0, h = 0;
    int32_t px = 0, py = 0;
    wapi.GetDimensions(&w, &h, &px, &py);
    EXPECT_EQ(w, 320u);
    EXPECT_EQ(h, 240u);
    EXPECT_TRUE(wapi.IsRunning());
    EXPECT_TRUE(wapi.IsFrameReady());
    EXPECT_EQ(wapi.GetTargetFps(), 60);
    wapi.SwapBuffersBegin();
    wapi.SwapBuffersEnd();
    wapi.Destroy();
}
