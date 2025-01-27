#ifndef GFX_PC_H
#define GFX_PC_H

#include <stdbool.h>
#include <stdint.h>
#include <unordered_map>
#include <list>
#include <cstddef>
#include <vector>
#include <stack>
#include <string>

#include "graphic/Fast3D/lus_gbi.h"
#include "libultraship/libultra/types.h"
#include "public/bridge/gfxbridge.h"
#include "gfx_cc.h"
#include "gfx_rendering_api.h"

#include "resource/type/Texture.h"
#include "resource/type/Light.h"
#include "resource/Resource.h"

// TODO figure out why changing these to 640x480 makes the game only render in a quarter of the window
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

struct GfxRenderingAPI;
struct GfxWindowManagerAPI;

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
union Gfx;

struct GfxExecStack {
    // This is a dlist stack used to handle dlist calls.
    std::stack<F3DGfx*> cmd_stack = {};
    // This is also a dlist stack but a std::vector is used to make it possible
    // to iterate on the elements.
    // The purpose of this is to identify an instruction at a poin in time
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

struct RGBA {
    uint8_t r, g, b, a;
};

struct LoadedVertex {
    float x, y, z, w;
    float u, v;
    struct RGBA color;
    uint8_t clip_rej;
};

struct RawTexMetadata {
    uint16_t width, height;
    float h_byte_scale = 1, v_pixel_scale = 1;
    std::shared_ptr<Fast::Texture> resource;
    Fast::TextureType type;
};

struct ShaderMod {
    bool enabled = false;
    int16_t id;
    uint8_t type;
};

#define MAX_BUFFERED 256
// #define MAX_LIGHTS 2
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

    struct LoadedVertex loaded_vertices[MAX_VERTICES + 4];
};

struct RDP {
    const uint8_t* palettes[2];
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
        uint8_t shifts, shiftt;
        uint16_t uls, ult, lrs, lrt; // U10.2
        uint16_t tmem;               // 0-511, in 64-bit word units
        uint32_t line_size_bytes;
        uint8_t palette;
        uint8_t tmem_index; // 0 or 1 for offset 0 kB or offset 2 kB, respectively
    } texture_tile[8];
    bool textures_changed[2];

    uint8_t first_tile_index;

    uint32_t other_mode_l, other_mode_h;
    uint64_t combine_mode;
    bool grayscale;
    ShaderMod current_shader;

    uint8_t prim_lod_fraction;
    struct RGBA env_color, prim_color, fog_color, fill_color, grayscale_color;
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
};

struct ColorCombiner {
    uint64_t shader_id0;
    uint32_t shader_id1;
    bool used_textures[2];
    struct ShaderProgram* prg[16];
    uint8_t shader_input_mapping[2][7];
};

struct RenderingState {
    uint8_t depth_test_and_mask; // 1: depth test, 2: depth mask
    bool decal_mode;
    bool alpha_blend;
    struct XYWidthHeight viewport, scissor;
    struct ShaderProgram* shader_program;
    TextureCacheNode* textures[SHADER_MAX_TEXTURES];
};

struct FBInfo {
    uint32_t orig_width, orig_height;       // Original shape
    uint32_t applied_width, applied_height; // Up-scaled for the viewport
    uint32_t native_width, native_height;   // Max "native" size of the screen, used for up-scaling
    bool resize;                            // Scale to match the viewport
};

struct MaskedTextureEntry {
    uint8_t* mask;
    uint8_t* replacementData;
};

class GfxPc {
    GfxPc();
public:
    static GfxPc* GetInstance();
    static GfxPc* CreateInstance();


    void Init(struct GfxWindowManagerAPI* wapi, struct GfxRenderingAPI* rapi, const char* game_name,
              bool start_in_fullscreen, uint32_t width, uint32_t height, uint32_t posX, uint32_t posY);
    void Destroy();
    void GetDimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY);
    GfxRenderingAPI* GetCurrentRenderingAPI();
    void StartFrame();
    void Run(Gfx* commands, const std::unordered_map<Mtx*, MtxF>& mtx_replacements);
    void EndFrame();
    void HandleWindowEvents();
    bool IsFrameReady();
    void SetTargetFPS(int fps);
    void SetMaxFrameLatency(int latency);
    int CreateFrameBuffer(uint32_t width, uint32_t height, uint32_t native_width, uint32_t native_height,
                                      uint8_t resize);
    void SetFrameBuffer(int fb, float noiseScale);
    void CopyFrameBuffer(int fb_dst_id, int fb_src_id, bool copyOnce, bool* hasCopiedPtr);
    void ResetFrameBuffer();
    void AdjustPixelDepthCoordinates(float& x, float& y);
    void GetPixelDepthPrepare(float x, float y);
    uint16_t GetPixelDepth(float x, float y);
    void RegisterBlendedTexture(const char* name, uint8_t* mask, uint8_t* replacement);
    void UnregisterBlendedTexture(const char* name);

    void SetNativeDimensions(float width, float height);
    void SetResolutionMultiplier(float multiplier);
    void SetMsaaLevel(uint32_t level);
    void GetCurDimensions(uint32_t* width, uint32_t* height);

    //private: TODO make these private
    void Flush();
    ShaderProgram* LookupOrCreateShaderProgram(uint64_t id0, uint64_t id1);
    ColorCombiner* LookupOrCreateColorCombiner(const ColorCombinerKey& key);
    void TextureCacheClear();
    bool TextureCacheLookup(int i, const TextureCacheKey& key);
    void TextureCacheDelete(const uint8_t* origAddr);
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
	void ImportTextureImg(int tile, bool importReplacement)
    void ImportTexture(int i, int tile, bool importReplacement);
    void ImportTextureMask(int i, int tile);
    void CalculateNormalDir(const F3DLight_t*, float coeffs[3]);

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
    void GfxDpSetTextureImage(uint32_t format, uint32_t size, uint32_t width, const char* texPath,
                                     uint32_t texFlags, RawTexMetadata rawTexMetdata, const void* addr);
    void GfxDpSetTile(uint8_t fmt, uint32_t siz, uint32_t line, uint32_t tmem, uint8_t tile, uint32_t palette,
                            uint32_t cmt, uint32_t maskt, uint32_t shiftt, uint32_t cms, uint32_t masks,
                            uint32_t shifts);
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
    void GfxDpImageRectangle(int32_t tile, int32_t w, int32_t h, int32_t ulx, int32_t uly, int16_t uls,
                                   int16_t ult, int32_t lrx, int32_t lry, int16_t lrs, int16_t lrt);
    void GfxDpFillRectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry);
    void GfxDpSetZImage(void* zBufAddr);
    void GfxDpSetColorImage(uint32_t format, uint32_t size, uint32_t width, void* address);
    void GfxSpSetOtherMode(uint32_t shift, uint32_t num_bits, uint64_t mode);
    void GfxDpSetOtherMode(uint32_t h, uint32_t l);

    void Gfxs2dexBgCopy(F3DuObjBg* bg);
    void Gfxs2dexBg1cyc(F3DuObjBg* bg);
    void Gfxs2dexRecyCopy(F3DuObjSprite* spr);


    void AdjustWidthHeightForScale(uint32_t& width, uint32_t& height, uint32_t nativeWidth, uint32_t nativeHeight) const;
    float AdjXForAspectRatio(float x) const;
    void AdjustVIewportOrScissor(XYWidthHeight* area);
    void CalcAndSetViewport(const F3DVp_t* viewport);

    void SpReset();
    void* SegAddr(uintptr_t w1);

    static const char* CCMUXtoStr(uint32_t ccmux);
    static const char* ACMUXtoStr(uint32_t acmux);
    static void GenerateCC(ColorCombiner* comb, const ColorCombinerKey& key);
    static std::string GetBaseTexturePath(const std::string& path);
    static void NormalizeVector(float v[3]);
    static void TransposedMatrixMul(float res[3], const float a[3], const float b[4][4]);
    static void MatrixMul(float res[4][4], const float a[4][4], const float b[4][4]);


    RSP mRsp;
    RDP mRdp;
    RenderingState mRenderingState;

    GfxTextureCache mTextureCache;
    std::map<ColorCombinerKey, ColorCombiner> mColorCombinerPool;//color_combiner_pool;
    std::map<ColorCombinerKey, ColorCombiner>::iterator mPrevCombiner = mColorCombinerPool.end();
    uint8_t* mTexUploadBuffer;

    GfxDimensions mGfxCurrentWindowDimensions;//gfx_current_window_dimensions;
    int32_t mCurWindowPosX;
    int32_t mCurWindowPosY;
    GfxDimensions mCurDimensions;//gfx_current_dimensions;
    GfxDimensions mPrvDimensions;//gfx_prev_dimensions;
    XYWidthHeight mGameWindowViewport;//gfx_current_game_window_viewport;
    XYWidthHeight mNativeDimensions;//gfx_native_dimensions;
    XYWidthHeight mPrevNativeDimensions;//gfx_prev_native_dimensions;
    uintptr_t mGfxFrameBuffer;

    unsigned int mMsaaLevel = 1;
    bool mDroppedFrame;
    float mBufVbo[MAX_BUFFERED * (32 * 3)]; // 3 vertices in a triangle and 32 floats per vtx
    size_t mBufVboLen;
    size_t mBufVboNumTris;
    GfxWindowManagerAPI* mWapi;
    GfxRenderingAPI* mRapi;

    uintptr_t mSegmentPointers[16];

    bool mFbActive;
    bool mRendersToFb;//game_renders_to_framebuffer;
    std::map<int, FBInfo>::iterator mActiveFrameBuffer;
    std::map<int, FBInfo> mFrameBuffers;

    int mGameFb;//game_framebuffer;
    int mGameFbMsaaResolved;//game_framebuffer_msaa_resolved;

    std::set<std::pair<float, float>> mGetPixelDepthPending;//get_pixel_depth_pending;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> mGetPixelDepthCached;//get_pixel_depth_cached;
    std::map<std::string, MaskedTextureEntry> mMaskedTextures;

    const std::unordered_map<Mtx*, MtxF>* mCurMtxReplacements;
    bool mMarkerOn; //This was originally a debug feature. Now it seems to control s2dex?
};



//extern "C" {

// extern struct XYWidthHeight
//     gfx_native_dimensions; // The dimensions of the VI mode for the console (typically SCREEN_WIDTH, SCREEN_HEIGHT)
//
// extern struct GfxDimensions gfx_current_window_dimensions; // The dimensions of the window
// extern struct GfxDimensions
//     gfx_current_dimensions; // The dimensions of the draw area the game draws to, before scaling (if applicable)
// extern struct XYWidthHeight
//     gfx_current_game_window_viewport; // The area of the window the game is drawn to, (0, 0) is top-left corner
// extern uint32_t gfx_msaa_level;
// }

//void gfx_init(struct GfxWindowManagerAPI* wapi, struct GfxRenderingAPI* rapi, const char* game_name,
//              bool start_in_fullscreen, uint32_t width = SCREEN_WIDTH, uint32_t height = SCREEN_HEIGHT,
//              uint32_t posX = 100, uint32_t posY = 100);
//void gfx_destroy(void);
//struct GfxRenderingAPI* gfx_get_current_rendering_api(void);
//void gfx_start_frame(void);

// Since this function is "exposted" to the games, it needs to take a normal Gfx
//void gfx_run(Gfx* commands, const std::unordered_map<Mtx*, MtxF>& mtx_replacements);
//void gfx_end_frame(void);

void gfx_set_target_ucode(UcodeHandlers ucode);
//void gfx_set_target_fps(int);
//void gfx_set_maximum_frame_latency(int latency);
//void gfx_texture_cache_delete(const uint8_t* orig_addr);
extern "C" void gfx_texture_cache_clear();
extern "C" int gfx_create_framebuffer(uint32_t width, uint32_t height, uint32_t native_width, uint32_t native_height,
                                      uint8_t resize);
//void gfx_get_pixel_depth_prepare(float x, float y);
//uint16_t gfx_get_pixel_depth(float x, float y);
void gfx_push_current_dir(char* path);
int32_t gfx_check_image_signature(const char* imgData);
//void gfx_register_blended_texture(const char* name, uint8_t* mask, uint8_t* replacement = nullptr);
//void gfx_unregister_blended_texture(const char* name);
const char* GfxGetOpcodeName(int8_t opcode);

#endif
