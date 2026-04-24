#ifdef ENABLE_VULKAN_PRDP
#pragma once

#include "gfx_rendering_api.h"
#include "../interpreter.h"

#include <cstdint>
#include <memory>
#include <vector>

// Forward-declare ParallelRDP / Granite types so this header doesn't pull in
// Vulkan headers (which would conflict with volk used by parallel-rdp).
namespace RDP {
class CommandProcessor;
} // namespace RDP
namespace Vulkan {
class Context;
class Device;
} // namespace Vulkan

namespace Fast {

// ---------------------------------------------------------------------------
// GfxRenderingAPIVulkan
//
// Implements the GfxRenderingAPI interface by forwarding raw N64 RDP commands
// to ParallelRDP (https://github.com/Themaister/parallel-rdp-standalone).
//
// Unlike the OpenGL / LLGL backends which rasterize triangles through GPU
// shaders, this backend delegates all rasterization to ParallelRDP's
// hardware-accurate RDP emulation running on Vulkan compute shaders.
//
// The backend also implements RdpCommandBackend so it can be attached to an
// Interpreter via SetRdpCommandBackend() to receive RDP commands emitted
// during display-list processing.
//
// Lifecycle:
//   1. Init()        – creates Vulkan device + ParallelRDP CommandProcessor
//   2. StartFrame()  – begins a new ParallelRDP frame context
//   3. (interpreter processes display list, emitting RDP commands)
//   4. EndFrame()    – signals timeline and waits for completion
//   5. ReadFramebufferToCPU() – copies rendered pixels from RDRAM
//
// Most GfxRenderingAPI methods (texture upload, shader management, draw
// triangles) are intentional no-ops because ParallelRDP handles everything
// at the RDP command level.
// ---------------------------------------------------------------------------
class GfxRenderingAPIVulkan final : public GfxRenderingAPI, public RdpCommandBackend {
  public:
    GfxRenderingAPIVulkan();
    ~GfxRenderingAPIVulkan() override;

    // ---- RdpCommandBackend --------------------------------------------------
    void SubmitCommand(size_t numWords, const uint32_t* words) override;

    // ---- GfxRenderingAPI ----------------------------------------------------
    const char* GetName() override;
    int GetMaxTextureSize() override;
    GfxClipParameters GetClipParameters() override;

    void UnloadShader(ShaderProgram* oldPrg) override;
    void LoadShader(ShaderProgram* newPrg) override;
    void ClearShaderCache() override;
    ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) override;
    ShaderProgram* LookupShader(uint64_t shaderId0, uint64_t shaderId1) override;
    void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;

    uint32_t NewTexture() override;
    void SelectTexture(int tile, uint32_t textureId) override;
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) override;
    void SetSamplerParameters(int sampler, bool linearFilter, uint32_t cms, uint32_t cmt) override;
    void DeleteTexture(uint32_t texId) override;

    void SetDepthTestAndMask(bool depthTest, bool zUpd) override;
    void SetZmodeDecal(bool decal) override;
    void SetViewport(int x, int y, int width, int height) override;
    void SetScissor(int x, int y, int width, int height) override;
    void SetUseAlpha(bool useAlpha) override;

    void DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) override;

    void Init() override;
    void OnResize() override;
    void StartFrame() override;
    void EndFrame() override;
    void FinishRender() override;

    int CreateFramebuffer() override;
    void UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height,
                                     uint32_t msaa_level, bool opengl_invertY,
                                     bool render_target, bool has_depth_buffer,
                                     bool can_extract_depth) override;
    void StartDrawToFramebuffer(int fbId, float noiseScale) override;
    void CopyFramebuffer(int fbDstId, int fbSrcId,
                         int srcX0, int srcY0, int srcX1, int srcY1,
                         int dstX0, int dstY0, int dstX1, int dstY1) override;
    void ClearFramebuffer(bool color, bool depth) override;
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height,
                              uint16_t* rgba16Buf) override;
    void ResolveMSAAColorBuffer(int fbIdTarget, int fbIdSrc) override;

    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id,
                  const std::set<std::pair<float, float>>& coordinates) override;

    void* GetFramebufferTextureId(int fbId) override;
    void SelectTextureFb(int fbId) override;
    void SetTextureFilter(FilteringMode mode) override;
    FilteringMode GetTextureFilter() override;
    void SetSrgbMode() override;
    ImTextureID GetTextureById(int id) override;

    // ---- Vulkan-specific ----------------------------------------------------
    bool IsAvailable() const { return mAvailable; }

    /// Direct access to the RDRAM backing store.  Game code or the interpreter
    /// can write texture / palette data here before emitting RDP load commands.
    uint8_t* GetRDRAM();
    size_t GetRDRAMSize() const;

    /// Read the RGBA5551 framebuffer from RDRAM at the given byte address.
    /// ParallelRDP writes pixels with adjacent uint16 pairs swapped within
    /// every 32-bit word, so this method compensates with an XOR-1 read.
    std::vector<uint16_t> ReadFramebufferFromRDRAM(uint32_t addr,
                                                    uint32_t width,
                                                    uint32_t height) const;

  private:
    bool mAvailable = false;
    bool mFrameActive = false;

    std::unique_ptr<Vulkan::Context> mContext;
    std::unique_ptr<Vulkan::Device> mDevice;
    std::unique_ptr<RDP::CommandProcessor> mProcessor;
    std::vector<uint8_t> mRdram;

    // Dummy shader program returned by CreateAndLoadNewShader.
    struct DummyShaderProgram {
        uint8_t numInputs = 0;
        bool usedTextures[SHADER_MAX_TEXTURES] = {};
    };
    DummyShaderProgram mDummyShader;

    static constexpr size_t RDRAM_SIZE = 8 * 1024 * 1024;
    static constexpr size_t HIDDEN_RDRAM_SIZE = 4 * 1024 * 1024;
};

} // namespace Fast
#endif // ENABLE_VULKAN_PRDP
