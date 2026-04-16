// Fast3D Test Common — shared fixtures and stubs for all Fast3D test layers.
#pragma once

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
// StubRenderingAPI: captures UploadTexture calls and basic state.
// Used by Layer 1 (texture tests), Layer 2 (scissor/viewport),
// and Layer 3 (lighting, display list dispatch).
// ============================================================

namespace fast3d_test {

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
    Fast::GfxClipParameters GetClipParameters() override { return { false, false }; }
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

// ============================================================
// TextureTestFixture: sets up an Interpreter with StubRenderingAPI
// and a usable mTexUploadBuffer for texture import tests.
// ============================================================

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
        interp->mRapi = nullptr;
        delete stub;
    }

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

        interp->mRdp->texture_tile[tile].line_size_bytes = tileLineSizeBytes ? tileLineSizeBytes : lineSizeBytes;

        interp->mRdp->texture_tile[tile].uls = 0;
        interp->mRdp->texture_tile[tile].ult = 0;
        interp->mRdp->texture_tile[tile].lrs = 4 * 1024 - 4;
        interp->mRdp->texture_tile[tile].lrt = 4 * 1024 - 4;
    }

    std::unique_ptr<Fast::Interpreter> interp;
    StubRenderingAPI* stub;
};

// ============================================================
// MakeCmd helper for building F3DGfx display list commands
// ============================================================

inline Fast::F3DGfx MakeCmd(uintptr_t w0, uintptr_t w1) {
    Fast::F3DGfx cmd = {};
    cmd.words.w0 = w0;
    cmd.words.w1 = w1;
    return cmd;
}

} // namespace fast3d_test
