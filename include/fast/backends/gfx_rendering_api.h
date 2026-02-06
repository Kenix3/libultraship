#pragma once

#include <stdint.h>

#include <unordered_map>
#include <set>
#include <map>
#include "imconfig.h"

namespace Fast {
struct ShaderProgram;

struct GfxClipParameters {
    bool z_is_from_0_to_1;
    bool invertY;
};

struct ShaderProgramKey {
    uint64_t shader_id0;
    uint32_t shader_id1;
    const char* display_list;

    bool operator==(const ShaderProgramKey&) const noexcept = default;
};

enum FilteringMode { FILTER_THREE_POINT, FILTER_LINEAR, FILTER_NONE };

// A hash function used to hash a: pair<float, float>
struct hash_pair_ff {
    size_t operator()(const std::pair<float, float>& p) const {
        const auto hash1 = std::hash<float>{}(p.first);
        const auto hash2 = std::hash<float>{}(p.second);

        // If hash1 == hash2, their XOR is zero.
        return (hash1 != hash2) ? hash1 ^ hash2 : hash1;
    }
};

template<typename T, size_t N> using Vector = std::array<T, N>;
template<typename T, size_t C, size_t R> using Matrix = std::array<T, C * R>;

using ShaderPrimitiveType = std::variant<
    bool,
    int32_t,
    uint32_t,
    uint64_t,
    float,
    double,

    Vector<float, 2>,    Vector<float, 3>,    Vector<float, 4>,
    Vector<int32_t, 2>,  Vector<int32_t, 3>,  Vector<int32_t, 4>,
    Vector<uint32_t, 2>, Vector<uint32_t, 3>, Vector<uint32_t, 4>,
    Vector<bool, 2>,     Vector<bool, 3>,     Vector<bool, 4>,

    Matrix<float, 2, 2>, Matrix<float, 3, 3>, Matrix<float, 4, 4>,
    Matrix<float, 3, 2>, Matrix<float, 4, 3>
>;

struct ShaderPrimitive {
    std::string Key;
    ShaderPrimitiveType Value;
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
    virtual ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint32_t shaderId1, const char* display_list) = 0;
    virtual ShaderProgram* LookupShader(uint64_t shaderId0, uint32_t shaderId1, const char* display_list) = 0;
    virtual void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) = 0;
    virtual uint32_t NewTexture() = 0;
    virtual void SelectTexture(int tile, uint32_t textureId) = 0;
    virtual void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) = 0;
    void SetShaderUniforms(const std::vector<ShaderPrimitive>& uniforms) {
        mShaderUniforms = uniforms;
    }
    virtual void BindShaderUniforms() = 0;
    virtual void SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) = 0;
    virtual void SetDepthTestAndMask(bool depth_test, bool z_upd) = 0;
    virtual void SetZmodeDecal(bool decal) = 0;
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

  protected:
    int8_t mCurrentDepthTest = 0;
    int8_t mCurrentDepthMask = 0;
    int8_t mCurrentZmodeDecal = 0;
    int8_t mLastDepthTest = -1;
    int8_t mLastDepthMask = -1;
    int8_t mLastZmodeDecal = -1;
    bool mSrgbMode = false;

    std::vector<ShaderPrimitive> mShaderUniforms;
};
} // namespace Fast

namespace std {
    template <>
    struct hash<Fast::ShaderProgramKey> {
        size_t operator()(const Fast::ShaderProgramKey& k) const noexcept {
            const size_t m = 0x9e3779b97f4a7c15; 
            size_t seed = 0;
            seed ^= k.shader_id0 + m + (seed << 6) + (seed >> 2);
            seed ^= k.shader_id1 + m + (seed << 6) + (seed >> 2);
            seed ^= k.display_list != nullptr ? std::hash<const char*>{}(k.display_list) : 0 + m + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}