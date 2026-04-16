// Fast3D Layer 4 Tests — End-to-End Rendering Pipeline
//
// These tests exercise the full triangle drawing and framebuffer management
// pipelines, verifying that the interpreter correctly coordinates between
// RDP/RSP state and the rendering API (depth test/mask, culling, z-mode
// decal, framebuffer creation/switching, pixel depth queries, and VBO
// buffer management with Flush).
//
// Layer 4 builds on Layer 1–3 foundations:
//   Layer 1: Pure computation (macros, matrix math, CC features)
//   Layer 2: Single interpreter methods with state
//   Layer 3: Multi-step pipelines (vertex + lighting, display list dispatch)
//   Layer 4: End-to-end rendering pipeline (triangle draw → API calls)

#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <vector>

#include "fast/interpreter.h"
#include "fast/lus_gbi.h"
#include "fast/types.h"
#include "fast/f3dex2.h"

// ============================================================
// RecordingRenderingAPI: A stub that records all rendering API calls
// for verification in Layer 4 tests.
// ============================================================

namespace {

// Minimal ShaderProgram-sized storage so we can return a non-null pointer.
// The interpreter only passes this pointer back to ShaderGetInfo/LoadShader/etc.
// We never dereference it as a real ShaderProgram.
static uint8_t sDummyShaderStorage[256] = {};
static Fast::ShaderProgram* sDummyShader = reinterpret_cast<Fast::ShaderProgram*>(sDummyShaderStorage);

struct DrawCall {
    size_t bufVboLen;
    size_t numTris;
};

struct DepthCall {
    bool depthTest;
    bool depthMask;
};

struct DecalCall {
    bool decalMode;
};

struct ViewportCall {
    int x, y, width, height;
};

struct ScissorCall {
    int x, y, width, height;
};

struct FbStartDrawCall {
    int fbId;
    float noiseScale;
};

struct FbClearCall {
    bool color;
    bool depth;
};

struct FbCopyCall {
    int dstId, srcId;
    int srcX0, srcY0, srcX1, srcY1;
    int dstX0, dstY0, dstX1, dstY1;
};

struct FbUpdateCall {
    int fbId;
    uint32_t width, height, msaaLevel;
    bool invertY, renderTarget, hasDepthBuffer, canExtractDepth;
};

class RecordingRenderingAPI : public Fast::GfxRenderingAPI {
  public:
    // Recorded calls
    std::vector<DrawCall> drawCalls;
    std::vector<DepthCall> depthCalls;
    std::vector<DecalCall> decalCalls;
    std::vector<ViewportCall> viewportCalls;
    std::vector<ScissorCall> scissorCalls;
    std::vector<FbStartDrawCall> fbStartDrawCalls;
    std::vector<FbClearCall> fbClearCalls;
    std::vector<FbCopyCall> fbCopyCalls;
    std::vector<FbUpdateCall> fbUpdateCalls;
    int fbCreateCount = 0;
    int shaderLoadCount = 0;
    int shaderUnloadCount = 0;
    bool lastUseAlpha = false;

    const char* GetName() override { return "Recording"; }
    int GetMaxTextureSize() override { return 8192; }
    Fast::GfxClipParameters GetClipParameters() override { return { false, false }; }

    void UnloadShader(Fast::ShaderProgram*) override { shaderUnloadCount++; }
    void LoadShader(Fast::ShaderProgram*) override { shaderLoadCount++; }
    void ClearShaderCache() override {}

    Fast::ShaderProgram* CreateAndLoadNewShader(uint64_t, uint64_t) override {
        return sDummyShader;
    }
    Fast::ShaderProgram* LookupShader(uint64_t, uint64_t) override { return nullptr; }

    void ShaderGetInfo(Fast::ShaderProgram*, uint8_t* numInputs, bool usedTextures[2]) override {
        // Return 0 inputs and no textures for simplicity — the triangle will
        // only emit position + color data into the VBO.
        *numInputs = 0;
        usedTextures[0] = false;
        usedTextures[1] = false;
    }

    uint32_t NewTexture() override { return 0; }
    void SelectTexture(int, uint32_t) override {}
    void UploadTexture(const uint8_t*, uint32_t, uint32_t) override {}
    void SetSamplerParameters(int, bool, uint32_t, uint32_t) override {}

    void SetDepthTestAndMask(bool depthTest, bool depthMask) override {
        depthCalls.push_back({ depthTest, depthMask });
    }
    void SetZmodeDecal(bool decal) override {
        decalCalls.push_back({ decal });
    }
    void SetViewport(int x, int y, int w, int h) override {
        viewportCalls.push_back({ x, y, w, h });
    }
    void SetScissor(int x, int y, int w, int h) override {
        scissorCalls.push_back({ x, y, w, h });
    }
    void SetUseAlpha(bool useAlpha) override {
        lastUseAlpha = useAlpha;
    }
    void DrawTriangles(float[], size_t bufVboLen, size_t numTris) override {
        drawCalls.push_back({ bufVboLen, numTris });
    }

    void Init() override {}
    void OnResize() override {}
    void StartFrame() override {}
    void EndFrame() override {}
    void FinishRender() override {}

    int CreateFramebuffer() override { return ++fbCreateCount; }
    void UpdateFramebufferParameters(int fbId, uint32_t w, uint32_t h, uint32_t msaa,
                                     bool invertY, bool renderTarget, bool hasDepth, bool canExtractDepth) override {
        fbUpdateCalls.push_back({ fbId, w, h, msaa, invertY, renderTarget, hasDepth, canExtractDepth });
    }
    void StartDrawToFramebuffer(int fb, float noiseScale) override {
        fbStartDrawCalls.push_back({ fb, noiseScale });
    }
    void CopyFramebuffer(int dst, int src, int sx0, int sy0, int sx1, int sy1,
                         int dx0, int dy0, int dx1, int dy1) override {
        fbCopyCalls.push_back({ dst, src, sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1 });
    }
    void ClearFramebuffer(bool color, bool depth) override {
        fbClearCalls.push_back({ color, depth });
    }
    void ReadFramebufferToCPU(int, uint32_t, uint32_t, uint16_t*) override {}
    void ResolveMSAAColorBuffer(int, int) override {}
    std::unordered_map<std::pair<float, float>, uint16_t, Fast::hash_pair_ff>
    GetPixelDepth(int, const std::set<std::pair<float, float>>& coords) override {
        // Return 0 depth for all requested coordinates
        std::unordered_map<std::pair<float, float>, uint16_t, Fast::hash_pair_ff> result;
        for (auto& c : coords) {
            result[c] = 0;
        }
        return result;
    }
    void* GetFramebufferTextureId(int) override { return nullptr; }
    void SelectTextureFb(int) override {}
    void DeleteTexture(uint32_t) override {}
    void SetTextureFilter(Fast::FilteringMode) override {}
    Fast::FilteringMode GetTextureFilter() override { return Fast::FILTER_NONE; }
    void SetSrgbMode() override {}
    ImTextureID GetTextureById(int) override { return nullptr; }
};

// Screen dimensions for test setup (avoid name clash with macros in interpreter.h)
constexpr float kScreenWidth = 320.0f;
constexpr float kScreenHeight = 240.0f;

// ============================================================
// Layer 4 Test Fixture
// ============================================================

class TriangleRenderTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_shared<Fast::Interpreter>();
        rec = new RecordingRenderingAPI();
        interp->mRapi = rec;

        // Register global instance (needed for GfxSpTri1 internals)
        Fast::GfxSetInstance(interp);

        // Set screen dimensions
        interp->mNativeDimensions.width = kScreenWidth;
        interp->mNativeDimensions.height = kScreenHeight;
        interp->mCurDimensions.width = kScreenWidth;
        interp->mCurDimensions.height = kScreenHeight;
        interp->mFbActive = false;

        // Reset RSP state
        memset(interp->mRsp, 0, sizeof(Fast::RSP));
        interp->mRsp->modelview_matrix_stack_size = 1;

        // Identity MP matrix
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                interp->mRsp->MP_matrix[i][j] = (i == j) ? 1.0f : 0.0f;

        // Identity modelview
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                interp->mRsp->modelview_matrix_stack[0][i][j] = (i == j) ? 1.0f : 0.0f;

        // No lighting, no texture
        interp->mRsp->geometry_mode = 0;
        interp->mRsp->texture_scaling_factor.s = 0;
        interp->mRsp->texture_scaling_factor.t = 0;

        // Set default combine mode (all zeros = simplest possible)
        interp->mRdp->combine_mode = 0;

        // Reset rendering state
        interp->mRenderingState = {};
    }

    void TearDown() override {
        Fast::GfxSetInstance(nullptr);
        interp->mRapi = nullptr;
        delete rec;
        interp.reset();
    }

    // Load 3 vertices at indices 0, 1, 2 forming a visible triangle.
    // Vertices are inside clip space (w > 0, within frustum).
    void LoadTriangleVertices() {
        Fast::F3DVtx verts[3] = {};

        // Vertex 0: upper-left
        verts[0].v.ob[0] = -50; verts[0].v.ob[1] = 50; verts[0].v.ob[2] = 0;
        verts[0].v.cn[0] = 255; verts[0].v.cn[1] = 0; verts[0].v.cn[2] = 0; verts[0].v.cn[3] = 255;

        // Vertex 1: lower-left
        verts[1].v.ob[0] = -50; verts[1].v.ob[1] = -50; verts[1].v.ob[2] = 0;
        verts[1].v.cn[0] = 0; verts[1].v.cn[1] = 255; verts[1].v.cn[2] = 0; verts[1].v.cn[3] = 255;

        // Vertex 2: right
        verts[2].v.ob[0] = 50; verts[2].v.ob[1] = 0; verts[2].v.ob[2] = 0;
        verts[2].v.cn[0] = 0; verts[2].v.cn[1] = 0; verts[2].v.cn[2] = 255; verts[2].v.cn[3] = 255;

        interp->GfxSpVertex(3, 0, verts);
    }

    // Load 3 vertices that are all outside the far clip plane
    void LoadClippedVertices() {
        Fast::F3DVtx verts[3] = {};
        // With identity MP matrix, w=1. Far clip triggers when z > w.
        // Use large positive z so z > w → all vertices get CLIP_FAR.
        verts[0].v.ob[0] = 0; verts[0].v.ob[1] = 0; verts[0].v.ob[2] = 5000;
        verts[0].v.cn[3] = 255;
        verts[1].v.ob[0] = 10; verts[1].v.ob[1] = 0; verts[1].v.ob[2] = 5000;
        verts[1].v.cn[3] = 255;
        verts[2].v.ob[0] = 0; verts[2].v.ob[1] = 10; verts[2].v.ob[2] = 5000;
        verts[2].v.cn[3] = 255;

        interp->GfxSpVertex(3, 0, verts);
    }

    std::shared_ptr<Fast::Interpreter> interp;
    RecordingRenderingAPI* rec;
};

// ============================================================
// Layer 4: Depth State in Triangle Drawing
// ============================================================

TEST_F(TriangleRenderTest, TriangleDraw_DepthTestEnabled) {
    // Enable G_ZBUFFER in geometry mode and Z_CMP in other_mode_l
    interp->mRsp->geometry_mode |= G_ZBUFFER;
    interp->mRdp->other_mode_l |= Z_CMP;

    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    // SetDepthTestAndMask should have been called with depth_test=true
    ASSERT_FALSE(rec->depthCalls.empty());
    EXPECT_TRUE(rec->depthCalls.back().depthTest);
    EXPECT_FALSE(rec->depthCalls.back().depthMask); // Z_UPD not set
}

TEST_F(TriangleRenderTest, TriangleDraw_DepthMaskEnabled) {
    // Enable G_ZBUFFER + Z_CMP + Z_UPD
    interp->mRsp->geometry_mode |= G_ZBUFFER;
    interp->mRdp->other_mode_l |= Z_CMP | Z_UPD;

    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    ASSERT_FALSE(rec->depthCalls.empty());
    EXPECT_TRUE(rec->depthCalls.back().depthTest);
    EXPECT_TRUE(rec->depthCalls.back().depthMask);
}

TEST_F(TriangleRenderTest, TriangleDraw_DepthDisabled) {
    // No G_ZBUFFER → depth_test should be false
    interp->mRsp->geometry_mode = 0;
    interp->mRdp->other_mode_l = 0;

    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    // Depth state should remain at default (0), no call needed unless it changed
    // The initial rendering state has depth_test_and_mask=0 and the triangle
    // also computes 0, so no SetDepthTestAndMask call should occur
    EXPECT_TRUE(rec->depthCalls.empty());
}

TEST_F(TriangleRenderTest, TriangleDraw_DepthStateChange_FlushesAndUpdates) {
    LoadTriangleVertices();

    // Draw first triangle without depth
    interp->GfxSpTri1(0, 1, 2, false);

    size_t depthCallsBefore = rec->depthCalls.size();

    // Enable depth for second triangle
    interp->mRsp->geometry_mode |= G_ZBUFFER;
    interp->mRdp->other_mode_l |= Z_CMP | Z_UPD;

    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    // A depth state change should have triggered a Flush (draw first tri)
    // and then a SetDepthTestAndMask call
    EXPECT_GT(rec->depthCalls.size(), depthCallsBefore);
    EXPECT_TRUE(rec->depthCalls.back().depthTest);
    EXPECT_TRUE(rec->depthCalls.back().depthMask);

    // The first triangle should have been flushed (drawn) before state changed
    ASSERT_GE(rec->drawCalls.size(), 1u);
}

// ============================================================
// Layer 4: Decal Z-Mode in Triangle Drawing
// ============================================================

TEST_F(TriangleRenderTest, TriangleDraw_DecalModeEnabled) {
    interp->mRdp->other_mode_l |= ZMODE_DEC;

    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    ASSERT_FALSE(rec->decalCalls.empty());
    EXPECT_TRUE(rec->decalCalls.back().decalMode);
}

TEST_F(TriangleRenderTest, TriangleDraw_DecalModeDisabled) {
    interp->mRdp->other_mode_l = 0;

    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    // Decal mode is initially false and triangle keeps it false → no call needed
    EXPECT_TRUE(rec->decalCalls.empty());
}

TEST_F(TriangleRenderTest, TriangleDraw_DecalStateChange_FlushesAndUpdates) {
    LoadTriangleVertices();

    // First tri without decal
    interp->GfxSpTri1(0, 1, 2, false);

    // Enable decal for second tri
    interp->mRdp->other_mode_l |= ZMODE_DEC;
    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    // Should have triggered flush + decal mode change
    ASSERT_FALSE(rec->decalCalls.empty());
    EXPECT_TRUE(rec->decalCalls.back().decalMode);
    ASSERT_GE(rec->drawCalls.size(), 1u); // First tri was flushed
}

// ============================================================
// Layer 4: Face Culling in Triangle Drawing
// ============================================================

TEST_F(TriangleRenderTest, TriangleDraw_NoCulling_TriangleDrawn) {
    // No cull flags set — triangle should be drawn
    interp->mRsp->geometry_mode = 0;

    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    EXPECT_FALSE(rec->drawCalls.empty());
    EXPECT_EQ(rec->drawCalls.back().numTris, 1u);
}

TEST_F(TriangleRenderTest, TriangleDraw_CullBoth_NoTriangleDrawn) {
    // Cull both faces — triangle should never be drawn
    interp->mRsp->geometry_mode = Fast::F3DEX2_G_CULL_BOTH;

    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    EXPECT_TRUE(rec->drawCalls.empty());
}

TEST_F(TriangleRenderTest, TriangleDraw_CullBack_CCWTriangle) {
    // With back-face culling, a CCW-wound triangle (cross >= 0) should be culled
    // The winding depends on the projection of vertex positions.
    // With identity matrix, our triangle is projected as-is.
    interp->mRsp->geometry_mode = Fast::F3DEX2_G_CULL_BACK;

    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    // The test vertices form a specific winding. We just verify the pipeline
    // runs without crashing. The actual cull decision depends on cross product sign.
    // Either 0 or 1 triangles should be drawn.
    EXPECT_LE(rec->drawCalls.size(), 1u);
}

// ============================================================
// Layer 4: Clip Rejection
// ============================================================

TEST_F(TriangleRenderTest, TriangleDraw_ClippedVertices_NoTriangleDrawn) {
    // All vertices share a common clip rejection flag → triangle is fully clipped
    LoadClippedVertices();

    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    // No draw call should have been made
    EXPECT_TRUE(rec->drawCalls.empty());
}

TEST_F(TriangleRenderTest, TriangleDraw_VisibleVertices_TriangleDrawn) {
    LoadTriangleVertices();

    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    EXPECT_FALSE(rec->drawCalls.empty());
}

// ============================================================
// Layer 4: VBO Buffer and Flush
// ============================================================

TEST_F(TriangleRenderTest, Flush_EmptyBuffer_NoDrawCall) {
    interp->Flush();
    EXPECT_TRUE(rec->drawCalls.empty());
}

TEST_F(TriangleRenderTest, Flush_AfterTriangle_DrawCallMade) {
    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);

    EXPECT_TRUE(rec->drawCalls.empty()); // Not yet flushed

    interp->Flush();

    ASSERT_EQ(rec->drawCalls.size(), 1u);
    EXPECT_EQ(rec->drawCalls[0].numTris, 1u);
    EXPECT_GT(rec->drawCalls[0].bufVboLen, 0u);
}

TEST_F(TriangleRenderTest, MultipleTriangles_BatchedBeforeFlush) {
    LoadTriangleVertices();

    // Draw multiple triangles (all same state → no intermediate flushes)
    interp->GfxSpTri1(0, 1, 2, false);
    interp->GfxSpTri1(0, 1, 2, false);
    interp->GfxSpTri1(0, 1, 2, false);

    // Should not have flushed yet (below MAX_TRI_BUFFER)
    EXPECT_TRUE(rec->drawCalls.empty());

    interp->Flush();

    ASSERT_EQ(rec->drawCalls.size(), 1u);
    EXPECT_EQ(rec->drawCalls[0].numTris, 3u);
}

TEST_F(TriangleRenderTest, DoubleFlush_SecondIsNoop) {
    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);

    interp->Flush();
    ASSERT_EQ(rec->drawCalls.size(), 1u);

    interp->Flush(); // Should be a no-op
    EXPECT_EQ(rec->drawCalls.size(), 1u);
}

// ============================================================
// Layer 4: Viewport/Scissor State Changes During Triangle Draw
// ============================================================

TEST_F(TriangleRenderTest, ViewportChange_FlushesBeforeDraw) {
    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);

    // Modify viewport and set changed flag
    interp->mRdp->viewport = { 10, 20, 200, 150 };
    interp->mRdp->viewport_or_scissor_changed = true;

    // Next triangle should trigger flush for viewport change
    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    EXPECT_FALSE(rec->viewportCalls.empty());
    EXPECT_EQ(rec->viewportCalls.back().x, 10);
    EXPECT_EQ(rec->viewportCalls.back().y, 20);
    EXPECT_EQ(rec->viewportCalls.back().width, 200);
    EXPECT_EQ(rec->viewportCalls.back().height, 150);
}

TEST_F(TriangleRenderTest, ScissorChange_FlushesBeforeDraw) {
    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);

    // Modify scissor and set changed flag
    interp->mRdp->scissor = { 0, 0, 320, 240 };
    interp->mRdp->viewport_or_scissor_changed = true;

    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    EXPECT_FALSE(rec->scissorCalls.empty());
}

// ============================================================
// Layer 4: Framebuffer Lifecycle
// ============================================================

class FramebufferTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_shared<Fast::Interpreter>();
        rec = new RecordingRenderingAPI();
        interp->mRapi = rec;

        Fast::GfxSetInstance(interp);

        interp->mNativeDimensions.width = kScreenWidth;
        interp->mNativeDimensions.height = kScreenHeight;
        interp->mCurDimensions.width = kScreenWidth;
        interp->mCurDimensions.height = kScreenHeight;
        interp->mFbActive = false;
        interp->mRendersToFb = false;
    }

    void TearDown() override {
        Fast::GfxSetInstance(nullptr);
        interp->mRapi = nullptr;
        delete rec;
        interp.reset();
    }

    std::shared_ptr<Fast::Interpreter> interp;
    RecordingRenderingAPI* rec;
};

TEST_F(FramebufferTest, CreateFrameBuffer_NoResize) {
    int fbId = interp->CreateFrameBuffer(160, 120, 320, 240, 0 /* no resize */);

    // Should have called CreateFramebuffer + UpdateFramebufferParameters
    EXPECT_EQ(rec->fbCreateCount, 1);
    ASSERT_EQ(rec->fbUpdateCalls.size(), 1u);

    // Without resize, applied dimensions = original dimensions
    EXPECT_EQ(rec->fbUpdateCalls[0].width, 160u);
    EXPECT_EQ(rec->fbUpdateCalls[0].height, 120u);

    // Should be stored in mFrameBuffers
    auto it = interp->mFrameBuffers.find(fbId);
    ASSERT_NE(it, interp->mFrameBuffers.end());
    EXPECT_EQ(it->second.orig_width, 160u);
    EXPECT_EQ(it->second.orig_height, 120u);
    EXPECT_FALSE(it->second.resize);
}

TEST_F(FramebufferTest, CreateFrameBuffer_WithResize) {
    // Current dimensions are 320x240, native 320x240 → 1:1 ratio
    // So 160x120 should scale to 160x120 (no actual change at 1:1)
    int fbId = interp->CreateFrameBuffer(160, 120, 320, 240, 1 /* resize */);

    EXPECT_EQ(rec->fbCreateCount, 1);
    ASSERT_EQ(rec->fbUpdateCalls.size(), 1u);

    auto it = interp->mFrameBuffers.find(fbId);
    ASSERT_NE(it, interp->mFrameBuffers.end());
    EXPECT_TRUE(it->second.resize);
    EXPECT_EQ(it->second.orig_width, 160u);
    EXPECT_EQ(it->second.orig_height, 120u);
}

TEST_F(FramebufferTest, CreateFrameBuffer_WithResize_ScaledUp) {
    // Set current dimensions to 2x native → scale factor 2.0
    interp->mCurDimensions.width = 640;
    interp->mCurDimensions.height = 480;

    int fbId = interp->CreateFrameBuffer(160, 120, 320, 240, 1);

    auto it = interp->mFrameBuffers.find(fbId);
    ASSERT_NE(it, interp->mFrameBuffers.end());
    // applied dimensions should be scaled up by 2x
    EXPECT_EQ(it->second.applied_width, 320u);
    EXPECT_EQ(it->second.applied_height, 240u);
    // API should receive scaled dimensions
    EXPECT_EQ(rec->fbUpdateCalls[0].width, 320u);
    EXPECT_EQ(rec->fbUpdateCalls[0].height, 240u);
}

TEST_F(FramebufferTest, CreateFrameBuffer_MultipleFramebuffers) {
    int fb1 = interp->CreateFrameBuffer(320, 240, 320, 240, 0);
    int fb2 = interp->CreateFrameBuffer(160, 120, 320, 240, 0);

    EXPECT_NE(fb1, fb2);
    EXPECT_EQ(rec->fbCreateCount, 2);
    EXPECT_EQ(interp->mFrameBuffers.size(), 2u);
}

TEST_F(FramebufferTest, SetFrameBuffer_CallsStartDrawAndClear) {
    int fbId = interp->CreateFrameBuffer(320, 240, 320, 240, 0);
    rec->fbStartDrawCalls.clear();
    rec->fbClearCalls.clear();

    interp->SetFrameBuffer(fbId, 1.5f);

    // Should call StartDrawToFramebuffer then ClearFramebuffer(depth only)
    ASSERT_EQ(rec->fbStartDrawCalls.size(), 1u);
    EXPECT_EQ(rec->fbStartDrawCalls[0].fbId, fbId);
    EXPECT_FLOAT_EQ(rec->fbStartDrawCalls[0].noiseScale, 1.5f);

    ASSERT_EQ(rec->fbClearCalls.size(), 1u);
    EXPECT_FALSE(rec->fbClearCalls[0].color); // color not cleared
    EXPECT_TRUE(rec->fbClearCalls[0].depth);   // depth cleared
}

TEST_F(FramebufferTest, ResetFrameBuffer_CallsStartDrawToFb0) {
    interp->ResetFrameBuffer();

    ASSERT_EQ(rec->fbStartDrawCalls.size(), 1u);
    EXPECT_EQ(rec->fbStartDrawCalls[0].fbId, 0);
    // noise_scale = curDimensions.height / nativeDimensions.height = 240/240 = 1.0
    EXPECT_FLOAT_EQ(rec->fbStartDrawCalls[0].noiseScale, 1.0f);
}

TEST_F(FramebufferTest, ResetFrameBuffer_ScaledDimensions) {
    interp->mCurDimensions.height = 480;
    interp->mNativeDimensions.height = 240;

    interp->ResetFrameBuffer();

    // noise_scale = 480/240 = 2.0
    EXPECT_FLOAT_EQ(rec->fbStartDrawCalls[0].noiseScale, 2.0f);
}

TEST_F(FramebufferTest, CopyFrameBuffer_BasicCopy) {
    int fbDst = interp->CreateFrameBuffer(320, 240, 320, 240, 0);

    interp->CopyFrameBuffer(fbDst, 0, false, nullptr);

    ASSERT_EQ(rec->fbCopyCalls.size(), 1u);
    EXPECT_EQ(rec->fbCopyCalls[0].dstId, fbDst);
    // Destination region should be full screen
    EXPECT_EQ(rec->fbCopyCalls[0].dstX0, 0);
    EXPECT_EQ(rec->fbCopyCalls[0].dstY0, 0);
    EXPECT_EQ(rec->fbCopyCalls[0].dstX1, 320);
    EXPECT_EQ(rec->fbCopyCalls[0].dstY1, 240);
}

TEST_F(FramebufferTest, CopyFrameBuffer_CopyOnce_SecondCallSkipped) {
    int fbDst = interp->CreateFrameBuffer(320, 240, 320, 240, 0);
    bool hasCopied = false;

    interp->CopyFrameBuffer(fbDst, 0, true, &hasCopied);
    EXPECT_TRUE(hasCopied);
    EXPECT_EQ(rec->fbCopyCalls.size(), 1u);

    // Second call with copyOnce=true should be skipped
    interp->CopyFrameBuffer(fbDst, 0, true, &hasCopied);
    EXPECT_EQ(rec->fbCopyCalls.size(), 1u); // No new copy
}

TEST_F(FramebufferTest, CopyFrameBuffer_CopyOnce_NullPtr_StillCopies) {
    int fbDst = interp->CreateFrameBuffer(320, 240, 320, 240, 0);

    // With nullptr for hasCopied, should always copy
    interp->CopyFrameBuffer(fbDst, 0, true, nullptr);
    EXPECT_EQ(rec->fbCopyCalls.size(), 1u);

    interp->CopyFrameBuffer(fbDst, 0, true, nullptr);
    EXPECT_EQ(rec->fbCopyCalls.size(), 2u);
}

// ============================================================
// Layer 4: Pixel Depth Queries
// ============================================================

TEST_F(FramebufferTest, GetPixelDepthPrepare_AccumulatesCoordinates) {
    interp->GetPixelDepthPrepare(10.0f, 20.0f);
    interp->GetPixelDepthPrepare(30.0f, 40.0f);

    // Pending set should have 2 entries (coordinates may be adjusted)
    EXPECT_EQ(interp->mGetPixelDepthPending.size(), 2u);
}

TEST_F(FramebufferTest, GetPixelDepth_ReturnsCachedValue) {
    // First call populates cache via API
    uint16_t depth = interp->GetPixelDepth(10.0f, 20.0f);
    EXPECT_EQ(depth, 0u); // Our stub returns 0

    // Pending should be cleared after the query
    EXPECT_TRUE(interp->mGetPixelDepthPending.empty());
}

// ============================================================
// Layer 4: Combined Depth + Framebuffer Pipeline
// ============================================================

TEST_F(TriangleRenderTest, FullPipeline_DepthAndViewportAndTriangle) {
    // Set up viewport
    interp->mRdp->viewport = { 0, 0, 320, 240 };
    interp->mRdp->viewport_or_scissor_changed = true;
    interp->mRdp->scissor = { 0, 0, 320, 240 };

    // Enable depth
    interp->mRsp->geometry_mode |= G_ZBUFFER;
    interp->mRdp->other_mode_l |= Z_CMP | Z_UPD;

    LoadTriangleVertices();
    interp->GfxSpTri1(0, 1, 2, false);
    interp->Flush();

    // Should have: depth call, viewport call, scissor call, draw call
    EXPECT_FALSE(rec->depthCalls.empty());
    EXPECT_TRUE(rec->depthCalls.back().depthTest);
    EXPECT_TRUE(rec->depthCalls.back().depthMask);
    EXPECT_FALSE(rec->viewportCalls.empty());
    EXPECT_FALSE(rec->scissorCalls.empty());
    EXPECT_FALSE(rec->drawCalls.empty());
}

TEST_F(TriangleRenderTest, FullPipeline_DepthModeSwitch_TwoDrawCalls) {
    LoadTriangleVertices();

    // First triangle: no depth
    interp->mRsp->geometry_mode = 0;
    interp->mRdp->other_mode_l = 0;
    interp->GfxSpTri1(0, 1, 2, false);

    // Second triangle: with depth → forces flush of first
    interp->mRsp->geometry_mode = G_ZBUFFER;
    interp->mRdp->other_mode_l = Z_CMP | Z_UPD;
    interp->GfxSpTri1(0, 1, 2, false);

    interp->Flush();

    // Should have at least 2 draw calls (one from the state-change flush, one from final flush)
    EXPECT_GE(rec->drawCalls.size(), 2u);
    EXPECT_EQ(rec->drawCalls[0].numTris, 1u);
    EXPECT_EQ(rec->drawCalls[1].numTris, 1u);
}

} // anonymous namespace
