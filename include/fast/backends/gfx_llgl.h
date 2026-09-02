#ifdef ENABLE_LLGL
#pragma once

#include "gfx_rendering_api.h"
#include "../interpreter.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

// Forward-declare LLGL types so that this header does not pull in OpenGL /
// platform headers (which define GL_* macros that conflict with prism's lexer
// token identifiers).  The full LLGL headers are included only inside
// gfx_llgl.cpp, after any prism / interpreter translation units.
namespace LLGL {
    class RenderSystem;
    class CommandBuffer;
    class CommandQueue;
    class Fence;
    class Buffer;
    class Texture;
    class Sampler;
    class PipelineState;
    class PipelineLayout;
    class ResourceHeap;
    class RenderTarget;
    class RenderSystemDeleter;
    template<class T, class Deleter> class unique_ptr_base; // unused – see below
} // namespace LLGL

namespace Fast {

// ---------------------------------------------------------------------------
// Describes one resource slot in a shader program's pipeline layout.
// The type tells DrawTriangles what to bind from the renderer's state.
// ---------------------------------------------------------------------------
enum class LLGLBindingType : uint8_t {
    FrameCount,  // shared frame-count constant buffer
    NoiseScale,  // shared noise-scale constant buffer
    Texture,     // mCurrentTexIds[slot]  (slot = 0 or 1)
    Sampler,     // mCurrentTexIds[slot].sampler
    MaskTex,     // used_masks texture
    MaskSampler, // used_masks sampler
    BlendTex,    // used_blend texture
    BlendSampler,// used_blend sampler
};

struct LLGLBindingSlot {
    LLGLBindingType type;
    uint8_t         index;  // texture/sampler slot index (0 or 1)
};

// ---------------------------------------------------------------------------
// Internal shader-program record for the LLGL backend.
// Named LLGLShaderProgram to avoid conflicting with the OpenGL ShaderProgram
// definition in gfx_opengl.h.  Returned as an opaque Fast::ShaderProgram*
// pointer to the interpreter via reinterpret_cast.
// ---------------------------------------------------------------------------
struct LLGLShaderProgram {
    LLGL::PipelineState*  pso            = nullptr;
    LLGL::PipelineLayout* pipelineLayout = nullptr;
    LLGL::ResourceHeap*   resourceHeap   = nullptr;

    uint8_t numFloats    = 4;
    uint8_t numInputs    = 1;
    bool    usedTextures[SHADER_MAX_TEXTURES] = {};
    bool    usedMasks[SHADER_MAX_TEXTURES]    = {};
    bool    usedBlend[SHADER_MAX_TEXTURES]    = {};
    bool    hasAlpha     = false;

    // Resource heap binding order (matches the pipeline layout binding list).
    std::vector<LLGLBindingSlot> heapSlots;

    // Persistent storage for HLSL semantic names (set by SpirvToLLGL).
    // Kept here so they outlive the VertexFormat/ShaderDescriptor used in
    // CreateShader().
    std::vector<std::string> attrSemanticNames;
};

// Per-texture slot metadata
struct LLGLTextureInfo {
    LLGL::Texture* texture  = nullptr;
    LLGL::Sampler* sampler  = nullptr;
    uint16_t       width    = 0;
    uint16_t       height   = 0;
};

// Per-framebuffer record
struct LLGLFramebuffer {
    LLGL::Texture*      colorTex     = nullptr;
    LLGL::Texture*      depthTex     = nullptr;
    LLGL::RenderTarget* renderTarget = nullptr;
    uint32_t            width        = 0;
    uint32_t            height       = 0;
    bool                hasDepth     = false;
    bool                invertY      = false;
};

// ---------------------------------------------------------------------------
// GfxRenderingAPILLGL – implements the full GfxRenderingAPI interface using
// LLGL (Low Level Graphics Library by Lukas Hermanns).  The backend
// auto-selects the best available renderer:
//   Windows : Direct3D12 → Direct3D11 → OpenGL
//   macOS   : Metal → OpenGL
//   Other   : OpenGL
// Shaders are written once as Vulkan GLSL (#version 450) and translated at
// runtime via glslang (GLSL→SPIR-V) + SPIRV-Cross (SPIR-V→target language).
// ---------------------------------------------------------------------------
class GfxRenderingAPILLGL final : public GfxRenderingAPI {
  public:
    ~GfxRenderingAPILLGL() override;

    const char*        GetName()          override;
    int                GetMaxTextureSize() override;
    GfxClipParameters GetClipParameters() override;

    void UnloadShader(ShaderProgram* oldPrg)   override;
    void LoadShader(ShaderProgram* newPrg)      override;
    void ClearShaderCache()                     override;
    ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) override;
    ShaderProgram* LookupShader(uint64_t shaderId0, uint64_t shaderId1)           override;
    void ShaderGetInfo(ShaderProgram* prg,
                       uint8_t* numInputs, bool usedTextures[2])                  override;

    uint32_t NewTexture()                                                      override;
    void     SelectTexture(int tile, uint32_t textureId)                       override;
    void     UploadTexture(const uint8_t* rgba32Buf, uint32_t w, uint32_t h)  override;
    void     SetSamplerParameters(int sampler, bool linearFilter,
                                  uint32_t cms, uint32_t cmt)                 override;
    void     DeleteTexture(uint32_t texId)                                     override;

    void SetDepthTestAndMask(bool depthTest, bool zUpd) override;
    void SetZmodeDecal(bool decal)                       override;
    void SetViewport(int x, int y, int w, int h)         override;
    void SetScissor(int x, int y, int w, int h)          override;
    void SetUseAlpha(bool useAlpha)                      override;

    void DrawTriangles(float buf_vbo[], size_t buf_vbo_len,
                       size_t buf_vbo_num_tris)                                override;

    void Init()         override;
    void OnResize()     override;
    void StartFrame()   override;
    void EndFrame()     override;
    void FinishRender() override;

    int  CreateFramebuffer() override;
    void UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height,
                                     uint32_t msaa_level, bool opengl_invertY,
                                     bool render_target, bool has_depth_buffer,
                                     bool can_extract_depth) override;
    void StartDrawToFramebuffer(int fbId, float noiseScale) override;
    void CopyFramebuffer(int fbDstId, int fbSrcId,
                         int srcX0, int srcY0, int srcX1, int srcY1,
                         int dstX0, int dstY0, int dstX1, int dstY1)  override;
    void ClearFramebuffer(bool color, bool depth)                      override;
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height,
                               uint16_t* rgba16Buf)                    override;
    void ResolveMSAAColorBuffer(int fbIdTarget, int fbIdSrc)           override;

    std::unordered_map<std::pair<float,float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id,
                  const std::set<std::pair<float,float>>& coordinates) override;

    void*       GetFramebufferTextureId(int fbId) override;
    void        SelectTextureFb(int fbId)         override;
    void        SetTextureFilter(FilteringMode mode) override;
    FilteringMode GetTextureFilter()              override;
    void        SetSrgbMode()                     override;
    ImTextureID GetTextureById(int id)            override;

    bool IsAvailable() const { return mRenderer != nullptr; }

  private:
    // --- Shader generation --------------------------------------------------
    // Build Vulkan GLSL (#version 450) source strings from the prism template.
    // The callbacks simultaneously populate vtxFmt and layoutDesc so that the
    // pipeline state can be created immediately afterwards.
    std::string BuildVertGlsl(const CCFeatures& cc,
                               void* vtxFmtPtr) const;
    std::string BuildFragGlsl(const CCFeatures& cc,
                               void* layoutDescPtr,
                               std::vector<LLGLBindingSlot>& outSlots) const;

    // Compile the vertex + fragment GLSL sources to SPIR-V, cross-compile to
    // the renderer's shading language, and create the PSO.
    bool BuildPipelineState(LLGLShaderProgram* prog,
                             const std::string& vsGlsl,
                             const std::string& fsGlsl,
                             void* vtxFmtPtr,
                             void* layoutDescPtr);

    // --- Sampler cache -------------------------------------------------------
    LLGL::Sampler* GetOrCreateSampler(bool linearFilter,
                                      uint32_t cms, uint32_t cmt);

    // --- Vertex buffer -------------------------------------------------------
    void EnsureVertexBufferSize(size_t bytesNeeded);

    // --- Resource heap -------------------------------------------------------
    // Rebuild (or reuse) the resource heap for the current draw.
    void UpdateResourceHeap(LLGLShaderProgram* prog);

    // --- Constant buffers (shared, created in Init()) -----------------------
    void EnsureConstantBuffers();

    // =========================================================================
    // Member state
    // =========================================================================

    // LLGL core objects — stored as raw pointers so that this header can
    // be compiled without any LLGL #includes (PIMPL-lite pattern).
    std::unique_ptr<void, void(*)(void*)> mRendererLifetime{ nullptr, nullptr };
    LLGL::RenderSystem*  mRenderer   = nullptr;
    LLGL::CommandQueue*  mCmdQueue   = nullptr;
    LLGL::CommandBuffer* mCmdBuf     = nullptr;
    LLGL::Fence*         mFence      = nullptr;
    LLGL::Buffer*        mVertexBuffer      = nullptr;
    size_t               mVertexBufferBytes = 0;

    // Shared constant buffers for frame_count and noise_scale.
    LLGL::Buffer* mFrameCountBuf  = nullptr;
    LLGL::Buffer* mNoiseScaleBuf  = nullptr;

    std::vector<LLGLFramebuffer>   mFBs;
    int                            mCurrentFB = 0;
    std::vector<LLGLTextureInfo>   mTextures;
    uint32_t  mCurrentTexIds[SHADER_MAX_TEXTURES] = {};
    int       mCurrentTile = 0;

    std::map<std::pair<uint64_t,uint64_t>, LLGLShaderProgram> mShaderCache;
    LLGLShaderProgram* mCurrentProg = nullptr;

    struct SamplerKey {
        bool linear; uint32_t cms, cmt;
        bool operator<(const SamplerKey& o) const {
            if (linear != o.linear) return linear < o.linear;
            if (cms    != o.cms)    return cms    < o.cms;
            return cmt < o.cmt;
        }
    };
    std::map<SamplerKey, LLGL::Sampler*> mSamplerCache;

    bool     mPendingDepthTest = false;
    bool     mPendingDepthMask = false;
    bool     mPendingDecal     = false;
    bool     mPendingAlpha     = true;
    int      mScissorX = 0, mScissorY = 0, mScissorW = 1, mScissorH = 1;
    int      mViewportX = 0, mViewportY = 0, mViewportW = 1, mViewportH = 1;
    float    mCurrentNoiseScale = 1.0f;
    uint32_t mFrameCount        = 0;
    bool     mInRenderPass      = false;
    bool     mSrgbMode          = false;
    FilteringMode mFilterMode   = FILTER_THREE_POINT;
    std::string   mModuleName;

    // Backend preference order – tried in sequence until one loads.
    static const char* const kPreferredModules[];
};

} // namespace Fast
#endif // ENABLE_LLGL

