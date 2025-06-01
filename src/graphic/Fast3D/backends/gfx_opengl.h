#ifdef ENABLE_OPENGL
#pragma once

#include "gfx_rendering_api.h"
#include "../interpreter.h"

#ifdef _MSC_VER
#include <SDL2/SDL.h>
// #define GL_GLEXT_PROTOTYPES 1
#include <GL/glew.h>
#elif FOR_WINDOWS
#include <GL/glew.h>
#include "SDL.h"
#define GL_GLEXT_PROTOTYPES 1
#include "SDL_opengl.h"
#elif __APPLE__
#include <SDL2/SDL.h>
#include <GL/glew.h>
#elif USE_OPENGLES
#include <SDL2/SDL.h>
#include <GLES3/gl3.h>
#else
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengl.h>
#endif
namespace Fast {
struct ShaderProgram {
    GLuint openglProgramId;
    uint8_t numInputs;
    bool usedTextures[SHADER_MAX_TEXTURES];
    uint8_t numFloats;
    GLint attribLocations[16];
    uint8_t attribSizes[16];
    uint8_t numAttribs;
    GLint frameCountLocation;
    GLint noiseScaleLocation;
    GLint texture_width_location;
    GLint texture_height_location;
    GLint texture_filtering_location;
};

struct FramebufferOGL {
    uint32_t width, height;
    bool has_depth_buffer;
    uint32_t msaa_level;
    bool invertY;

    GLuint fbo, clrbuf, clrbufMsaa, rbo;
};

class GfxRenderingAPIOGL final : public GfxRenderingAPI {
  public:
    ~GfxRenderingAPIOGL() override = default;
    const char* GetName() override;
    int GetMaxTextureSize() override;
    GfxClipParameters GetClipParameters() override;
    void UnloadShader(ShaderProgram* oldPrg) override;
    void LoadShader(ShaderProgram* newPrg) override;
    ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint32_t shaderId1) override;
    ShaderProgram* LookupShader(uint64_t shaderId0, uint32_t shaderId1) override;
    void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;
    uint32_t NewTexture() override;
    void SelectTexture(int tile, uint32_t textureId) override;
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) override;
    void SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) override;
    void SetDepthTestAndMask(bool depth_test, bool z_upd) override;
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
    void UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                     bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                     bool can_extract_depth) override;
    void StartDrawToFramebuffer(int fbId, float noiseScale) override;
    void CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0,
                         int dstX1, int dstY1) override;
    void ClearFramebuffer(bool color, bool depth) override;
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) override;
    void ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) override;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) override;
    void* GetFramebufferTextureId(int fbId) override;
    void SelectTextureFb(int fbId) override;
    void DeleteTexture(uint32_t texId) override;
    void SetTextureFilter(FilteringMode mode) override;
    FilteringMode GetTextureFilter() override;
    void SetSrgbMode() override;
    ImTextureID GetTextureById(int id) override;

  private:
    void SetUniforms(ShaderProgram* prg) const;
    std::string BuildFsShader(const CCFeatures& cc_features);
    void SetPerDrawUniforms();

    struct TextureInfo {
        uint16_t width;
        uint16_t height;
        uint16_t filtering;
    } textures[1024];

    GLuint mCurrentTextureIds[2];
    uint8_t mCurrentTile;

    std::map<std::pair<uint64_t, uint32_t>, ShaderProgram> mShaderProgramPool;
    ShaderProgram* mCurrentShaderProgram;

    GLuint mOpenglVbo = 0;
#if defined(__APPLE__) || defined(USE_OPENGLES)
    GLuint mOpenglVao;
#endif

    uint32_t mFrameCount = 0;

    std::vector<FramebufferOGL> mFrameBuffers;
    size_t mCurrentFrameBuffer = 0;
    float mCurrentNoiseScale = 0.0f;
    FilteringMode mCurrentFilterMode = FILTER_THREE_POINT;

    GLint mMaxMsaaLevel = 1;
    GLuint mPixelDepthRb = 0;
    GLuint mPixelDepthFb = 0;
    size_t mPixelDepthRbSize = 0;
};

} // namespace Fast
#endif
