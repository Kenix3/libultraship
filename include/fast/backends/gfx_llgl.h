#ifdef ENABLE_LLGL
#pragma once

#include "gfx_rendering_api.h"
#include "../interpreter.h"

#include <LLGL/LLGL.h>
#include <LLGL/Backend/OpenGL/NativeHandle.h>

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace Fast {

// Internal shader-program record for the LLGL backend.
// Named LLGLShaderProgram to avoid conflicting with the OpenGL ShaderProgram
// definition in gfx_opengl.h.
struct LLGLShaderProgram {
    LLGL::PipelineState*    pso             = nullptr;
    LLGL::PipelineLayout*   pipelineLayout  = nullptr;

    // Resource heap is per draw call (updated per frame)
    LLGL::ResourceHeap*     resourceHeap    = nullptr;

    // VBO stride info (floats per vertex) needed to set up BufferDescriptor
    uint8_t  numFloats      = 4;
    uint8_t  numInputs      = 1;
    bool     usedTextures[SHADER_MAX_TEXTURES] = {};
    bool     hasAlpha       = false;

    // Vertex attribute layout — mirrors the same info as the OpenGL ShaderProgram
    struct AttribInfo {
        uint8_t  offset;   // float offset within a vertex
        uint8_t  size;     // number of floats
        uint32_t location; // shader binding location
    };
    std::vector<AttribInfo> attribs;
};

// Per-texture metadata
struct LLGLTextureInfo {
    LLGL::Texture* texture  = nullptr;
    LLGL::Sampler* sampler  = nullptr;
    uint16_t       width    = 0;
    uint16_t       height   = 0;
    uint16_t       filtering = 0; // 0=none,1=linear,2=nearest
};

// Per-framebuffer record
struct LLGLFramebuffer {
    LLGL::Texture*      colorTex        = nullptr;
    LLGL::Texture*      depthTex        = nullptr;
    LLGL::RenderTarget* renderTarget    = nullptr;
    uint32_t            width           = 0;
    uint32_t            height          = 0;
    bool                hasDepth        = false;
    bool                invertY         = false;
};

class GfxRenderingAPILLGL final : public GfxRenderingAPI {
  public:
    ~GfxRenderingAPILLGL() override;

    const char*         GetName()               override;
    int                 GetMaxTextureSize()      override;
    GfxClipParameters  GetClipParameters()      override;

    void UnloadShader(ShaderProgram* oldPrg)        override;
    void LoadShader(ShaderProgram* newPrg)           override;
    void ClearShaderCache()                          override;
    ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) override;
    ShaderProgram* LookupShader(uint64_t shaderId0, uint64_t shaderId1)           override;
    void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;

    uint32_t  NewTexture()                                                    override;
    void      SelectTexture(int tile, uint32_t textureId)                     override;
    void      UploadTexture(const uint8_t* rgba32Buf, uint32_t w, uint32_t h) override;
    void      SetSamplerParameters(int sampler, bool linearFilter,
                                   uint32_t cms, uint32_t cmt)               override;
    void      DeleteTexture(uint32_t texId)                                   override;

    void SetDepthTestAndMask(bool depthTest, bool zUpd) override;
    void SetZmodeDecal(bool decal)                      override;
    void SetViewport(int x, int y, int w, int h)        override;
    void SetScissor(int x, int y, int w, int h)         override;
    void SetUseAlpha(bool useAlpha)                     override;

    void DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) override;

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
    void StartDrawToFramebuffer(int fbId, float noiseScale)                         override;
    void CopyFramebuffer(int fbDstId, int fbSrcId,
                         int srcX0, int srcY0, int srcX1, int srcY1,
                         int dstX0, int dstY0, int dstX1, int dstY1)               override;
    void ClearFramebuffer(bool color, bool depth)                                   override;
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height,
                               uint16_t* rgba16Buf)                                 override;
    void ResolveMSAAColorBuffer(int fbIdTarget, int fbIdSrc)                        override;

    std::unordered_map<std::pair<float,float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id, const std::set<std::pair<float,float>>& coords)       override;

    void* GetFramebufferTextureId(int fbId) override;
    void  SelectTextureFb(int fbId)         override;

    void           SetTextureFilter(FilteringMode mode) override;
    FilteringMode  GetTextureFilter()                   override;
    void           SetSrgbMode()                        override;
    ImTextureID    GetTextureById(int id)               override;

    // Returns true if the LLGL renderer was successfully initialised.
    bool IsAvailable() const { return mRenderer != nullptr; }

  private:
    // Shader helpers
    std::string BuildFragShader(const CCFeatures& cc) const;
    std::string BuildVertShader(const CCFeatures& cc, size_t& outNumFloats,
                                std::vector<LLGLShaderProgram::AttribInfo>& outAttribs) const;

    // Build / rebuild the PSO for a given shader program.
    bool BuildPipelineState(LLGLShaderProgram* prog,
                            const std::string& vsGlsl,
                            const std::string& fsGlsl);

    // Look up or create the sampler for given wrap/filter params.
    LLGL::Sampler* GetOrCreateSampler(bool linearFilter, uint32_t cms, uint32_t cmt);

    // Ensure the vertex buffer is large enough; (re)create if needed.
    void EnsureVertexBufferSize(size_t bytesNeeded);

    // Flush state changes before a draw call.
    void FlushRenderState();

    LLGL::RenderSystemPtr mRenderer;
    LLGL::CommandQueue*   mCmdQueue   = nullptr;
    LLGL::CommandBuffer*  mCmdBuf     = nullptr;
    LLGL::Fence*          mFence      = nullptr;

    // Vertex streaming buffer
    LLGL::Buffer*  mVertexBuffer      = nullptr;
    size_t         mVertexBufferBytes = 0;

    // Framebuffers  (index 0 = default back-buffer)
    std::vector<LLGLFramebuffer> mFBs;
    int mCurrentFB = 0;

    // Textures
    std::vector<LLGLTextureInfo> mTextures; // indexed by texture ID
    uint32_t mCurrentTexIds[SHADER_MAX_TEXTURES] = {};
    int      mCurrentTile = 0;

    // Shader / pipeline cache: keyed by (shader_id0, shader_id1)
    std::map<std::pair<uint64_t,uint64_t>, LLGLShaderProgram> mShaderCache;
    LLGLShaderProgram* mCurrentProg = nullptr;

    // Sampler cache
    struct SamplerKey {
        bool     linear;
        uint32_t cms, cmt;
        bool operator<(const SamplerKey& o) const {
            if (linear != o.linear) return linear < o.linear;
            if (cms    != o.cms)    return cms    < o.cms;
            return cmt < o.cmt;
        }
    };
    std::map<SamplerKey, LLGL::Sampler*> mSamplerCache;

    // Per-draw state that is deferred until FlushRenderState()
    bool     mPendingDepthTest      = false;
    bool     mPendingDepthMask      = false;
    bool     mPendingDecal          = false;
    bool     mPendingAlpha          = true;
    int      mScissorX = 0, mScissorY = 0, mScissorW = 1, mScissorH = 1;
    int      mViewportX = 0, mViewportY = 0, mViewportW = 1, mViewportH = 1;

    // Noise scale / frame count forwarded to the fragment shader via push constant or UBO
    float    mCurrentNoiseScale = 1.0f;
    uint32_t mFrameCount        = 0;

    // Whether we are inside a BeginRenderPass block
    bool     mInRenderPass = false;

    FilteringMode mFilterMode = FILTER_THREE_POINT;

    // Module name used when loading the renderer (e.g. "OpenGL", "Vulkan")
    std::string mModuleName;

    // Preferred backend order (tried in sequence during Init)
    static const char* const kPreferredModules[];
};

} // namespace Fast
#endif // ENABLE_LLGL
