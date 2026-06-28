#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <unordered_map>
#include <map>
#include <list>
#include <cstddef>
#include <vector>
#include <stack>
#include <array>
#include <mutex>
#include <string>
#include <string_view>
#include <memory>
#include <future>
#include <unordered_set>

#include "fast/lus_gbi.h"
#include "fast/types.h"
#include "fast/ucodehandlers.h"
#include "backends/gfx_rendering_api.h"
#include <prism/processor.h>
#include "fast/debug/GfxDebugger.h"

#include "fast/resource/type/Texture.h"
#include "ship/resource/Resource.h"

namespace Ship {
class ResourceManager;
class ConsoleVariable;
} // namespace Ship

// TODO figure out why changing these to 640x480 makes the game only render in a quarter of the window
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <compare>
#endif

/*enum {
    CC_0,
    CC_TEXEL0,
    CC_TEXEL1,
    CC_PRIM,
    CC_SHADE,
    CC_ENV,
    CC_TEXEL0A,
    CC_LOD
};*/

enum {
    SHADER_0,
    SHADER_INPUT_1,
    SHADER_INPUT_2,
    SHADER_INPUT_3,
    SHADER_INPUT_4,
    SHADER_INPUT_5,
    SHADER_INPUT_6,
    SHADER_INPUT_7,
    SHADER_TEXEL0,
    SHADER_TEXEL0A,
    SHADER_TEXEL1,
    SHADER_TEXEL1A,
    SHADER_1,
    SHADER_COMBINED,
    SHADER_NOISE,
    SHADER_LOD_FRAC
};

#ifdef __cplusplus
enum class ShaderOpts {
    ALPHA,
    FOG,
    TEXTURE_EDGE,
    NOISE,
    _2CYC,
    ALPHA_THRESHOLD,
    INVISIBLE,
    GRAYSCALE,
    TEXEL0_CLAMP_S,
    TEXEL0_CLAMP_T,
    TEXEL1_CLAMP_S,
    TEXEL1_CLAMP_T,
    TEXEL0_MASK,
    TEXEL1_MASK,
    TEXEL0_BLEND,
    TEXEL1_BLEND,
    PRIM_DEPTH,
    TEX_LOD,        // G_TL_LOD: LOD_FRACTION comes from per-pixel UV derivatives
    MIP_LOD,        // a real mip pyramid is bound to TEXEL0; TEXEL1 = next mip level
    LIGHTING,       // shade color computed in the vertex shader from normals + lights
    POINT_LIGHTING, // point (positional) lights are present; needs world pos + MV rows
    TEXGEN,         // UVs generated in the vertex shader from normals (env mapping)
    TEXGEN_LINEAR,  // acos-based texgen variant
    TEXEL0_PALETTE, // TEXEL0 is a CI index texture; palette lookup in the shader
    TEXEL1_PALETTE, // TEXEL1 is a CI index texture; palette lookup in the shader
    PRISM_SHADER,   // 16-bit width
    MAX
};

#define SHADER_OPT(opt) ((uint64_t)(1 << static_cast<int>(ShaderOpts::opt)))
#endif

struct ColorCombinerKey {
    uint64_t combine_mode;
    uint64_t options;
    uint64_t shader_id;

#ifdef __cplusplus
    auto operator<=>(const ColorCombinerKey&) const = default;
#endif
};

#define SHADER_MAX_TEXTURES 7
#define SHADER_FIRST_TEXTURE 0
#define SHADER_FIRST_MASK_TEXTURE 2
#define SHADER_FIRST_REPLACEMENT_TEXTURE 4
#define SHADER_PALETTE_TEXTURE 6

struct CCFeatures {
    int c[2][2][4];
    bool opt_alpha;
    bool opt_fog;
    bool opt_texture_edge;
    bool opt_noise;
    bool opt_2cyc;
    bool opt_alpha_threshold;
    bool opt_invisible;
    bool opt_grayscale;
    bool opt_prim_depth;
    bool opt_tex_lod;   // LOD_FRACTION computed from per-pixel UV derivatives
    bool opt_mip_lod;   // TEXEL0 carries a real mip pyramid; TEXEL1 = next mip level
    bool uses_lod_frac; // any combiner slot references SHADER_LOD_FRAC
    bool opt_shade;     // combiner reads the per-vertex shade color (SHADER_INPUT_7)
    bool opt_lighting;  // shade computed in the vertex shader from normals + lights
    bool opt_point_lighting;
    bool opt_texgen;
    bool opt_texgen_linear;
    bool usedTextures[2];
    bool used_palette[2]; // texel is a CI index texture; palette lookup in the shader
    bool used_masks[2];
    bool used_blend[2];
    bool clamp[2][2];
    int numInputs;
    bool do_single[2][2];
    bool do_multiply[2][2];
    bool do_mix[2][2];
    bool color_alpha_same[2];
    int16_t shader_id;
};

void gfx_cc_get_features(uint64_t shader_id0, uint64_t shader_id1, struct CCFeatures* cc_features);

union Gfx;

namespace Fast {

class GfxRenderingAPI;
class GfxWindowBackend;
class Fast3dWindow;

constexpr size_t MAX_SEGMENT_POINTERS = 16;
constexpr size_t SHADER_ID_SHIFT = 25;
// The 16-bit custom shader id is packed into the combiner options immediately
// after the ShaderOpts flags; adding a ShaderOpt past PRISM_SHADER would
// silently overlap the id field.
static_assert(static_cast<size_t>(ShaderOpts::PRISM_SHADER) == SHADER_ID_SHIFT,
              "ShaderOpts grew into the shader-id bitfield (see SHADER_ID_SHIFT)");
constexpr int16_t ShaderIdUnmask(uint64_t id) {
    return (id >> SHADER_ID_SHIFT) & 0xFFFF;
}

struct GfxExecStack {
    // This is a dlist stack used to handle dlist calls.
    std::stack<F3DGfx*> cmd_stack = {};
    // This is also a dlist stack but a std::vector is used to make it possible
    // to iterate on the elements.
    // The purpose of this is to identify an instruction at a point in time
    // which would not be possible with just a F3DGfx* because a dlist can be called multiple times
    // what we do instead is store the call path that leads to the instruction (including branches)
    std::vector<const F3DGfx*> gfx_path = {};
    struct CodeDisp {
        const char* file;
        int line;
    };
    // stack for OpenDisp/CloseDisps
    std::vector<CodeDisp> disp_stack{};

    void start(F3DGfx* dlist);
    void stop();
    F3DGfx*& currCmd();
    void openDisp(const char* file, int line);
    void closeDisp();
    const std::vector<CodeDisp>& getDisp() const;
    void branch(F3DGfx* caller);
    void call(F3DGfx* caller, F3DGfx* callee);
    F3DGfx* ret();
};

struct XYWidthHeight {
    int16_t x, y;
    uint32_t width, height;
};

struct GfxDimensions {
    float internal_mul;
    uint32_t width, height;
    float aspect_ratio;
};

struct TextureCacheKey {
    const uint8_t* texture_addr;
    const uint8_t* palette_addrs[2];
    uint8_t fmt, siz;
    uint8_t palette_index;
    uint32_t size_bytes;
    // Number of mip levels uploaded for this texture (0 or 1 = just the base level).
    // Part of the key so the same data uploaded with/without a mip chain doesn't collide.
    uint8_t mip_levels;
    // 1 = uploaded as a CI index texture (palette applied in the shader). Indexed
    // entries do not key on palette contents, which is what makes TLUT swaps free.
    uint8_t indexed;

    bool operator==(const TextureCacheKey&) const noexcept = default;

    struct Hasher {
        size_t operator()(const TextureCacheKey& key) const noexcept {
            uintptr_t addr = (uintptr_t)key.texture_addr;
            return (size_t)(addr ^ (addr >> 5));
        }
    };
};

typedef std::unordered_map<TextureCacheKey, struct TextureCacheValue, TextureCacheKey::Hasher> TextureCacheMap;
typedef std::pair<const TextureCacheKey, struct TextureCacheValue> TextureCacheNode;

struct TextureCacheValue {
    uint32_t texture_id;
    uint8_t cms, cmt;
    bool linear_filter;

    std::list<struct TextureCacheMapIter>::iterator lru_location;
};

struct TextureCacheMapIter {
    TextureCacheMap::iterator it;
};

struct RGBA {
    uint8_t r, g, b, a;
};

struct LoadedVertex {
    // Object-space position (w normally 1). The vertex shader transforms it with
    // the matrix-palette entry selected by mtx_slot. Rectangle/s2dex paths store
    // final clip coordinates here and reference the identity palette entry.
    float x, y, z, w;
    float u, v;
    struct RGBA color;
    // Raw signed vertex normal (when G_LIGHTING); consumed by the vertex shader
    int8_t normal[3];
    // Index into the interpreter's matrix history captured at vertex-load time
    uint8_t mtx_slot;
};

struct RawTexMetadata {
    uint16_t width, height;
    float h_byte_scale = 1, v_pixel_scale = 1;
    std::shared_ptr<Fast::Texture> resource;
    Fast::TextureType type;
};

#define MAX_LIGHTS 32
#define MAX_VERTICES 64

struct RSP {
    float modelview_matrix_stack[11][4][4];
    uint8_t modelview_matrix_stack_size;

    float MP_matrix[4][4];
    float P_matrix[4][4];

    F3DLight_t lookat[2];
    F3DLight current_lights[MAX_LIGHTS + 1];
    float current_lights_coeffs[MAX_LIGHTS][3];
    float current_lookat_coeffs[2][3]; // lookat_x, lookat_y
    uint8_t current_num_lights;        // includes ambient light
    bool lights_changed;

    uint32_t geometry_mode;
    int16_t fog_mul, fog_offset;

    uint32_t extra_geometry_mode;

    struct {
        // U0.16
        uint16_t s, t;
    } texture_scaling_factor;

    // Max mip level from gsSPTexture (number of mip levels - 1). 0 = no mipmapping.
    uint8_t texture_level;

    struct LoadedVertex loaded_vertices[MAX_VERTICES + 4];
};

struct RDP {
    const uint8_t* palettes[2];
    // Original DRAM source address of the most recent TLUT load per palette half.
    // Used in texture cache keys instead of palettes[] (which always points to staging).
    const uint8_t* palette_dram_addr[2];
    // CI4 palette staging buffer: N64 TMEM holds up to 16 CI4 palettes (16 entries x 2 bytes each = 32 bytes per
    // palette). palettes[0] covers indices 0-7 (256 bytes), palettes[1] covers 8-15 (256 bytes). GfxDpLoadTlut copies
    // TLUT data here at the correct offset so multi-palette CI4 models work.
    uint8_t palette_staging[2][256];
    struct {
        const uint8_t* addr;
        uint8_t siz;
        uint32_t width;
        uint32_t tex_flags;
        struct RawTexMetadata raw_tex_metadata;
    } texture_to_load;
    struct {
        const uint8_t* addr;
        uint32_t orig_size_bytes;
        uint32_t size_bytes;
        uint32_t full_image_line_size_bytes;
        uint32_t line_size_bytes;
        uint32_t tex_flags;
        struct RawTexMetadata raw_tex_metadata;
        bool masked;
        bool blended;
    } loaded_texture[2];
    struct {
        uint8_t fmt;
        uint8_t siz;
        uint8_t cms, cmt;
        uint8_t masks, maskt; // mask exponents from SetTile; 2^mask is the WRAP/MIRROR period
        uint8_t shifts, shiftt;
        float uls, ult, lrs, lrt;
        uint16_t tmem; // 0-511, in 64-bit word units
        uint32_t line_size_bytes;
        uint8_t palette;
        uint8_t tmem_index; // 0 or 1 for offset 0 kB or offset 2 kB, respectively
    } texture_tile[8];
    bool textures_changed[2];
    // Set when a TLUT load changes the palette staging; the GPU palette texture
    // is rebuilt lazily on the next palettized draw.
    bool palette_texture_dirty;

    // Journal of recent TMEM loads so mip-pyramid tiles can be mapped back to their
    // DRAM source even when each level was loaded with its own LoadBlock command.
    // Only linear loads (LoadBlock) are recorded; LoadTile loads are strided in DRAM
    // and are marked non-linear so the mip importer can skip them.
    static constexpr size_t TMEM_JOURNAL_SIZE = 32;
    struct TmemLoadEntry {
        uint16_t tmem_word;       // TMEM start, in 64-bit words
        uint16_t size_words;      // length in 64-bit words
        const uint8_t* dram_addr; // DRAM source of the load
        bool linear;              // true when DRAM data is contiguous (LoadBlock)
    } tmem_loads[TMEM_JOURNAL_SIZE];
    uint8_t tmem_load_head; // next slot to write (circular)

    uint8_t first_tile_index;

    uint32_t other_mode_l, other_mode_h;
    uint64_t combine_mode;
    bool grayscale;

    uint8_t prim_lod_fraction;
    // Minimum LOD clamp from gsDPSetPrimColor's m parameter (sharpen/detail modes)
    uint8_t prim_lod_min;
    uint16_t prim_depth;
    struct RGBA env_color, prim_color, fog_color, blend_color, fill_color, grayscale_color;

    // Chroma key parameters (G_SETKEYR / G_SETKEYGB)
    struct RGBA key_center;
    struct RGBA key_scale;
    int16_t convert_k[6]; // YUV convert coefficients (G_SETCONVERT) — K0-K5

    struct XYWidthHeight viewport, scissor;
    bool viewport_or_scissor_changed;
    void* z_buf_address;
    void* color_image_address;
};

typedef enum Attribute {
    MTX_PROJECTION,
    MTX_LOAD,
    MTX_PUSH,
    MTX_NOPUSH,
    CULL_FRONT,
    CULL_BACK,
    CULL_BOTH,
    MV_VIEWPORT,
    MV_LIGHT,
} Attribute;

extern GfxExecStack g_exec_stack;

struct GfxTextureCache {
    TextureCacheMap map;
    std::list<TextureCacheMapIter> lru;
    std::vector<uint32_t> free_texture_ids;
    std::vector<uint32_t> deferred_free_texture_ids;
};

struct ColorCombiner {
    uint64_t shader_id0;
    uint64_t shader_id1;
    bool usedTextures[2];
    // Combiner reads the per-vertex shade color (mapped to SHADER_INPUT_7)
    bool usedShade;
    struct ShaderProgram* prg[16];
    // Which RDP register feeds each constant input slot ([0] = color, [1] = alpha).
    // Slots are limited to SHADER_INPUT_1..6; consumed by FillCombinerUniforms.
    uint8_t shader_input_mapping[2][7];
};

struct RenderingState {
    uint8_t depth_test_and_mask; // 1: depth test, 2: depth mask
    bool decal_mode;
    bool alpha_blend;
    // GPU backface culling: sign of the kept RSP cross product (0 = no culling)
    int8_t cull_keep_sign;
    struct XYWidthHeight viewport, scissor;
    struct ShaderProgram* mShaderProgram;
    TextureCacheNode* mTextures[SHADER_MAX_TEXTURES];
};

struct FBInfo {
    uint32_t orig_width, orig_height;       // Original shape
    uint32_t applied_width, applied_height; // Up-scaled for the viewport
    uint32_t native_width, native_height;   // Max "native" size of the screen, used for up-scaling
    bool resize;                            // Scale to match the viewport
    bool forceFixedAspect;                  // Preserve aspect ratio even if resize is true
};

struct MaskedTextureEntry {
    uint8_t* mask;
    uint8_t* replacementData;
};

class Interpreter {
  public:
    Interpreter();
    ~Interpreter();

    void Init(GfxWindowBackend* wapi, class GfxRenderingAPI* rapi, const char* game_name, bool start_in_fullscreen,
              uint32_t width, uint32_t height, uint32_t posX, uint32_t posY,
              std::shared_ptr<Ship::ConsoleVariable> consoleVariable = nullptr,
              std::shared_ptr<Ship::ResourceManager> resourceManager = nullptr);
    void Destroy();
    void SetGfxDebugger(std::shared_ptr<GfxDebugger> debugger);
    std::shared_ptr<GfxDebugger> GetGfxDebugger() const;
    void SetFast3dWindow(std::shared_ptr<Fast3dWindow> window);
    static std::shared_ptr<Fast3dWindow> GetCurrentWindow();
    void GetDimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY);
    GfxRenderingAPI* GetCurrentRenderingAPI();
    void StartFrame();
    void RunGuiOnly();
    void Run(Gfx* commands, const std::unordered_map<Mtx*, MtxF>& mtx_replacements,
             const std::unordered_map<Gfx*, Gfx*>& dl_replacements);
    void EndFrame();
    void HandleWindowEvents();
    bool IsFrameReady();
    bool ViewportMatchesRendererResolution();
    int GetTargetFps();
    void SetTargetFps(int fps);
    void SetMaxFrameLatency(int latency);
    int CreateFrameBuffer(uint32_t width, uint32_t height, uint32_t native_width, uint32_t native_height,
                          uint8_t resize, bool forceFixedAspect = false);
    void SetFrameBuffer(int fb, float noiseScale);
    struct PostPass {
        int id;
        std::string path;
        std::string pack;
        bool enabled;
    };
    struct ShaderSettings {
        std::string path;
        std::string pack;
        std::vector<prism::SettingDecl> decls;
        // Current values; scalar types use [0], colors use [0..2]
        std::unordered_map<std::string, std::array<float, 4>> values;
    };

    // ---- Post-processing chain ----
    // Each registered pass is a prism shader template applied as a fullscreen
    // step over the game image at the end of the frame (first pass samples the
    // game framebuffer, later passes sample the previous pass). Returns a
    // handle for UnregisterPostPass. Thread-safe; takes effect next frame.
    int RegisterPostPass(const char* o2rShaderPath);
    void UnregisterPostPass(int id);
    void ClearPostPasses();
    bool HasPostPasses();
    // Snapshot for UI display; enable state edited via SetPostPassEnabled.
    std::vector<PostPass> GetPostPasses();
    void SetPostPassEnabled(int id, bool enabled);
    // Settings registry for UI display (render thread only).
    const std::map<size_t, ShaderSettings>& GetShaderSettingsRegistry() {
        return mShaderSettings;
    }
    // Updates a setting, persists it to CVars and recompiles all shaders.
    // Scalar types pass the value in components[0]; colors use [0..2].
    void SetShaderSettingValue(size_t shaderId, const std::string& var, const float components[4]);
    // Updates the stored value only (used while a slider/picker is being
    // dragged so the widget tracks the drag); commit with SetShaderSettingValue.
    void UpdateShaderSettingValue(size_t shaderId, const std::string& var, const float components[4]);

    // Write one register of the custom uniform file (uCustom in shader
    // templates). Flushes the pending batch when the value changes so the
    // new value only affects subsequent draws. Registers 0-1 are engine
    // built-ins (see CustomUniforms); games use 2..GFX_NUM_CUSTOM_UNIFORMS-1.
    void SetCustomUniform(uint8_t idx, const float values[4]);
    void CopyFrameBuffer(int fb_dst_id, int fb_src_id, bool copyOnce, bool* hasCopiedPtr);
    void ResetFrameBuffer();
    void AdjustPixelDepthCoordinates(float& x, float& y);
    void GetPixelDepthPrepare(float x, float y);
    uint16_t GetPixelDepth(float x, float y);
    void RegisterBlendedTexture(const char* name, uint8_t* mask, uint8_t* replacement);
    void UnregisterBlendedTexture(const char* name);

    // Register a CPU address as a mirror of a GPU framebuffer texture.
    // When ImportTexture encounters this address, it uses SelectTextureFb instead
    // of reading from CPU memory — giving full GPU resolution with no readback.
    void RegisterFbTexture(const void* cpuAddr, int fbId);
    void UnregisterFbTexture(const void* cpuAddr);

    void SetNativeDimensions(float width, float height);
    void SetResolutionMultiplier(float multiplier);
    void SetMsaaLevel(uint32_t level);
    void GetCurDimensions(uint32_t* width, uint32_t* height);

    // private: TODO make these private
    void Flush();
    // Fill the combiner-constant uniforms for the pending batch from RDP state
    // and hand them to the rendering backend.
    void LatchCombinerUniforms();
    // Flush pending triangles when a combiner-visible RDP register is about to change.
    void FlushIfRegisterChanges(const RGBA& reg, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    ShaderProgram* LookupOrCreateShaderProgram(uint64_t id0, uint64_t id1);
    ColorCombiner* LookupOrCreateColorCombiner(const ColorCombinerKey& key);
    void ShaderCacheClear();
    void TextureCacheClear();
    std::shared_ptr<Ship::IResource> ResolveResourceCached(const char* path);
    bool TextureCacheLookup(int i, const TextureCacheKey& key);
    void TextureCacheDelete(const uint8_t* origAddr);
    void TextureCacheDeleteByPalette(const uint8_t* palAddr);
    void ImportTextureRgba16(int tile, bool importReplacement);
    void ImportTextureRgba32(int tile, bool importReplacement);
    void ImportTextureIA4(int tile, bool importReplacement);
    void ImportTextureIA8(int tile, bool importReplacement);
    void ImportTextureIA16(int tile, bool importReplacement);
    void ImportTextureI4(int tile, bool importReplacement);
    void ImportTextureI8(int tile, bool importReplacement);
    void ImportTextureCi4(int tile, bool importReplacement);
    void ImportTextureCi8(int tile, bool importReplacement);
    void ImportTextureRaw(int tile, bool importReplacement);
    void ImportTextureImg(int tile, bool importReplacement);
    void ImportTexture(int i, int tile, bool importReplacement);
    void ImportTextureMask(int i, int tile);
    // Mipmapping support: detect a usable mip pyramid in the tile descriptors,
    // route base-level uploads, and decode/upload the extra levels.
    uint8_t DetectMipChain(uint32_t baseTile) const;
    void UploadBaseTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height);
    void UploadMipChain(uint32_t baseTile);
    // Box-filter downsample one RGBA32 level into the next (halved, min 1px).
    static void BoxDownsampleRgba32(const uint8_t* src, uint32_t srcW, uint32_t srcH, uint8_t* dst);
    const RDP::TmemLoadEntry* FindTmemLoad(uint16_t tmemWord) const;
    void CalculateNormalDir(const F3DLight_t*, float coeffs[3]);

    /**
     * Opt-in memoization of OTR texture-path resolution, keyed by display-list
     * pointer and dropped with the texture cache. Safe for ports whose display
     * lists carry stable path pointers; off by default.
     */
    void SetResolvedResourceCacheEnabled(bool enabled);

    void GfxSpMatrix(uint8_t params, const int32_t* addr);
    void GfxSpPopMatrix(uint32_t count);
    void GfxSpVertex(size_t numVertices, size_t destIndex, const F3DVtx* vertices);
    void GfxSpModifyVertex(uint16_t vtxIdx, uint8_t where, uint32_t val);
    void GfxSpTri1(uint8_t vtx1Idx, uint8_t vtx2Idx, uint8_t vtx3Idx, bool isRect);
    void GfxSpGeometryMode(uint32_t clear, uint32_t set);
    void GfxSpExtraGeometryMode(uint32_t clear, uint32_t set);
    void GfxSpMovememF3dex2(uint8_t index, uint8_t offset, const void* data);
    void GfxSpMovememF3d(uint8_t index, uint8_t offset, const void* data);
    void GfxSpMovewordF3dex2(uint8_t index, uint16_t offset, uintptr_t data);
    void GfxSpMovewordF3d(uint8_t index, uint16_t offset, uintptr_t data);
    void GfxSpTexture(uint16_t sc, uint16_t tc, uint8_t level, uint8_t tile, uint8_t on);
    void GfxDpSetScissor(uint32_t mode, uint32_t ulx, uint32_t uly, uint32_t lrx, uint32_t lry);
    void GfxDpSetTextureImage(uint32_t format, uint32_t size, uint32_t width, const char* texPath, uint32_t texFlags,
                              RawTexMetadata rawTexMetdata, const void* addr);
    void GfxDpSetTile(uint8_t fmt, uint32_t siz, uint32_t line, uint32_t tmem, uint8_t tile, uint32_t palette,
                      uint32_t cmt, uint32_t maskt, uint32_t shiftt, uint32_t cms, uint32_t masks, uint32_t shifts);
    void GfxDpSetTileSize(uint8_t tile, uint16_t uls, uint16_t ult, uint16_t lrs, uint16_t lrt);
    void GfxDpLoadTlut(uint8_t tile, uint32_t high_index);
    void GfxDpLoadBlock(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t dxt);
    void GfxDpLoadTile(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt);
    void GfxDpSetCombineMode(uint32_t rgb, uint32_t alpha, uint32_t rgb_cyc2, uint32_t alpha_cyc2);
    void GfxDpSetGrayscaleColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void GfxDpSetEnvColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void GfxDpSetPrimColor(uint8_t m, uint8_t r, uint8_t l, uint8_t g, uint8_t b, uint8_t a);
    void GfxDpSetFogColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void GfxDpSetBlendColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void GfxDpSetFillColor(uint32_t pickedColor);
    void GfxDrawRectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry);
    void GfxDpTextureRectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, uint8_t tile, int16_t uls,
                               int16_t ult, int16_t dsdx, int16_t dtdy, bool flip);
    void GfxDpImageRectangle(int32_t tile, int32_t w, int32_t h, int32_t ulx, int32_t uly, int16_t uls, int16_t ult,
                             int32_t lrx, int32_t lry, int16_t lrs, int16_t lrt);
    void GfxDpFillRectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry);
    void GfxDpSetZImage(void* zBufAddr);
    void GfxDpSetColorImage(uint32_t format, uint32_t size, uint32_t width, void* address);
    void GfxSpSetOtherMode(uint32_t shift, uint32_t num_bits, uint64_t mode);
    void GfxDpSetOtherMode(uint32_t h, uint32_t l);

    void Gfxs2dexBgCopy(F3DuObjBg* bg);
    void Gfxs2dexBg1cyc(F3DuObjBg* bg);
    void Gfxs2dexRecyCopy(F3DuObjSprite* spr);

    void AdjustWidthHeightForScale(uint32_t& width, uint32_t& height, uint32_t nativeWidth,
                                   uint32_t nativeHeight) const;
    float AdjXForAspectRatio(float x) const;
    void AdjustVIewportOrScissor(XYWidthHeight* area);
    void CalcAndSetViewport(const F3DVp_t* viewport);

    void SpReset();
    void* SegAddr(uintptr_t w1);

    static const char* CCMUXtoStr(uint32_t ccmux);
    static const char* ACMUXtoStr(uint32_t acmux);
    static void GenerateCC(ColorCombiner* comb, const ColorCombinerKey& key);
    static std::string_view GetBaseTexturePath(std::string_view path);
    static void NormalizeVector(float v[3]);
    static void TransposedMatrixMul(float res[3], const float a[3], const float b[4][4]);
    static void MatrixMul(float res[4][4], const float a[4][4], const float b[4][4]);

    RSP* mRsp;
    RDP* mRdp;
    RenderingState mRenderingState{};

    GfxTextureCache mTextureCache{};
    std::unordered_map<const void*, std::shared_ptr<Ship::IResource>> mResolvedResourceCache;
    bool mResolvedResourceCacheEnabled = false;
    // Extra mip levels (beyond the base) for the texture import in progress.
    // Set around ImportTexture calls in GfxSpTri1; 0 everywhere else.
    uint8_t mCurrentMipExtraLevels{};
    std::map<ColorCombinerKey, ColorCombiner> mColorCombinerPool; // color_combiner_pool;
    std::map<ColorCombinerKey, ColorCombiner>::iterator mPrevCombiner = mColorCombinerPool.end();
    // Combiner the currently batched triangles were packed with; its input mapping
    // selects which RDP registers feed the combiner uniforms at flush time.
    ColorCombiner* mPendingCombiner = nullptr;
    // Last lighting/texgen uniforms handed to the backend; a change mid-batch
    // forces a flush so queued triangles keep their values.
    LightingUniforms mLatchedLighting{};
    // Per-draw vertex-pipeline constants (UV transform, clamp bounds, fog factor
    // source), latched in GfxSpTri1 with the same flush-on-change rule and copied
    // into the combiner uniforms at flush time.
    float mUvTransform[2][4]{};
    float mTextureClamp[2][4]{};
    float mFogParams[4]{};
    float mLodParams[4]{};

    // GPU vertex transform: history of recent model-view-projection matrices
    // (aspect scale folded in) captured at vertex-load time. Each LoadedVertex
    // references an entry; GfxSpTri1 maps the referenced entries onto the small
    // per-draw matrix palette uploaded to the vertex shader.
    static constexpr size_t MTX_HISTORY_SIZE = 64;
    float mMtxHistory[MTX_HISTORY_SIZE][4][4]{};
    uint8_t mMtxHistoryHead = 0;
    uint8_t mMtxHistoryCurrent = 0;
    bool mMtxCurrentValid = false;
    float mMtxCurrentAspect = 1.0f;
    uint8_t mMtxIdentityEntry = 0;
    bool mMtxIdentityValid = false;
    // Per-batch mapping from history entry -> palette slot (-1 = not in palette)
    int8_t mBatchSlotForHistory[MTX_HISTORY_SIZE]{};
    uint8_t mBatchMtxCount = 0;
    TransformUniforms mTransform{};

    uint8_t AppendMtxHistory(const float m[4][4], float aspectScale);
    uint8_t GetIdentityMtxSlot();

    // GPU palettization: the import in progress uploads raw CI indices instead of
    // decoded colors (palette lookup happens in the fragment shader).
    bool mImportIndexed = false;
    // The import in progress is an HD (upscaled) texture. Auto-generated mipmaps
    // are only built for these; original low-res N64 textures upload single-level.
    bool mImportIsHd = false;
    // Palette textures are versioned by TLUT content and never mutated once
    // uploaded: backends queue draw commands (Metal executes at end of frame),
    // so rewriting a bound palette would retroactively recolor earlier draws.
    // A content-hash ring caches one immutable 256-entry texture per TLUT.
    static constexpr size_t PALETTE_RING_SIZE = 1024;
    uint32_t mPaletteRingTexture[PALETTE_RING_SIZE];
    uint64_t mPaletteRingHash[PALETTE_RING_SIZE]{};
    // Last frame each slot was used; a slot used this frame must not be recycled.
    uint32_t mPaletteRingFrameUsed[PALETTE_RING_SIZE]{};
    size_t mPaletteRingNext = 0;
    std::unordered_map<uint64_t, size_t> mPaletteSlotByHash;
    uint64_t mCurrentPaletteHash = 0;
    uint32_t mBoundPaletteTexture = 0xFFFFFFFF;
    // Returns the texture id for the current TLUT content, uploading it if new
    uint32_t AcquirePaletteTexture();
    // Per-draw palette parameters: x = palette bank entry offset, y = filter mode
    float mPaletteParams[2][4]{};
    uint8_t* mTexUploadBuffer = nullptr;
    // Ping-pong scratch buffers for CPU-generated mip levels (auto mipmapping)
    std::vector<uint8_t> mMipScratch[2];

    GfxDimensions mGfxCurrentWindowDimensions{}; // gfx_current_window_dimensions;
    int32_t mCurWindowPosX{};
    int32_t mCurWindowPosY{};
    GfxDimensions mCurDimensions{};        // gfx_current_dimensions;
    GfxDimensions mPrvDimensions{};        // gfx_prev_dimensions;
    XYWidthHeight mGameWindowViewport{};   // gfx_current_game_window_viewport;
    XYWidthHeight mNativeDimensions{};     // gfx_native_dimensions;
    XYWidthHeight mPrevNativeDimensions{}; // gfx_prev_native_dimensions;
    uintptr_t mGfxFrameBuffer{};

    unsigned int mMsaaLevel = 1;
    bool mDroppedFrame{};
    float* mBufVbo; // 3 vertices in a triangle and 32 floats per vtx
    size_t mBufVboLen{};
    size_t mBufVboNumTris{};
    GfxWindowBackend* mWapi = nullptr;
    GfxRenderingAPI* mRapi = nullptr;
    std::shared_ptr<GfxDebugger> mGfxDebugger;
    std::shared_ptr<Ship::ResourceManager> mResourceManager; ///< Cached ResourceManager, set in Init().
    std::shared_ptr<Ship::ConsoleVariable> mConsoleVariable; ///< Cached ConsoleVariable, set in Init().
    std::weak_ptr<Fast3dWindow> mFast3dWindow;               ///< Cached Fast3dWindow, set in OnInit().

    uintptr_t mSegmentPointers[MAX_SEGMENT_POINTERS]{};

    bool mFbActive{};
    bool mRendersToFb{}; // game_renders_to_framebuffer;
    std::map<int, FBInfo>::iterator mActiveFrameBuffer;
    std::map<int, FBInfo> mFrameBuffers;

    int mGameFb{};             // game_framebuffer;
    int mGameFbMsaaResolved{}; // game_framebuffer_msaa_resolved;

    std::set<std::pair<float, float>> mGetPixelDepthPending; // get_pixel_depth_pending;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> mGetPixelDepthCached; // get_pixel_depth_cached;
    std::map<std::string, MaskedTextureEntry, std::less<>> mMaskedTextures;
    std::unordered_map<uintptr_t, int> mFbTextures; // CPU addr -> GPU FB id

    const std::unordered_map<Mtx*, MtxF>* mCurMtxReplacements;
    const std::unordered_map<Gfx*, Gfx*>* mCurDlReplacements;
    bool mMarkerOn; // This was originally a debug feature. Now it seems to control s2dex?
    // Registered custom shader templates (id -> o2r path). Paths are owned
    // copies: ids live in compiled-pipeline cache keys for the whole session.
    std::unordered_map<size_t, std::string> mShaders;

    typedef size_t ShaderId;
    std::stack<ShaderId> mShaderStack;
    size_t mShadersIndex = 0;
    CustomUniforms mCustomUniforms{};
    uint32_t mCustomFrameCount = 0;
    double mCustomTimeSeconds = 0.0;

    // Post-processing chain state (see RegisterPostPass)
    std::vector<PostPass> mPostPasses;
    std::mutex mPostPassMutex;

    // Tweakable shader settings discovered from @setting(...) directives at
    // compile time (see gfx_register_shader_settings). Values are baked into
    // the generated shader source; edits recompile via gfx_shader_cache_clear.
    std::map<size_t, ShaderSettings> mShaderSettings;

    // Per-display-list shader mapping ("materials" in the shader-pack
    // manifest): o2r DL path -> shader template path. Scopes are cmd_stack
    // depths at which a mapped shader was pushed; popped on the matching ENDDL.
    std::unordered_map<std::string, std::string> mMaterialShaders;
    std::unordered_map<std::string, std::string> mShaderPackNames;
    std::vector<size_t> mDlShaderScopes;
    void ApplyMaterialShader(const char* dlistPath);
    void PopMaterialShaderScopes();
    int mPostPassNextId = 1;
    int mPostPassFb[2] = { -1, -1 }; // ping-pong targets, created lazily
    bool mPostManifestChecked = false;
    // Registers (or finds) a custom shader template path, returning its id for
    // the combiner key. Shared by G_PUSH_SHADER and the post-process chain.
    size_t RegisterShaderPath(const char* path);
    // Executes the registered passes over the game framebuffer; returns the
    // rapi framebuffer id holding the final image, or -1 when nothing ran.
    int RunPostPasses();
    void LoadPostPassManifest();
    int mInterpolationIndex;
    int mInterpolationIndexTarget;
    // Interpolation factor for the current rendered frame within a game tick:
    // 0 = previous tick, 1 = current tick. Set by the port before each
    // DrawAndRunGraphicsCommands call, like mInterpolationIndex.
    float mInterpolationT = 1.0f;
    int mInterpolationTotal;
    float mInterpolationFrac;
    // N64 RGB framebuffer dither (G_CD_*), applied per-draw in the fragment shader.
    // Cached once per frame from the gEnhancements.Graphics.DitherNoise CVar.
    bool mRgbDitherEnabled = true;

    // Budgeted HD-replacement uploads. Big (e.g. 4K) replacement textures are expensive
    // to upload; uploading several the same frame they first appear causes a hitch.
    // We cap how many *new* replacement textures upload per frame and render the base
    // texture in the meantime (the blend reproduces the base), retrying on later frames.
    int mReplacementUploadBudget = 1; // max new replacement uploads per frame (0 = unlimited)
    int mFrameReplacementUploads = 0; // count uploaded so far this frame
    bool mAllowReplacementDefer = false; // set by the draw path only for non-indexed bases
    bool mDeferredReplacementUpload = false; // set by ImportTexture when it deferred an upload
    bool mReplacementUploadedThisCall = false; // set by ImportTexture when it uploaded an HD this call

    // Debug visualization of HD replacement state, tinting each draw in the fragment
    // shader (gEnhancements.Graphics.TextureReplacementDebug). State per draw:
    // 1 = HD active (blue, subtle), 2 = just uploaded this frame (green), 3 = base
    // shown because the HD upload was deferred (red). 0 = no replacement / no tint.
    bool mTextureReplacementDebug = false;
    int mDebugTintState = 0;

    // Async texture loading (gEnhancements.Graphics.AsyncTextureLoad). When on (and alt
    // assets enabled), a texture's HD/replacement resource is decoded on the resource
    // thread pool while the cheap vanilla version renders; the HD swaps in once its load
    // finishes ("replaced between frames"), so the render thread never blocks on the decode.
    bool mAsyncTextureLoad = false;
    std::unordered_map<std::string, std::shared_future<std::shared_ptr<Ship::IResource>>> mTexFutures;
    // Textures whose HD version has already been swapped in (uploaded once, now cache-resident);
    // they bypass the per-frame swap budget so they never flicker back to vanilla.
    std::unordered_set<std::string> mTexSwappedIn;
    // Returns the texture resource to draw with: the resolved HD if its async load is ready,
    // otherwise the vanilla fallback (kicking the async load on first reference). Falls back
    // to a plain cached load when async is disabled or alt assets are off.
    std::shared_ptr<Ship::IResource> AcquireDrawTexture(const char* name);
};

void gfx_set_target_ucode(UcodeHandlers ucode);
const char* gfx_get_current_ucode_name();
void gfx_push_current_dir(char* path);
int32_t gfx_check_image_signature(const char* imgData);
const char* gfx_get_shader(int16_t id);

// Shader-pack settings plumbing used by the backend shader builders:
// register the @setting declarations discovered while processing a template,
// and fetch the current values pre-formatted as prism context items.
void gfx_register_shader_settings(int16_t shaderId, const std::vector<prism::SettingDecl>& decls);
std::vector<std::pair<std::string, prism::ContextTypes>> gfx_get_shader_setting_values(int16_t shaderId);
const char* GfxGetOpcodeName(int8_t opcode);

} // namespace Fast

extern "C" void gfx_texture_cache_clear();
extern "C" void gfx_set_custom_uniform(uint8_t idx, const float values[4]);
extern "C" int gfx_register_post_pass(const char* o2rShaderPath);
extern "C" void gfx_unregister_post_pass(int id);
extern "C" void gfx_clear_post_passes();
extern "C" void gfx_shader_cache_clear();
extern "C" int gfx_create_framebuffer(uint32_t width, uint32_t height, uint32_t native_width, uint32_t native_height,
                                      uint8_t resize, bool forceFixedAspect = false);
