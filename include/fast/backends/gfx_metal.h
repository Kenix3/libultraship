//
//  gfx_metal.h
//  libultraship
//
//  Created by David Chavez on 16.08.22.
//
#pragma once
#ifdef __APPLE__

#include <memory>
#include "gfx_rendering_api.h"
#include "../interpreter.h"

#include <imgui_impl_sdl3.h>
#include <simd/simd.h>

namespace Ship {
class ConsoleVariable;
class ResourceManager;
} // namespace Ship

/** @brief Number of rotating vertex buffers kept to reduce CPU/GPU sync stalls. */
static constexpr size_t kMaxVertexBufferPoolSize = 3;
/** @brief Maximum multisample count supported by this Metal backend implementation. */
static constexpr size_t METAL_MAX_MULTISAMPLE_SAMPLE_COUNT = 8;
/** @brief Maximum number of pixel-depth coordinates queried in a single pass. */
static constexpr size_t MAX_PIXEL_DEPTH_COORDS = 1024;

namespace MTL {
class Texture;
class SamplerState;
class CommandBuffer;
class RenderPassDescriptor;
class RenderCommandEncoder;
class SamplerState;
class ScissorRect;
class Device;
class Function;
class Buffer;
class RenderPipelineState;
class ComputePipelineState;
class CommandQueue;
class Viewport;
} // namespace MTL

namespace CA {
class MetalDrawable;
class MetalLayer;
} // namespace CA

namespace NS {
class AutoreleasePool;
}

/** @brief Combines two integer keys using the Cantor pairing function. */
static size_t cantor(uint64_t a, uint64_t b) {
    return (a + b) * (a + b + 1) / 2 + b;
}

/** @brief Hash functor for `(shaderId, msaaLevel)` pairs used by shader caches. */
struct hash_pair_shader_ids {
    size_t operator()(const std::pair<uint64_t, uint64_t>& p) const {
        auto value1 = p.first;
        auto value2 = p.second;
        return cantor(value1, value2);
    }
};

namespace Fast {

/**
 * @brief Cached Metal shader program and pipeline variants for one combiner key.
 */
struct ShaderProgramMetal {
    uint64_t shader_id0;
    uint64_t shader_id1;

    uint8_t numInputs;
    uint8_t numFloats;
    bool usedTextures[SHADER_MAX_TEXTURES];
    bool markedForDeletion = false;

    // hashed by msaa_level
    MTL::RenderPipelineState* pipeline_state_variants[9];
};

/**
 * @brief Metal texture object metadata tracked by the renderer cache.
 */
struct TextureDataMetal {
    MTL::Texture* texture;
    MTL::Texture* msaaTexture;
    MTL::SamplerState* sampler;
    uint32_t width;
    uint32_t height;
    uint32_t filtering;
    // Total mip levels uploaded (0/1 = base level only)
    uint32_t mip_levels;
    bool linear_filtering;
};

/**
 * @brief Framebuffer state for an offscreen or onscreen Metal render target.
 */
struct FramebufferMetal {
    MTL::CommandBuffer* mCommandBuffer;
    MTL::RenderPassDescriptor* mRenderPassDescriptor;
    MTL::RenderCommandEncoder* mCommandEncoder;

    MTL::Texture* mDepthTexture;
    MTL::Texture* mMsaaDepthTexture;
    uint32_t mTextureId;
    bool mHasDepthBuffer;
    uint32_t mMsaaLevel;
    bool mRenderTarget;

    // State
    bool mHasEndedEncoding;
    bool mHasBoundVertexShader;
    bool mHasBoundFragShader;

    struct ShaderProgramMetal* mLastShaderProgram;
    MTL::Texture* mLastBoundTextures[SHADER_MAX_TEXTURES];
    MTL::SamplerState* mLastBoundSamplers[SHADER_MAX_TEXTURES];
    MTL::ScissorRect* mScissorRect;
    MTL::Viewport* mViewport;

    int8_t mLastDepthTest = -1;
    int8_t mLastDepthMask = -1;
    int8_t mLastZmodeDecal = -1;
    int8_t mLastStrictDecal = -1;

    // When true, command buffer is created on the readback queue (no enqueue ordering).
    // First ReadFramebufferToCPU returns zeros and flips this; real data from frame 2+.
    bool mUseReadbackQueue = false;
};

/** @brief Per-frame uniforms consumed by Metal shader programs. */
struct FrameUniforms {
    simd::int1 frameCount;
    simd::float1 noiseScale;
};

/** @brief Per-draw uniforms consumed by Metal shader programs. */
struct DrawUniforms {
    simd::int1 textureFiltering[SHADER_MAX_TEXTURES];
    simd::float1 prim_depth;
    simd::float1 lod_max;
};

/** @brief Buffer payload used for batched pixel-depth coordinate queries. */
struct CoordUniforms {
    simd::uint2 coords[MAX_PIXEL_DEPTH_COORDS];
};

/// Metal implementation of the Fast3D rendering API.
class GfxRenderingAPIMetal final : public GfxRenderingAPI {
  public:
    /** @brief Constructs the Metal renderer with optional shared dependencies. */
    GfxRenderingAPIMetal(std::shared_ptr<Ship::ConsoleVariable> consoleVariable = nullptr,
                         std::shared_ptr<Ship::ResourceManager> resourceManager = nullptr);
    ~GfxRenderingAPIMetal() override = default;

    /** @name GfxRenderingAPI implementation */
    /** @{ */
    const char* GetName() override;
    int GetMaxTextureSize() override;
    GfxClipParameters GetClipParameters() override;
    void UnloadShader(ShaderProgram* oldPrg) override;
    void LoadShader(ShaderProgram* newPrg) override;
    ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) override;
    ShaderProgram* LookupShader(uint64_t shaderId0, uint64_t shaderId1) override;
    void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;
    void ClearShaderCache() override;
    uint32_t NewTexture() override;
    void SelectTexture(int tile, uint32_t textureId) override;
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) override;
    void UploadTextureMip(const uint8_t* rgba32Buf, uint32_t width, uint32_t height, uint32_t level,
                          uint32_t totalLevels) override;
    void SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) override;
    void SetDepthTestAndMask(bool depth_test, bool z_upd) override;
    void SetCurrentPrimDepth(float depth) override;
    void SetCurrentMaxLod(float maxLod) override;
    void SetZmodeDecal(bool decal) override;
    void SetStrictDecal(bool on) override;
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
    void UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                     bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                     bool can_extract_depth) override;
    void StartDrawToFramebuffer(int fbId, float noiseScale) override;
    void CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0,
                         int dstX1, int dstY1) override;
    void ClearFramebuffer(bool color, bool depth) override;
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) override;
    void ResolveMSAAColorBuffer(int fbIdTarge, int fbIdSrc) override;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) override;
    void* GetFramebufferTextureId(int fbId) override;
    void SelectTextureFb(int fbId) override;
    void DeleteTexture(uint32_t texId) override;
    void SetTextureFilter(FilteringMode mode) override;
    FilteringMode GetTextureFilter() override;
    void SetSrgbMode() override;
    ImTextureID GetTextureById(int id) override;
    /** @} */

    /** @brief Begins a new ImGui frame for the Metal renderer bridge. */
    void NewFrame();
    /** @brief Prepares floating-frame state for rendering ImGui draw data. */
    void SetupFloatingFrame();
    /** @brief Renders ImGui draw data using the active Metal command encoder. */
    void RenderDrawData(ImDrawData* drawData);
    /** @brief Initializes Metal/SDL bridge objects for this renderer instance. */
    bool MetalInit(SDL_Renderer* renderer);

  private:
    bool NonUniformThreadGroupSupported();
    void SetupScreenFramebuffer(uint32_t width, uint32_t height);
    // Elements that only need to be setup once
    SDL_Renderer* mRenderer;
    CA::MetalLayer* mLayer; // CA::MetalLayer*
    MTL::Device* mDevice;
    MTL::CommandQueue* mCommandQueue;
    MTL::CommandQueue* mReadbackQueue;

    int mCurrentVertexBufferPoolIndex = 0;
    MTL::Buffer* mVertexBufferPool[kMaxVertexBufferPoolSize];
    std::unordered_map<std::pair<uint64_t, uint64_t>, struct ShaderProgramMetal, hash_pair_shader_ids>
        mShaderProgramPool;

    std::vector<struct TextureDataMetal> mTextures;
    std::vector<FramebufferMetal> mFramebuffers;
    FrameUniforms mFrameUniforms;
    CoordUniforms mCoordUniforms;
    DrawUniforms mDrawUniforms;
    MTL::Buffer* mFrameUniformBuffer;

    uint32_t mMsaaNumQualityLevels[METAL_MAX_MULTISAMPLE_SAMPLE_COUNT];

    // Depth querying
    MTL::Buffer* mCoordUniformBuffer;
    MTL::Buffer* mDepthValueOutputBuffer;
    size_t mCoordBufferSize;
    MTL::Function* mDepthComputeFunction;
    MTL::Function* mConvertToRgb5a1Function;
    MTL::ComputePipelineState* mConvertToRgb5a1PipelineState = nullptr;
    std::shared_ptr<Ship::ConsoleVariable> mConsoleVariable;
    std::shared_ptr<Ship::ResourceManager> mResourceManager;

    // Screen FB deferred readback: blit in EndFrame, CPU conversion next frame.
    // mScreenReadbackCmdBuf retains the command buffer that encoded the blit so
    // we can waitUntilCompleted before reading the shared buffer contents.
    MTL::CommandBuffer* mScreenReadbackCmdBuf = nullptr;
    MTL::Buffer* mScreenReadbackBuffer = nullptr;
    uint32_t mScreenReadbackWidth = 0;
    uint32_t mScreenReadbackHeight = 0;
    bool mScreenReadbackDataReady = false;
    bool mScreenReadbackRequested = false;

    // Current state
    struct ShaderProgramMetal* mShaderProgram;
    CA::MetalDrawable* mCurrentDrawable;
    std::set<int> mDrawnFramebuffers;
    NS::AutoreleasePool* mFrameAutoreleasePool;

    int mCurrentTile;
    uint32_t mCurrentTextureIds[SHADER_MAX_TEXTURES];

    int32_t mRenderTargetHeight;
    int mCurrentFramebuffer;
    size_t mCurrentVertexBufferOffset;
    FilteringMode mCurrentFilterMode = FILTER_THREE_POINT;
    bool mLodMaxDirty = true;

    bool mNonUniformThreadgroupSupported;
};

} // namespace Fast

/** @brief Returns true when Metal is available on the current runtime platform. */
bool Metal_IsSupported();

#endif
