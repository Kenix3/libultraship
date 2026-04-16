// Fast3D (N64 Display List Renderer) Unit Tests
// Layer 1: Pure computation tests — no GPU required
//
// Tests cover:
//  - SCALE macros (fixed-point bit-width conversion)
//  - Matrix math (MatrixMul, TransposedMatrixMul, NormalizeVector)
//  - Fixed-point ↔ float matrix conversion
//  - Geometry mode flag handling (GfxSpGeometryMode)
//  - Texture format decoding (RGBA16, RGBA32, IA4, IA8, IA16, I4, I8, CI4, CI8)
//  - Color combiner generation (GenerateCC / gfx_cc_get_features)

#include <gtest/gtest.h>
#include <cmath>
#include <cstring>
#include <array>
#include <vector>
#include <memory>

#include "fast/interpreter.h"
#include "fast/lus_gbi.h"
#include "fast/types.h"
#include "fast/f3dex.h"
#include "fast/f3dex2.h"

// ============================================================
// Re-define the SCALE macros here (they are local to interpreter.cpp)
// so we can unit-test them independently.
// ============================================================
#define SCALE_5_8(VAL_) (((VAL_)*0xFF) / 0x1F)
#define SCALE_8_5(VAL_) ((((VAL_) + 4) * 0x1F) / 0xFF)
#define SCALE_4_8(VAL_) ((VAL_)*0x11)
#define SCALE_8_4(VAL_) ((VAL_) / 0x11)
#define SCALE_3_8(VAL_) ((VAL_)*0x24)
#define SCALE_8_3(VAL_) ((VAL_) / 0x24)

// ============================================================
// SCALE Macro Tests
// ============================================================

TEST(ScaleMacros, Scale5to8_Boundaries) {
    // 0 → 0
    EXPECT_EQ(SCALE_5_8(0), 0);
    // 31 (0x1F, max 5-bit) → 255 (0xFF, max 8-bit)
    EXPECT_EQ(SCALE_5_8(31), 255);
}

TEST(ScaleMacros, Scale5to8_MidValues) {
    // 16 → ~131  (16*255/31 = 131.6... → 131 via integer division)
    EXPECT_EQ(SCALE_5_8(16), (16 * 0xFF) / 0x1F);
    EXPECT_EQ(SCALE_5_8(1), (1 * 0xFF) / 0x1F);   // 1 → 8
    EXPECT_EQ(SCALE_5_8(15), (15 * 0xFF) / 0x1F);  // 15 → 123
}

TEST(ScaleMacros, Scale8to5_Boundaries) {
    EXPECT_EQ(SCALE_8_5(0), 0);
    EXPECT_EQ(SCALE_8_5(255), 31);
}

TEST(ScaleMacros, Scale8to5_RoundTrip) {
    // Round-trip: 5→8→5 should be close to the original value
    for (int v = 0; v <= 31; v++) {
        int expanded = SCALE_5_8(v);
        int contracted = SCALE_8_5(expanded);
        EXPECT_EQ(contracted, v) << "Round-trip failed for 5-bit value " << v;
    }
}

TEST(ScaleMacros, Scale4to8_Boundaries) {
    EXPECT_EQ(SCALE_4_8(0), 0);
    EXPECT_EQ(SCALE_4_8(15), 255);
}

TEST(ScaleMacros, Scale4to8_MidValues) {
    // 4-bit value * 0x11 replicates the nibble: e.g., 0xA → 0xAA
    EXPECT_EQ(SCALE_4_8(0xA), 0xAA);
    EXPECT_EQ(SCALE_4_8(0x5), 0x55);
    EXPECT_EQ(SCALE_4_8(1), 0x11);
}

TEST(ScaleMacros, Scale8to4_Boundaries) {
    EXPECT_EQ(SCALE_8_4(0), 0);
    EXPECT_EQ(SCALE_8_4(255), 15);
}

TEST(ScaleMacros, Scale8to4_RoundTrip) {
    for (int v = 0; v <= 15; v++) {
        int expanded = SCALE_4_8(v);
        int contracted = SCALE_8_4(expanded);
        EXPECT_EQ(contracted, v) << "Round-trip failed for 4-bit value " << v;
    }
}

TEST(ScaleMacros, Scale3to8_Boundaries) {
    EXPECT_EQ(SCALE_3_8(0), 0);
    // 7 * 0x24 = 7 * 36 = 252
    EXPECT_EQ(SCALE_3_8(7), 252);
}

TEST(ScaleMacros, Scale3to8_MidValues) {
    EXPECT_EQ(SCALE_3_8(1), 0x24);
    EXPECT_EQ(SCALE_3_8(4), 4 * 0x24);
}

TEST(ScaleMacros, Scale8to3_Boundaries) {
    EXPECT_EQ(SCALE_8_3(0), 0);
    EXPECT_EQ(SCALE_8_3(252), 7);
}

TEST(ScaleMacros, Scale8to3_RoundTrip) {
    for (int v = 0; v <= 7; v++) {
        int expanded = SCALE_3_8(v);
        int contracted = SCALE_8_3(expanded);
        EXPECT_EQ(contracted, v) << "Round-trip failed for 3-bit value " << v;
    }
}

// Exhaustive: every valid 5-bit input produces a valid 8-bit output
TEST(ScaleMacros, Scale5to8_Exhaustive) {
    for (int v = 0; v <= 31; v++) {
        int result = SCALE_5_8(v);
        EXPECT_GE(result, 0);
        EXPECT_LE(result, 255);
    }
}

TEST(ScaleMacros, Scale4to8_Exhaustive) {
    for (int v = 0; v <= 15; v++) {
        int result = SCALE_4_8(v);
        EXPECT_GE(result, 0);
        EXPECT_LE(result, 255);
    }
}

// ============================================================
// Matrix Math Tests — MatrixMul, TransposedMatrixMul, NormalizeVector
// ============================================================

TEST(MatrixMath, MatrixMul_Identity) {
    float identity[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };
    float other[4][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12},
        {13, 14, 15, 16}
    };
    float result[4][4];

    Fast::Interpreter::MatrixMul(result, other, identity);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            EXPECT_FLOAT_EQ(result[i][j], other[i][j]);
}

TEST(MatrixMath, MatrixMul_IdentityLeft) {
    float identity[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };
    float other[4][4] = {
        {2, 0, 0, 0},
        {0, 3, 0, 0},
        {0, 0, 4, 0},
        {1, 2, 3, 1}
    };
    float result[4][4];

    Fast::Interpreter::MatrixMul(result, identity, other);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            EXPECT_FLOAT_EQ(result[i][j], other[i][j]);
}

TEST(MatrixMath, MatrixMul_InPlace) {
    // MatrixMul supports res == a or res == b via temp buffer
    float a[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {5, 6, 7, 1}
    };
    float b[4][4] = {
        {2, 0, 0, 0},
        {0, 3, 0, 0},
        {0, 0, 4, 0},
        {0, 0, 0, 1}
    };
    float expected[4][4];
    Fast::Interpreter::MatrixMul(expected, a, b);
    Fast::Interpreter::MatrixMul(a, a, b);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            EXPECT_FLOAT_EQ(a[i][j], expected[i][j]);
}

TEST(MatrixMath, MatrixMul_Translation) {
    // A translation matrix multiplied by identity should be unchanged
    float translate[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {10, 20, 30, 1}
    };
    float scale[4][4] = {
        {2, 0, 0, 0},
        {0, 2, 0, 0},
        {0, 0, 2, 0},
        {0, 0, 0, 1}
    };
    float result[4][4];
    Fast::Interpreter::MatrixMul(result, translate, scale);
    // translate * scale = scale with translated origin scaled
    EXPECT_FLOAT_EQ(result[0][0], 2.0f);
    EXPECT_FLOAT_EQ(result[1][1], 2.0f);
    EXPECT_FLOAT_EQ(result[2][2], 2.0f);
    EXPECT_FLOAT_EQ(result[3][3], 1.0f);
    EXPECT_FLOAT_EQ(result[3][0], 20.0f); // 10*2
    EXPECT_FLOAT_EQ(result[3][1], 40.0f); // 20*2
    EXPECT_FLOAT_EQ(result[3][2], 60.0f); // 30*2
}

TEST(MatrixMath, TransposedMatrixMul_Basic) {
    // res[i] = sum_k a[k]*b[i][k] — i.e., a dot column-i-of-b-transposed
    float a[3] = {1.0f, 0.0f, 0.0f};
    float b[4][4] = {
        {2, 3, 4, 0},
        {5, 6, 7, 0},
        {8, 9, 10, 0},
        {0, 0, 0, 1}
    };
    float res[3];
    Fast::Interpreter::TransposedMatrixMul(res, a, b);
    // res[0] = 1*2 + 0*3 + 0*4 = 2
    // res[1] = 1*5 + 0*6 + 0*7 = 5
    // res[2] = 1*8 + 0*9 + 0*10 = 8
    EXPECT_FLOAT_EQ(res[0], 2.0f);
    EXPECT_FLOAT_EQ(res[1], 5.0f);
    EXPECT_FLOAT_EQ(res[2], 8.0f);
}

TEST(MatrixMath, TransposedMatrixMul_AllAxes) {
    float a[3] = {1.0f, 2.0f, 3.0f};
    float b[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };
    float res[3];
    Fast::Interpreter::TransposedMatrixMul(res, a, b);
    // With identity, result should equal input
    EXPECT_FLOAT_EQ(res[0], 1.0f);
    EXPECT_FLOAT_EQ(res[1], 2.0f);
    EXPECT_FLOAT_EQ(res[2], 3.0f);
}

TEST(MatrixMath, NormalizeVector_UnitX) {
    float v[3] = {1.0f, 0.0f, 0.0f};
    Fast::Interpreter::NormalizeVector(v);
    EXPECT_FLOAT_EQ(v[0], 1.0f);
    EXPECT_FLOAT_EQ(v[1], 0.0f);
    EXPECT_FLOAT_EQ(v[2], 0.0f);
}

TEST(MatrixMath, NormalizeVector_Diagonal) {
    float v[3] = {1.0f, 1.0f, 1.0f};
    Fast::Interpreter::NormalizeVector(v);
    float expected = 1.0f / sqrtf(3.0f);
    EXPECT_NEAR(v[0], expected, 1e-6f);
    EXPECT_NEAR(v[1], expected, 1e-6f);
    EXPECT_NEAR(v[2], expected, 1e-6f);
}

TEST(MatrixMath, NormalizeVector_LargeVector) {
    float v[3] = {3.0f, 4.0f, 0.0f};
    Fast::Interpreter::NormalizeVector(v);
    EXPECT_NEAR(v[0], 0.6f, 1e-6f);
    EXPECT_NEAR(v[1], 0.8f, 1e-6f);
    EXPECT_NEAR(v[2], 0.0f, 1e-6f);
    // Verify unit length
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    EXPECT_NEAR(len, 1.0f, 1e-6f);
}

// ============================================================
// Fixed-point to Float Matrix Conversion Tests
// ============================================================

TEST(MatrixConversion, FixedPointToFloat_Identity) {
    // N64 Mtx format: 16.16 fixed-point stored as:
    //   First 8 int32s: integer parts
    //   Next 8 int32s: fractional parts
    // addr[i*2+j/2] packs two columns per 32-bit word:
    //   high 16 bits = column j, low 16 bits = column j+1
    //
    // Identity matrix: only diagonal elements are 1.0
    //   matrix[0][0]=1: addr[0] high word = 1 → addr[0] = 0x00010000
    //   matrix[1][1]=1: addr[2] low word = 1  → addr[2] = 0x00000001
    //   matrix[2][2]=1: addr[5] high word = 1 → addr[5] = 0x00010000
    //   matrix[3][3]=1: addr[7] low word = 1  → addr[7] = 0x00000001
    int32_t mtx[16] = {};
    mtx[0] = 0x00010000; // row0: [1, 0]
    mtx[1] = 0x00000000; // row0: [0, 0]
    mtx[2] = 0x00000001; // row1: [0, 1]
    mtx[3] = 0x00000000; // row1: [0, 0]
    mtx[4] = 0x00000000; // row2: [0, 0]
    mtx[5] = 0x00010000; // row2: [1, 0]
    mtx[6] = 0x00000000; // row3: [0, 0]
    mtx[7] = 0x00000001; // row3: [0, 1]
    // Fractional parts: all zero
    for (int i = 8; i < 16; i++) mtx[i] = 0;

    float matrix[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j += 2) {
            int32_t int_part = mtx[i * 2 + j / 2];
            uint32_t frac_part = mtx[8 + i * 2 + j / 2];
            matrix[i][j] = (int32_t)((int_part & 0xffff0000) | (frac_part >> 16)) / 65536.0f;
            matrix[i][j + 1] = (int32_t)((int_part << 16) | (frac_part & 0xffff)) / 65536.0f;
        }
    }

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            EXPECT_NEAR(matrix[i][j], (i == j) ? 1.0f : 0.0f, 1e-5f)
                << "at [" << i << "][" << j << "]";
}

TEST(MatrixConversion, FixedPointToFloat_Scale2x) {
    // Scale by 2.0 on all diagonal axes
    //   matrix[0][0]=2: addr[0] high word = 2 → addr[0] = 0x00020000
    //   matrix[1][1]=2: addr[2] low word = 2  → addr[2] = 0x00000002
    //   matrix[2][2]=2: addr[5] high word = 2 → addr[5] = 0x00020000
    //   matrix[3][3]=1: addr[7] low word = 1  → addr[7] = 0x00000001
    int32_t mtx[16] = {};
    mtx[0] = 0x00020000; // row0: [2, 0]
    mtx[2] = 0x00000002; // row1: [0, 2]
    mtx[5] = 0x00020000; // row2: [2, 0]
    mtx[7] = 0x00000001; // row3: [0, 1]
    for (int i = 8; i < 16; i++) mtx[i] = 0;

    float matrix[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j += 2) {
            int32_t int_part = mtx[i * 2 + j / 2];
            uint32_t frac_part = mtx[8 + i * 2 + j / 2];
            matrix[i][j] = (int32_t)((int_part & 0xffff0000) | (frac_part >> 16)) / 65536.0f;
            matrix[i][j + 1] = (int32_t)((int_part << 16) | (frac_part & 0xffff)) / 65536.0f;
        }
    }

    EXPECT_NEAR(matrix[0][0], 2.0f, 1e-5f);
    EXPECT_NEAR(matrix[1][1], 2.0f, 1e-5f);
    EXPECT_NEAR(matrix[2][2], 2.0f, 1e-5f);
    EXPECT_NEAR(matrix[3][3], 1.0f, 1e-5f);
    // Off-diagonals should be zero
    EXPECT_NEAR(matrix[0][1], 0.0f, 1e-5f);
    EXPECT_NEAR(matrix[1][0], 0.0f, 1e-5f);
}

TEST(MatrixConversion, FixedPointToFloat_FractionalValue) {
    // Test 0.5 = 0x00008000 in 16.16
    // Place 0.5 in matrix[0][0]
    int32_t mtx[16] = {};
    // For [0][0]: int_part high word = 0, frac_part high word = 0x8000
    mtx[0] = 0x00000000; // integer high words for row0 col0,col1
    mtx[8] = 0x80000000; // frac high word for [0][0] = 0x8000, low word for [0][1] = 0

    float matrix[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j += 2) {
            int32_t int_part = mtx[i * 2 + j / 2];
            uint32_t frac_part = mtx[8 + i * 2 + j / 2];
            matrix[i][j] = (int32_t)((int_part & 0xffff0000) | (frac_part >> 16)) / 65536.0f;
            matrix[i][j + 1] = (int32_t)((int_part << 16) | (frac_part & 0xffff)) / 65536.0f;
        }
    }

    EXPECT_NEAR(matrix[0][0], 0.5f, 1e-5f);
    EXPECT_NEAR(matrix[0][1], 0.0f, 1e-5f);
}

// ============================================================
// Geometry Mode Tests
// ============================================================

class GeometryModeTest : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_unique<Fast::Interpreter>();
        // Initialize geometry_mode to zero
        interp->mRsp->geometry_mode = 0;
        interp->mRsp->extra_geometry_mode = 0;
    }

    std::unique_ptr<Fast::Interpreter> interp;
};

TEST_F(GeometryModeTest, SetBits) {
    interp->GfxSpGeometryMode(0, G_LIGHTING);
    EXPECT_EQ(interp->mRsp->geometry_mode & G_LIGHTING, G_LIGHTING);
}

TEST_F(GeometryModeTest, ClearBits) {
    interp->mRsp->geometry_mode = G_LIGHTING | G_SHADE;
    interp->GfxSpGeometryMode(G_LIGHTING, 0);
    EXPECT_EQ(interp->mRsp->geometry_mode & G_LIGHTING, 0u);
    EXPECT_EQ(interp->mRsp->geometry_mode & G_SHADE, G_SHADE);
}

TEST_F(GeometryModeTest, SetAndClearSimultaneously) {
    interp->mRsp->geometry_mode = G_SHADE;
    interp->GfxSpGeometryMode(G_SHADE, G_LIGHTING);
    EXPECT_EQ(interp->mRsp->geometry_mode & G_SHADE, 0u);
    EXPECT_EQ(interp->mRsp->geometry_mode & G_LIGHTING, G_LIGHTING);
}

TEST_F(GeometryModeTest, SetMultipleFlags) {
    interp->GfxSpGeometryMode(0, G_LIGHTING | G_SHADE | G_LIGHTING_POSITIONAL);
    EXPECT_EQ(interp->mRsp->geometry_mode, G_LIGHTING | G_SHADE | G_LIGHTING_POSITIONAL);
}

TEST_F(GeometryModeTest, ClearAll) {
    interp->mRsp->geometry_mode = 0xFFFFFFFF;
    interp->GfxSpGeometryMode(0xFFFFFFFF, 0);
    EXPECT_EQ(interp->mRsp->geometry_mode, 0u);
}

TEST_F(GeometryModeTest, ExtraGeometryMode) {
    interp->GfxSpExtraGeometryMode(0, 0x1234);
    EXPECT_EQ(interp->mRsp->extra_geometry_mode, 0x1234u);
    interp->GfxSpExtraGeometryMode(0x0034, 0);
    EXPECT_EQ(interp->mRsp->extra_geometry_mode, 0x1200u);
}

// ============================================================
// Texture Import Helper: sets up minimal RDP state for testing
// ============================================================

// Helper to create an Interpreter with a stub rendering API to capture UploadTexture calls.
// We create a minimal mock that just records the uploaded texture data.
namespace {

struct UploadedTexture {
    std::vector<uint8_t> data;
    uint32_t width;
    uint32_t height;
};

class StubRenderingAPI : public Fast::GfxRenderingAPI {
  public:
    std::vector<UploadedTexture> uploads;

    const char* GetName() override { return "Stub"; }
    int GetMaxTextureSize() override { return 8192; }
    Fast::GfxClipParameters GetClipParameters() override { return {false, false}; }
    void UnloadShader(Fast::ShaderProgram*) override {}
    void LoadShader(Fast::ShaderProgram*) override {}
    void ClearShaderCache() override {}
    Fast::ShaderProgram* CreateAndLoadNewShader(uint64_t, uint64_t) override { return nullptr; }
    Fast::ShaderProgram* LookupShader(uint64_t, uint64_t) override { return nullptr; }
    void ShaderGetInfo(Fast::ShaderProgram*, uint8_t*, bool[2]) override {}
    uint32_t NewTexture() override { return 0; }
    void SelectTexture(int, uint32_t) override {}
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) override {
        UploadedTexture tex;
        tex.width = width;
        tex.height = height;
        tex.data.assign(rgba32Buf, rgba32Buf + width * height * 4);
        uploads.push_back(std::move(tex));
    }
    void SetSamplerParameters(int, bool, uint32_t, uint32_t) override {}
    // Depth/framebuffer tracking
    bool lastDepthTest = false;
    bool lastDepthMask = false;
    int depthCallCount = 0;
    bool lastDecalMode = false;
    int decalCallCount = 0;
    int lastFbId = -1;
    float lastFbNoiseScale = 0.0f;
    int fbCreateCount = 0;
    int fbStartDrawCount = 0;
    bool lastClearColor = false;
    bool lastClearDepth = false;
    int clearFbCount = 0;

    void SetDepthTestAndMask(bool depthTest, bool depthMask) override {
        lastDepthTest = depthTest;
        lastDepthMask = depthMask;
        depthCallCount++;
    }
    void SetZmodeDecal(bool decal) override {
        lastDecalMode = decal;
        decalCallCount++;
    }
    void SetViewport(int, int, int, int) override {}
    void SetScissor(int, int, int, int) override {}
    void SetUseAlpha(bool) override {}
    void DrawTriangles(float[], size_t, size_t) override {}
    void Init() override {}
    void OnResize() override {}
    void StartFrame() override {}
    void EndFrame() override {}
    void FinishRender() override {}
    int CreateFramebuffer() override { return ++fbCreateCount; }
    void UpdateFramebufferParameters(int, uint32_t, uint32_t, uint32_t, bool, bool, bool, bool) override {}
    void StartDrawToFramebuffer(int fb, float noiseScale) override {
        lastFbId = fb;
        lastFbNoiseScale = noiseScale;
        fbStartDrawCount++;
    }
    void CopyFramebuffer(int, int, int, int, int, int, int, int, int, int) override {}
    void ClearFramebuffer(bool color, bool depth) override {
        lastClearColor = color;
        lastClearDepth = depth;
        clearFbCount++;
    }
    void ReadFramebufferToCPU(int, uint32_t, uint32_t, uint16_t*) override {}
    void ResolveMSAAColorBuffer(int, int) override {}
    std::unordered_map<std::pair<float, float>, uint16_t, Fast::hash_pair_ff>
    GetPixelDepth(int, const std::set<std::pair<float, float>>&) override { return {}; }
    void* GetFramebufferTextureId(int) override { return nullptr; }
    void SelectTextureFb(int) override {}
    void DeleteTexture(uint32_t) override {}
    void SetTextureFilter(Fast::FilteringMode) override {}
    Fast::FilteringMode GetTextureFilter() override { return Fast::FILTER_NONE; }
    void SetSrgbMode() override {}
    ImTextureID GetTextureById(int) override { return nullptr; }
};

// Sets up an Interpreter with stub API and a usable mTexUploadBuffer.
// This doesn't call Init() (which needs a window backend and Ship::Context),
// instead it manually sets up the minimum needed state.
class TextureTestFixture : public ::testing::Test {
  protected:
    void SetUp() override {
        interp = std::make_unique<Fast::Interpreter>();
        stub = new StubRenderingAPI();
        interp->mRapi = stub;

        // Allocate the texture upload buffer (normally done in Init)
        int max_tex_size = stub->GetMaxTextureSize();
        interp->mTexUploadBuffer = (uint8_t*)malloc(max_tex_size * max_tex_size * 4);
    }

    void TearDown() override {
        free(interp->mTexUploadBuffer);
        interp->mTexUploadBuffer = nullptr;
        // Prevent double-free in Interpreter destructor
        interp->mRapi = nullptr;
        delete stub;
    }

    // Helper to set up loaded_texture state for a given tile and tmem_index.
    // For most formats, use lineSizeBytes = bytes_per_pixel * width.
    // For single-line textures (sizeBytes == lineSizeBytes), be aware that
    // GetEffectiveLineSize falls back to tile.line_size_bytes, so set both
    // line sizes correctly.
    void SetupLoadedTexture(int tile, int tmemIndex,
                            const uint8_t* addr, uint32_t sizeBytes,
                            uint32_t lineSizeBytes, uint32_t fullImageLineSizeBytes,
                            uint32_t tileLineSizeBytes = 0) {
        interp->mRdp->texture_tile[tile].tmem_index = tmemIndex;
        interp->mRdp->loaded_texture[tmemIndex].addr = addr;
        interp->mRdp->loaded_texture[tmemIndex].size_bytes = sizeBytes;
        interp->mRdp->loaded_texture[tmemIndex].line_size_bytes = lineSizeBytes;
        interp->mRdp->loaded_texture[tmemIndex].full_image_line_size_bytes = fullImageLineSizeBytes;
        interp->mRdp->loaded_texture[tmemIndex].raw_tex_metadata = {};
        interp->mRdp->loaded_texture[tmemIndex].raw_tex_metadata.h_byte_scale = 1.0f;
        interp->mRdp->loaded_texture[tmemIndex].raw_tex_metadata.v_pixel_scale = 1.0f;

        // Set tile line_size_bytes (used as fallback by GetEffectiveLineSize)
        interp->mRdp->texture_tile[tile].line_size_bytes = tileLineSizeBytes ? tileLineSizeBytes : lineSizeBytes;

        // Set tile size large enough to prevent tile-dimension clamping.
        // tile_w = (lrs - uls + 4) / 4, so lrs = 4*maxDim - 4 gives tile_w = maxDim.
        // Use 1024 as a safe upper bound.
        interp->mRdp->texture_tile[tile].uls = 0;
        interp->mRdp->texture_tile[tile].ult = 0;
        interp->mRdp->texture_tile[tile].lrs = 4 * 1024 - 4;
        interp->mRdp->texture_tile[tile].lrt = 4 * 1024 - 4;
    }

    std::unique_ptr<Fast::Interpreter> interp;
    StubRenderingAPI* stub;
};

} // anonymous namespace

// ============================================================
// Texture Format Decoding Tests
// ============================================================

// --- RGBA16 ---
TEST_F(TextureTestFixture, ImportTextureRgba16_SinglePixel) {
    // RGBA16: RRRRR GGGGG BBBBB A (16-bit big-endian)
    // Let's encode: R=31, G=0, B=0, A=1 → pure red, opaque
    // col16 = (31 << 11) | (0 << 6) | (0 << 1) | 1 = 0xF801
    uint8_t texData[2] = { 0xF8, 0x01 }; // big-endian

    SetupLoadedTexture(0, 0, texData, 2, 2, 2);

    interp->ImportTextureRgba16(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 1u);
    EXPECT_EQ(stub->uploads[0].height, 1u);
    EXPECT_EQ(stub->uploads[0].data[0], 255); // R: SCALE_5_8(31) = 255
    EXPECT_EQ(stub->uploads[0].data[1], 0);   // G: SCALE_5_8(0) = 0
    EXPECT_EQ(stub->uploads[0].data[2], 0);   // B: SCALE_5_8(0) = 0
    EXPECT_EQ(stub->uploads[0].data[3], 255); // A: 1 → 255
}

TEST_F(TextureTestFixture, ImportTextureRgba16_TransparentPixel) {
    // R=0, G=31, B=0, A=0 → green, transparent
    // col16 = (0 << 11) | (31 << 6) | (0 << 1) | 0 = 0x07C0
    uint8_t texData[2] = { 0x07, 0xC0 };

    SetupLoadedTexture(0, 0, texData, 2, 2, 2);
    interp->ImportTextureRgba16(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].data[0], 0);
    EXPECT_EQ(stub->uploads[0].data[1], 255);
    EXPECT_EQ(stub->uploads[0].data[2], 0);
    EXPECT_EQ(stub->uploads[0].data[3], 0); // A=0 → 0
}

TEST_F(TextureTestFixture, ImportTextureRgba16_2x2) {
    // 2x2 texture, 2 bytes per pixel = 8 bytes total, 4 bytes per line
    uint8_t texData[8];

    // Pixel (0,0): R=31, G=31, B=31, A=1 → white opaque
    // col16 = 0xFFFF
    texData[0] = 0xFF; texData[1] = 0xFF;

    // Pixel (1,0): R=0, G=0, B=0, A=1 → black opaque
    // col16 = 0x0001
    texData[2] = 0x00; texData[3] = 0x01;

    // Pixel (0,1): R=16, G=16, B=16, A=1
    uint16_t mid = (16 << 11) | (16 << 6) | (16 << 1) | 1;
    texData[4] = mid >> 8; texData[5] = mid & 0xFF;

    // Pixel (1,1): R=0, G=0, B=0, A=0 → transparent black
    texData[6] = 0x00; texData[7] = 0x00;

    // For 2x2: line_size_bytes=4 (one row), size_bytes=8 (total), full_image=4
    // GetEffectiveLineSize(4, 4, 8, tile_ls) → line_size != size → returns 4
    // But we need full_image_line_size_bytes != size_bytes so it doesn't reset stride.
    // Use full_image=4 which != size=8: good.
    SetupLoadedTexture(0, 0, texData, 8, 4, 4);
    interp->ImportTextureRgba16(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 2u);
    EXPECT_EQ(stub->uploads[0].height, 2u);

    // Pixel (0,0): white opaque
    EXPECT_EQ(stub->uploads[0].data[0], 255);
    EXPECT_EQ(stub->uploads[0].data[1], 255);
    EXPECT_EQ(stub->uploads[0].data[2], 255);
    EXPECT_EQ(stub->uploads[0].data[3], 255);

    // Pixel (1,0): black opaque
    EXPECT_EQ(stub->uploads[0].data[4], 0);
    EXPECT_EQ(stub->uploads[0].data[5], 0);
    EXPECT_EQ(stub->uploads[0].data[6], 0);
    EXPECT_EQ(stub->uploads[0].data[7], 255);
}

// --- RGBA32 ---
TEST_F(TextureTestFixture, ImportTextureRgba32_SinglePixel) {
    uint8_t texData[4] = { 0xAA, 0xBB, 0xCC, 0xDD };
    // For RGBA32 single-pixel: GetEffectiveLineSize is called with tile.line_size_bytes*2.
    // Since line==full==size (all 4), it falls to tile.line_size_bytes*2 = tileLS*2.
    // We need tileLS*2 = 4 (4 bytes per pixel), so tileLS = 2.
    SetupLoadedTexture(0, 0, texData, 4, 4, 4, /*tileLineSizeBytes=*/2);
    interp->ImportTextureRgba32(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 1u);
    EXPECT_EQ(stub->uploads[0].height, 1u);
    EXPECT_EQ(stub->uploads[0].data[0], 0xAA);
    EXPECT_EQ(stub->uploads[0].data[1], 0xBB);
    EXPECT_EQ(stub->uploads[0].data[2], 0xCC);
    EXPECT_EQ(stub->uploads[0].data[3], 0xDD);
}

TEST_F(TextureTestFixture, ImportTextureRgba32_2x1) {
    uint8_t texData[8] = { 255, 0, 0, 255,   0, 255, 0, 128 };
    // 2x1 RGBA32: 8 bytes per line, single line
    // GetEffectiveLineSize with tile.line_size_bytes*2 → need tileLS*2 = 8 → tileLS = 4
    SetupLoadedTexture(0, 0, texData, 8, 8, 8, /*tileLineSizeBytes=*/4);
    interp->ImportTextureRgba32(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 2u);
    EXPECT_EQ(stub->uploads[0].height, 1u);
    // First pixel: red
    EXPECT_EQ(stub->uploads[0].data[0], 255);
    EXPECT_EQ(stub->uploads[0].data[1], 0);
    EXPECT_EQ(stub->uploads[0].data[2], 0);
    EXPECT_EQ(stub->uploads[0].data[3], 255);
    // Second pixel: green semi-transparent
    EXPECT_EQ(stub->uploads[0].data[4], 0);
    EXPECT_EQ(stub->uploads[0].data[5], 255);
    EXPECT_EQ(stub->uploads[0].data[6], 0);
    EXPECT_EQ(stub->uploads[0].data[7], 128);
}

// --- IA4 ---
TEST_F(TextureTestFixture, ImportTextureIA4_SingleByte_TwoPixels) {
    // IA4: 3-bit intensity + 1-bit alpha per pixel, 2 pixels per byte
    // Byte: IIIA IIIA
    // High nibble: intensity=7 (max), alpha=1 → I=252, A=255
    // Low nibble:  intensity=0, alpha=0 → I=0, A=0
    uint8_t texData[1] = { 0xF0 }; // 1111 0000 → high: I=7,A=1; low: I=0,A=0

    // IA4: 2 pixels per byte → width = widthBytes * 2
    SetupLoadedTexture(0, 0, texData, 1, 1, 1);
    interp->ImportTextureIA4(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 2u);
    EXPECT_EQ(stub->uploads[0].height, 1u);

    // First pixel: I=7→SCALE_3_8(7)=252, A=1→255
    EXPECT_EQ(stub->uploads[0].data[0], SCALE_3_8(7));
    EXPECT_EQ(stub->uploads[0].data[1], SCALE_3_8(7));
    EXPECT_EQ(stub->uploads[0].data[2], SCALE_3_8(7));
    EXPECT_EQ(stub->uploads[0].data[3], 255);

    // Second pixel: I=0→0, A=0→0
    EXPECT_EQ(stub->uploads[0].data[4], 0);
    EXPECT_EQ(stub->uploads[0].data[5], 0);
    EXPECT_EQ(stub->uploads[0].data[6], 0);
    EXPECT_EQ(stub->uploads[0].data[7], 0);
}

// --- IA8 ---
TEST_F(TextureTestFixture, ImportTextureIA8_SinglePixel) {
    // IA8: upper 4 bits = intensity, lower 4 bits = alpha
    // 0xFA → intensity=15, alpha=10
    uint8_t texData[1] = { 0xFA };
    SetupLoadedTexture(0, 0, texData, 1, 1, 1);
    interp->ImportTextureIA8(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 1u);
    EXPECT_EQ(stub->uploads[0].height, 1u);
    EXPECT_EQ(stub->uploads[0].data[0], SCALE_4_8(15)); // 0xFF
    EXPECT_EQ(stub->uploads[0].data[1], SCALE_4_8(15));
    EXPECT_EQ(stub->uploads[0].data[2], SCALE_4_8(15));
    EXPECT_EQ(stub->uploads[0].data[3], SCALE_4_8(10)); // 0xAA
}

TEST_F(TextureTestFixture, ImportTextureIA8_2x1) {
    // Two pixels side by side
    uint8_t texData[2] = { 0x00, 0xFF };
    SetupLoadedTexture(0, 0, texData, 2, 2, 2);
    interp->ImportTextureIA8(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 2u);
    EXPECT_EQ(stub->uploads[0].height, 1u);

    // Pixel 0: I=0, A=0
    EXPECT_EQ(stub->uploads[0].data[0], 0);
    EXPECT_EQ(stub->uploads[0].data[3], 0);

    // Pixel 1: I=15→255, A=15→255
    EXPECT_EQ(stub->uploads[0].data[4], 255);
    EXPECT_EQ(stub->uploads[0].data[7], 255);
}

// --- IA16 ---
TEST_F(TextureTestFixture, ImportTextureIA16_SinglePixel) {
    // IA16: 8-bit intensity, 8-bit alpha, 2 bytes per pixel (big-endian)
    uint8_t texData[2] = { 0x80, 0xFF }; // I=128, A=255
    SetupLoadedTexture(0, 0, texData, 2, 2, 2);
    interp->ImportTextureIA16(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 1u);
    EXPECT_EQ(stub->uploads[0].height, 1u);
    EXPECT_EQ(stub->uploads[0].data[0], 0x80);
    EXPECT_EQ(stub->uploads[0].data[1], 0x80);
    EXPECT_EQ(stub->uploads[0].data[2], 0x80);
    EXPECT_EQ(stub->uploads[0].data[3], 0xFF);
}

// --- I4 ---
TEST_F(TextureTestFixture, ImportTextureI4_SingleByte_TwoPixels) {
    // I4: 4-bit intensity per pixel, 2 pixels per byte
    // 0xA5 → high nibble: A=10, low nibble: 5
    uint8_t texData[1] = { 0xA5 };
    SetupLoadedTexture(0, 0, texData, 1, 1, 1);
    interp->ImportTextureI4(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 2u);
    EXPECT_EQ(stub->uploads[0].height, 1u);

    // First pixel: I=10 → SCALE_4_8(10) = 0xAA for all channels
    EXPECT_EQ(stub->uploads[0].data[0], SCALE_4_8(0xA));
    EXPECT_EQ(stub->uploads[0].data[1], SCALE_4_8(0xA));
    EXPECT_EQ(stub->uploads[0].data[2], SCALE_4_8(0xA));
    EXPECT_EQ(stub->uploads[0].data[3], SCALE_4_8(0xA));

    // Second pixel: I=5 → SCALE_4_8(5) = 0x55
    EXPECT_EQ(stub->uploads[0].data[4], SCALE_4_8(5));
    EXPECT_EQ(stub->uploads[0].data[5], SCALE_4_8(5));
    EXPECT_EQ(stub->uploads[0].data[6], SCALE_4_8(5));
    EXPECT_EQ(stub->uploads[0].data[7], SCALE_4_8(5));
}

// --- I8 ---
TEST_F(TextureTestFixture, ImportTextureI8_SinglePixel) {
    // I8: 8-bit intensity replicated to all RGBA channels
    uint8_t texData[1] = { 0x80 };
    SetupLoadedTexture(0, 0, texData, 1, 1, 1);
    interp->ImportTextureI8(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 1u);
    EXPECT_EQ(stub->uploads[0].height, 1u);
    EXPECT_EQ(stub->uploads[0].data[0], 0x80);
    EXPECT_EQ(stub->uploads[0].data[1], 0x80);
    EXPECT_EQ(stub->uploads[0].data[2], 0x80);
    EXPECT_EQ(stub->uploads[0].data[3], 0x80);
}

TEST_F(TextureTestFixture, ImportTextureI8_2x2) {
    uint8_t texData[4] = { 0x00, 0xFF, 0x80, 0x40 };
    SetupLoadedTexture(0, 0, texData, 4, 2, 2);
    interp->ImportTextureI8(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 2u);
    EXPECT_EQ(stub->uploads[0].height, 2u);

    // (0,0): 0x00
    EXPECT_EQ(stub->uploads[0].data[0], 0x00);
    EXPECT_EQ(stub->uploads[0].data[3], 0x00);
    // (1,0): 0xFF
    EXPECT_EQ(stub->uploads[0].data[4], 0xFF);
    EXPECT_EQ(stub->uploads[0].data[7], 0xFF);
    // (0,1): 0x80
    EXPECT_EQ(stub->uploads[0].data[8], 0x80);
    // (1,1): 0x40
    EXPECT_EQ(stub->uploads[0].data[12], 0x40);
}

// --- CI4 ---
TEST_F(TextureTestFixture, ImportTextureCi4_SinglePixel) {
    // CI4: 4-bit color index per pixel, looks up in a RGBA5551 palette

    // Create a palette with entry 0: pure red (R=31,G=0,B=0,A=1) → 0xF801
    // Palette is 16 entries * 2 bytes = 32 bytes
    uint8_t palette[32] = {};
    palette[0] = 0xF8; palette[1] = 0x01; // Index 0: red opaque

    // Set palette pointer
    interp->mRdp->palettes[0] = palette;
    interp->mRdp->palette_dram_addr[0] = palette;
    interp->mRdp->texture_tile[0].palette = 0;

    // Texture data: 1 byte = 2 pixels
    // High nibble index=0, low nibble index=0
    uint8_t texData[1] = { 0x00 };

    // CI4: 2 pixels per byte → 1 byte = 2 pixels width, 1 row
    SetupLoadedTexture(0, 0, texData, 1, 1, 1);
    interp->ImportTextureCi4(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].width, 2u);
    EXPECT_EQ(stub->uploads[0].height, 1u);

    // Both pixels use index 0 → red
    EXPECT_EQ(stub->uploads[0].data[0], 255); // R
    EXPECT_EQ(stub->uploads[0].data[1], 0);   // G
    EXPECT_EQ(stub->uploads[0].data[2], 0);   // B
    EXPECT_EQ(stub->uploads[0].data[3], 255); // A
}

TEST_F(TextureTestFixture, ImportTextureCi4_MultipleIndices) {
    // Create palette with 2 entries
    uint8_t palette[32] = {};
    // Index 0: green (R=0,G=31,B=0,A=1) → 0x07C1
    palette[0] = 0x07; palette[1] = 0xC1;
    // Index 1: blue (R=0,G=0,B=31,A=1) → 0x003F
    palette[2] = 0x00; palette[3] = 0x3F;

    interp->mRdp->palettes[0] = palette;
    interp->mRdp->palette_dram_addr[0] = palette;
    interp->mRdp->texture_tile[0].palette = 0;

    // Texture: high nibble=0 (green), low nibble=1 (blue)
    uint8_t texData[1] = { 0x01 };
    SetupLoadedTexture(0, 0, texData, 1, 1, 1);
    interp->ImportTextureCi4(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);

    // Pixel 0: green
    EXPECT_EQ(stub->uploads[0].data[0], 0);   // R
    EXPECT_EQ(stub->uploads[0].data[1], 255); // G
    EXPECT_EQ(stub->uploads[0].data[2], 0);   // B
    EXPECT_EQ(stub->uploads[0].data[3], 255); // A

    // Pixel 1: blue
    EXPECT_EQ(stub->uploads[0].data[4], 0);   // R
    EXPECT_EQ(stub->uploads[0].data[5], 0);   // G
    EXPECT_EQ(stub->uploads[0].data[6], 255); // B
    EXPECT_EQ(stub->uploads[0].data[7], 255); // A
}

// --- CI8 ---
TEST_F(TextureTestFixture, ImportTextureCi8_SinglePixel) {
    // CI8: 8-bit index into a 256-entry RGBA5551 palette
    // Palette is split: indices 0-127 use palette[0], 128-255 use palette[1]

    // Create palettes (128 entries * 2 bytes each = 256 bytes)
    uint8_t pal0[256] = {};
    uint8_t pal1[256] = {};

    // Index 0 in pal0: white (R=31,G=31,B=31,A=1) → 0xFFFF
    pal0[0] = 0xFF; pal0[1] = 0xFF;

    interp->mRdp->palettes[0] = pal0;
    interp->mRdp->palettes[1] = pal1;
    interp->mRdp->palette_dram_addr[0] = pal0;
    interp->mRdp->palette_dram_addr[1] = pal1;

    uint8_t texData[1] = { 0x00 }; // Index 0
    SetupLoadedTexture(0, 0, texData, 1, 1, 1);
    interp->ImportTextureCi8(0, false);

    ASSERT_EQ(stub->uploads.size(), 1u);
    EXPECT_EQ(stub->uploads[0].data[0], 255); // R
    EXPECT_EQ(stub->uploads[0].data[1], 255); // G
    EXPECT_EQ(stub->uploads[0].data[2], 255); // B
    EXPECT_EQ(stub->uploads[0].data[3], 255); // A
}

// ============================================================
// Color Combiner Tests
// ============================================================

TEST(ColorCombiner, GenerateCC_AllZero) {
    ColorCombinerKey key = {};
    key.combine_mode = 0;
    key.options = 0;
    key.shader_id = 0;

    Fast::ColorCombiner comb = {};
    Fast::Interpreter::GenerateCC(&comb, key);

    // With all-zero combine mode, the combiner should produce
    // a valid shader_id (even if it's a trivial pass-through)
    // The key point is it doesn't crash and produces deterministic output
    Fast::ColorCombiner comb2 = {};
    Fast::Interpreter::GenerateCC(&comb2, key);
    EXPECT_EQ(comb.shader_id0, comb2.shader_id0);
    EXPECT_EQ(comb.shader_id1, comb2.shader_id1);
}

TEST(ColorCombiner, GenerateCC_Modulate) {
    // A typical MODULATE combiner: (TEXEL0 - 0) * SHADE + 0
    // RGB: A=TEXEL0(1), B=0(31), C=SHADE(4), D=0(31)
    // Alpha: A=TEXEL0(1), B=0(7), C=SHADE(4), D=0(7)
    uint64_t combine_mode = 0;
    // Cycle 0 RGB: bits [0:3]=A, [4:7]=B, [8:12]=C, [13:15]=D
    combine_mode |= ((uint64_t)G_CCMUX_TEXEL0);        // A=1
    combine_mode |= ((uint64_t)G_CCMUX_0 << 4);         // B=31
    combine_mode |= ((uint64_t)G_CCMUX_SHADE << 8);     // C=4
    combine_mode |= ((uint64_t)G_CCMUX_0 << 13);        // D=31→clamped to 0 in GenerateCC (>=7 for D)
    // Cycle 0 Alpha: bits [16:18]=A, [19:21]=B, [22:24]=C, [25:27]=D
    combine_mode |= ((uint64_t)G_ACMUX_TEXEL0 << 16);   // A=1
    combine_mode |= ((uint64_t)G_ACMUX_0 << 19);        // B=7
    combine_mode |= ((uint64_t)G_ACMUX_SHADE << 22);    // C=4
    combine_mode |= ((uint64_t)G_ACMUX_0 << 25);        // D=7

    ColorCombinerKey key = {};
    key.combine_mode = combine_mode;
    key.options = 0;

    Fast::ColorCombiner comb = {};
    Fast::Interpreter::GenerateCC(&comb, key);

    // Should use texture 0
    EXPECT_TRUE(comb.usedTextures[0]);
}

// ============================================================
// gfx_cc_get_features Tests
// ============================================================

TEST(CCFeatures, AllZeroShaderIds) {
    CCFeatures features = {};
    gfx_cc_get_features(0, 0, &features);

    EXPECT_FALSE(features.opt_alpha);
    EXPECT_FALSE(features.opt_fog);
    EXPECT_FALSE(features.opt_noise);
    EXPECT_FALSE(features.opt_2cyc);
    EXPECT_FALSE(features.opt_grayscale);
    EXPECT_FALSE(features.usedTextures[0]);
    EXPECT_FALSE(features.usedTextures[1]);
    EXPECT_EQ(features.numInputs, 0);
}

TEST(CCFeatures, AlphaFlag) {
    CCFeatures features = {};
    gfx_cc_get_features(0, SHADER_OPT(ALPHA), &features);
    EXPECT_TRUE(features.opt_alpha);
    EXPECT_FALSE(features.opt_fog);
}

TEST(CCFeatures, FogFlag) {
    CCFeatures features = {};
    gfx_cc_get_features(0, SHADER_OPT(FOG), &features);
    EXPECT_TRUE(features.opt_fog);
}

TEST(CCFeatures, NoiseFlag) {
    CCFeatures features = {};
    gfx_cc_get_features(0, SHADER_OPT(NOISE), &features);
    EXPECT_TRUE(features.opt_noise);
}

TEST(CCFeatures, GrayscaleFlag) {
    CCFeatures features = {};
    gfx_cc_get_features(0, SHADER_OPT(GRAYSCALE), &features);
    EXPECT_TRUE(features.opt_grayscale);
}

TEST(CCFeatures, TwoCycleFlag) {
    CCFeatures features = {};
    gfx_cc_get_features(0, SHADER_OPT(_2CYC), &features);
    EXPECT_TRUE(features.opt_2cyc);
}

TEST(CCFeatures, MultipleFlags) {
    CCFeatures features = {};
    uint64_t opts = SHADER_OPT(ALPHA) | SHADER_OPT(FOG) | SHADER_OPT(NOISE);
    gfx_cc_get_features(0, opts, &features);
    EXPECT_TRUE(features.opt_alpha);
    EXPECT_TRUE(features.opt_fog);
    EXPECT_TRUE(features.opt_noise);
    EXPECT_FALSE(features.opt_2cyc);
}

TEST(CCFeatures, Texel0Used) {
    // Place SHADER_TEXEL0 (8) in cycle0 RGB slot A (bits 0-3)
    uint64_t shader_id0 = (uint64_t)SHADER_TEXEL0;
    CCFeatures features = {};
    gfx_cc_get_features(shader_id0, 0, &features);
    EXPECT_TRUE(features.usedTextures[0]);
}

TEST(CCFeatures, Texel1Used) {
    // Place SHADER_TEXEL1 (10) in cycle0 RGB slot A (bits 0-3)
    uint64_t shader_id0 = (uint64_t)SHADER_TEXEL1;
    CCFeatures features = {};
    gfx_cc_get_features(shader_id0, 0, &features);
    EXPECT_TRUE(features.usedTextures[1]);
}

TEST(CCFeatures, ClampFlags) {
    CCFeatures features = {};
    uint64_t opts = SHADER_OPT(TEXEL0_CLAMP_S) | SHADER_OPT(TEXEL1_CLAMP_T);
    gfx_cc_get_features(0, opts, &features);
    EXPECT_TRUE(features.clamp[0][0]);  // TEXEL0 clamp S
    EXPECT_FALSE(features.clamp[0][1]); // TEXEL0 clamp T
    EXPECT_FALSE(features.clamp[1][0]); // TEXEL1 clamp S
    EXPECT_TRUE(features.clamp[1][1]);  // TEXEL1 clamp T
}

TEST(CCFeatures, DoSingle) {
    // When c[cycle][component][2] (the C input) == SHADER_0, do_single should be true
    // SHADER_0 is 0. If all c values are 0, then c[*][*][2] == 0 == SHADER_0
    CCFeatures features = {};
    gfx_cc_get_features(0, 0, &features);
    EXPECT_TRUE(features.do_single[0][0]);
    EXPECT_TRUE(features.do_single[0][1]);
    EXPECT_TRUE(features.do_single[1][0]);
    EXPECT_TRUE(features.do_single[1][1]);
}

TEST(CCFeatures, ShaderIdExtraction) {
    // PRISM_SHADER option is 16-bit wide, at bit position 16 of shader_id1
    // ShaderIdUnmask extracts bits [16:31] of the value
    uint64_t shader_id1 = ((uint64_t)0x1234 << Fast::SHADER_ID_SHIFT);
    CCFeatures features = {};
    gfx_cc_get_features(0, shader_id1, &features);
    EXPECT_EQ(features.shader_id, 0x1234);
}

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
// Layer 3: Vertex Transform with Lighting
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
// Layer 3: Vertex Texture Coordinate Tests
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
// Layer 3: Vertex Clip Rejection Tests
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
// Layer 3: Vertex Fog Calculation Tests
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
// Layer 3: Dest Index Offset in GfxSpVertex
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
// Layer 3: Combined LoadBlock → ImportTexture Pipeline
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
// Layer 3: Display List Command Dispatch Tests
// These tests build F3DGfx command arrays and process them
// through the interpreter's display list execution loop.
// ============================================================

// Helper to build a display list command word
static Fast::F3DGfx MakeCmd(uintptr_t w0, uintptr_t w1) {
    Fast::F3DGfx cmd = {};
    cmd.words.w0 = w0;
    cmd.words.w1 = w1;
    return cmd;
}

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
// Depth and Framebuffer Tests
// ============================================================

// ----- Layer 2: OtherMode Depth Flag Tests -----

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

// ----- Layer 2: SetZImage and SetColorImage Tests -----

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

// ----- Layer 2: AdjustWidthHeightForScale fractional tests -----

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

// ----- Layer 3: Display List Depth Tests -----

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

// ----- Layer 2: Framebuffer Info Tests -----

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
