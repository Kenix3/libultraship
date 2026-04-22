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
    bool    hasAlpha     = false;

    struct AttribInfo {
        uint8_t  offset;   // float offset within a vertex
        uint8_t  size;     // number of floats
        uint32_t location; // shader binding location
    };
    std::vector<AttribInfo> attribs;
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
// auto-selects the best available renderer (D3D11 → Metal → OpenGL) at init.
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
    // Shader generation helpers (GLSL via prism, same templates as OpenGL backend)
    std::string BuildFragShader(const CCFeatures& cc) const;
    std::string BuildVertShader(const CCFeatures& cc,
                                size_t& outNumFloats,
                                std::vector<LLGLShaderProgram::AttribInfo>& outAttribs) const;

    // Build the LLGL graphics PSO for the given shader program.
    bool BuildPipelineState(LLGLShaderProgram* prog,
                            const std::string& vsGlsl,
                            const std::string& fsGlsl);

    // Look up (or create) a sampler for the given filter/wrap params.
    LLGL::Sampler* GetOrCreateSampler(bool linearFilter,
                                      uint32_t cms, uint32_t cmt);

    // Ensure the streaming vertex buffer is at least bytesNeeded bytes.
    void EnsureVertexBufferSize(size_t bytesNeeded);

    // LLGL core objects — stored as raw pointers so that this header can
    // be compiled without any LLGL #includes (PIMPL-lite pattern).
    // The destructor is defined in gfx_llgl.cpp where LLGL headers are fully included.
    // mRendererLifetime is a unique_ptr<void, void(*)(void*)> that owns the RenderSystem.
    std::unique_ptr<void, void(*)(void*)> mRendererLifetime{ nullptr, nullptr };
    LLGL::RenderSystem*  mRenderer   = nullptr;
    LLGL::CommandQueue*  mCmdQueue   = nullptr;
    LLGL::CommandBuffer* mCmdBuf     = nullptr;
    LLGL::Fence*         mFence      = nullptr;
    LLGL::Buffer*        mVertexBuffer      = nullptr;
    size_t               mVertexBufferBytes = 0;

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
    FilteringMode mFilterMode   = FILTER_THREE_POINT;
    std::string   mModuleName;

    static const char* const kPreferredModules[];
};

} // namespace Fast
#endif // ENABLE_LLGL
