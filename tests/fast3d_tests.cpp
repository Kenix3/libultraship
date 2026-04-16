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
    void SetDepthTestAndMask(bool, bool) override {}
    void SetZmodeDecal(bool) override {}
    void SetViewport(int, int, int, int) override {}
    void SetScissor(int, int, int, int) override {}
    void SetUseAlpha(bool) override {}
    void DrawTriangles(float[], size_t, size_t) override {}
    void Init() override {}
    void OnResize() override {}
    void StartFrame() override {}
    void EndFrame() override {}
    void FinishRender() override {}
    int CreateFramebuffer() override { return 0; }
    void UpdateFramebufferParameters(int, uint32_t, uint32_t, uint32_t, bool, bool, bool, bool) override {}
    void StartDrawToFramebuffer(int, float) override {}
    void CopyFramebuffer(int, int, int, int, int, int, int, int, int, int) override {}
    void ClearFramebuffer(bool, bool) override {}
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

        // Set native dimensions to avoid division by zero in aspect ratio
        interp->mNativeDimensions.width = SCREEN_WIDTH;
        interp->mNativeDimensions.height = SCREEN_HEIGHT;
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
