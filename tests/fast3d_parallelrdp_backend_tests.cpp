#include <gtest/gtest.h>

#include "fast/interpreter.h"

namespace {
class CapturingRdpBackend : public Fast::RdpCommandBackend {
  public:
    std::vector<std::vector<uint32_t>> commands;

    void SubmitCommand(size_t numWords, const uint32_t* words) override {
        commands.emplace_back(words, words + numWords);
    }
};

class DummyRenderingAPI : public Fast::GfxRenderingAPI {
  public:
    const char* GetName() override { return "Dummy"; }
    int GetMaxTextureSize() override { return 64; }
    Fast::GfxClipParameters GetClipParameters() override { return { false, false }; }
    void UnloadShader(Fast::ShaderProgram*) override {}
    void LoadShader(Fast::ShaderProgram*) override {}
    void ClearShaderCache() override {}
    Fast::ShaderProgram* CreateAndLoadNewShader(uint64_t, uint64_t) override { return nullptr; }
    Fast::ShaderProgram* LookupShader(uint64_t, uint64_t) override { return nullptr; }
    void ShaderGetInfo(Fast::ShaderProgram*, uint8_t*, bool[2]) override {}
    uint32_t NewTexture() override { return 0; }
    void SelectTexture(int, uint32_t) override {}
    void UploadTexture(const uint8_t*, uint32_t, uint32_t) override {}
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
    GetPixelDepth(int, const std::set<std::pair<float, float>>& coordinates) override {
        return {};
    }
    void* GetFramebufferTextureId(int) override { return nullptr; }
    void SelectTextureFb(int) override {}
    void DeleteTexture(uint32_t) override {}
    void SetTextureFilter(Fast::FilteringMode) override {}
    Fast::FilteringMode GetTextureFilter() override { return Fast::FILTER_NONE; }
    void SetSrgbMode() override {}
    ImTextureID GetTextureById(int) override { return nullptr; }
};
} // namespace

TEST(Fast3DParallelRdpBackendTest, EmitsSetScissorWords) {
    Fast::Interpreter interp;
    DummyRenderingAPI rapi;
    CapturingRdpBackend backend;
    interp.SetRdpCommandBackend(&backend);
    interp.mRapi = &rapi;
    interp.mNativeDimensions.width = 320.0f;
    interp.mNativeDimensions.height = 240.0f;
    interp.mCurDimensions.width = 320.0f;
    interp.mCurDimensions.height = 240.0f;

    interp.GfxDpSetScissor(1, 0x12, 0x34, 0x56, 0x78);

    ASSERT_EQ(backend.commands.size(), 1u);
    ASSERT_EQ(backend.commands[0].size(), 2u);
    EXPECT_EQ(backend.commands[0][0], (static_cast<uint32_t>(static_cast<uint8_t>(Fast::RDP_G_SETSCISSOR)) << 24) |
                                          (0x12u << 12) | 0x34u);
    EXPECT_EQ(backend.commands[0][1], (1u << 24) | (0x56u << 12) | 0x78u);
}

TEST(Fast3DParallelRdpBackendTest, EmitsSetColorFillAndRectWords) {
    Fast::Interpreter interp;
    DummyRenderingAPI rapi;
    CapturingRdpBackend backend;
    interp.SetRdpCommandBackend(&backend);
    interp.mRapi = &rapi;
    interp.mNativeDimensions.width = 320.0f;
    interp.mNativeDimensions.height = 240.0f;
    interp.mCurDimensions.width = 320.0f;
    interp.mCurDimensions.height = 240.0f;

    interp.GfxDpSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 319, reinterpret_cast<void*>(0x00100000));
    interp.GfxDpSetZImage(reinterpret_cast<void*>(0x00100000));
    interp.GfxDpSetFillColor(0xA55A);
    interp.GfxDpFillRectangle(0x10, 0x20, 0x30, 0x40);

    ASSERT_EQ(backend.commands.size(), 4u);
    ASSERT_EQ(backend.commands[0].size(), 2u);
    ASSERT_EQ(backend.commands[1].size(), 2u);
    ASSERT_EQ(backend.commands[2].size(), 2u);
    ASSERT_EQ(backend.commands[3].size(), 2u);

    EXPECT_EQ(backend.commands[0][0], (static_cast<uint32_t>(static_cast<uint8_t>(Fast::RDP_G_SETCIMG)) << 24) |
                                          (G_IM_FMT_RGBA << 21) | (G_IM_SIZ_16b << 19) | 319u);
    EXPECT_EQ(backend.commands[0][1], 0x00100000u);

    EXPECT_EQ(backend.commands[1][0], static_cast<uint32_t>(static_cast<uint8_t>(Fast::RDP_G_SETZIMG)) << 24);
    EXPECT_EQ(backend.commands[1][1], 0x00100000u);

    EXPECT_EQ(backend.commands[2][0], static_cast<uint32_t>(static_cast<uint8_t>(Fast::RDP_G_SETFILLCOLOR)) << 24);
    EXPECT_EQ(backend.commands[2][1], 0xA55Au);

    EXPECT_EQ(backend.commands[3][0], (static_cast<uint32_t>(static_cast<uint8_t>(Fast::RDP_G_FILLRECT)) << 24) |
                                          (0x30u << 12) | 0x40u);
    EXPECT_EQ(backend.commands[3][1], (0x10u << 12) | 0x20u);
}
