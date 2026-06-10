#pragma once

#include <stdint.h>
#include <cstring>

#include <unordered_map>
#include <set>
#include "imconfig.h"

namespace Fast {
struct ShaderProgram;

struct GfxClipParameters {
    bool z_is_from_0_to_1;
    bool invertY;
};

enum FilteringMode { FILTER_THREE_POINT, FILTER_LINEAR, FILTER_NONE };

// Per-draw color-combiner constants, latched at flush time. The combiner
// formula runs on the GPU; these are the RDP register operands it reads.
// inputs[] is filled according to the combiner's shader_input_mapping.
struct CombinerUniforms {
    float inputs[6][4];
    float fog_color[4];       // rgb = fog (or blend) color; a unused
    float grayscale_color[4]; // rgb = target color, a = lerp factor
};

constexpr int GFX_MAX_GPU_LIGHTS = 32;

// Per-draw lighting/texgen state, consumed by the vertex shader when the
// LIGHTING/TEXGEN shader options are active. Layout is mirrored in the shader
// templates; keep the two in sync.
struct LightingUniforms {
    // Per light: [0] = rgb color (0..1) + w = 1.0 for point lights, 0.0 directional;
    //            [1] = model-space direction coefficients + w = kc (linear atten);
    //            [2] = world-space position + w = kq (quadratic atten)
    float lights[GFX_MAX_GPU_LIGHTS][3][4];
    float ambient[4];     // rgb ambient light (0..1)
    float lookat_x[4];    // model-space lookat coefficients for texgen
    float lookat_y[4];
    float texgen[2][4];   // per texture: (scaleS, offsetS, scaleT, offsetT)
    float mv_rows[3][4];  // top modelview rows for point lighting transpose-multiply
    int32_t num_lights;   // excluding ambient
    int32_t padding[3];
};

// A hash function used to hash a: pair<float, float>
struct hash_pair_ff {
    size_t operator()(const std::pair<float, float>& p) const {
        const auto hash1 = std::hash<float>{}(p.first);
        const auto hash2 = std::hash<float>{}(p.second);

        // If hash1 == hash2, their XOR is zero.
        return (hash1 != hash2) ? hash1 ^ hash2 : hash1;
    }
};

class GfxRenderingAPI {
  public:
    virtual ~GfxRenderingAPI() = default;
    virtual const char* GetName() = 0;
    virtual int GetMaxTextureSize() = 0;
    virtual GfxClipParameters GetClipParameters() = 0;
    virtual void UnloadShader(ShaderProgram* oldPrg) = 0;
    virtual void LoadShader(ShaderProgram* newPrg) = 0;
    virtual void ClearShaderCache() = 0;
    virtual ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) = 0;
    virtual ShaderProgram* LookupShader(uint64_t shaderId0, uint64_t shaderId1) = 0;
    virtual void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) = 0;
    virtual uint32_t NewTexture() = 0;
    virtual void SelectTexture(int tile, uint32_t textureId) = 0;
    virtual void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) = 0;
    // Upload one level of a mipmapped texture to the currently selected texture.
    // Level 0 must be uploaded first, with totalLevels indicating the full chain size.
    // Backends without mipmap support fall back to uploading only the base level.
    virtual void UploadTextureMip(const uint8_t* rgba32Buf, uint32_t width, uint32_t height, uint32_t level,
                                  uint32_t totalLevels) {
        if (level == 0) {
            UploadTexture(rgba32Buf, width, height);
        }
    }
    virtual void SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) = 0;
    virtual void SetDepthTestAndMask(bool depth_test, bool z_upd) = 0;
    virtual void SetZmodeDecal(bool decal) = 0;
    virtual void SetStrictDecal(bool on) = 0;
    virtual void SetViewport(int x, int y, int width, int height) = 0;
    virtual void SetScissor(int x, int y, int width, int height) = 0;
    virtual void SetUseAlpha(bool useAlpha) = 0;
    virtual void DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) = 0;
    virtual void Init() = 0;
    virtual void OnResize() = 0;
    virtual void StartFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void FinishRender() = 0;
    virtual int CreateFramebuffer() = 0;
    virtual void UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                             bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                             bool can_extract_depth) = 0;
    virtual void StartDrawToFramebuffer(int fbId, float noiseScale) = 0;
    virtual void CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0,
                                 int dstY0, int dstX1, int dstY1) = 0;
    virtual void ClearFramebuffer(bool color, bool depth) = 0;
    virtual void ClearDepthRegion(int x, int y, int w, int h) {
        // Default: full depth clear. Backends that support scissored depth clears
        // (e.g. OpenGL) should override for a more precise partial clear.
        ClearFramebuffer(false, true);
    }
    virtual void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) = 0;
    virtual void ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) = 0;
    virtual std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) = 0;
    virtual void* GetFramebufferTextureId(int fbId) = 0;
    virtual void SelectTextureFb(int fbId) = 0;
    virtual void DeleteTexture(uint32_t texId) = 0;
    virtual void SetTextureFilter(FilteringMode mode) = 0;
    virtual FilteringMode GetTextureFilter() = 0;
    virtual void SetSrgbMode() = 0;
    virtual ImTextureID GetTextureById(int id) = 0;
    virtual void SetCurrentPrimDepth(float depth) = 0;
    // Highest mip/LOD level (as float) usable by the current draw's LOD computation.
    // 0 means only the base level exists.
    virtual void SetCurrentMaxLod(float maxLod) {
        mCurrentMaxLod = maxLod;
    }
    // Combiner constants for the next DrawTriangles call.
    virtual void SetCombinerUniforms(const CombinerUniforms& uniforms) {
        if (memcmp(&uniforms, &mCombinerUniforms, sizeof(CombinerUniforms)) != 0) {
            mCombinerUniforms = uniforms;
            mCombinerUniformsDirty = true;
        }
    }
    // Lighting/texgen state for the next DrawTriangles call (vertex shader).
    virtual void SetLightingUniforms(const LightingUniforms& uniforms) {
        if (memcmp(&uniforms, &mLightingUniforms, sizeof(LightingUniforms)) != 0) {
            mLightingUniforms = uniforms;
            mLightingUniformsDirty = true;
        }
    }

  protected:
    int8_t mCurrentDepthTest = 0;
    int8_t mCurrentDepthMask = 0;
    int8_t mCurrentZmodeDecal = 0;
    int8_t mCurrentStrictDecal = 0;
    int8_t mLastDepthTest = -1;
    int8_t mLastDepthMask = -1;
    int8_t mLastZmodeDecal = -1;
    int8_t mLastStrictDecal = -1;
    bool mSrgbMode = false;
    float mCurrentPrimDepth = 0.0f;
    bool mPrimDepthDirty = true;
    float mCurrentMaxLod = 0.0f;
    CombinerUniforms mCombinerUniforms = {};
    bool mCombinerUniformsDirty = true;
    LightingUniforms mLightingUniforms = {};
    bool mLightingUniformsDirty = true;
};
} // namespace Fast
