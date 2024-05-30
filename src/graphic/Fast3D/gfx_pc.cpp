#define NOMINMAX

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#include <any>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>
#include <list>
#include <stack>

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include "debug/GfxDebugger.h"
#include "libultraship/libultra/types.h"
//#include "libultraship/libultra/gs2dex.h"
#include <string>

#include "gfx_pc.h"
#include "gfx_cc.h"
#include "lus_gbi.h"
#include "gfx_window_manager_api.h"
#include "gfx_rendering_api.h"
#include "gfx_screen_config.h"

#include "log/luslog.h"
#include "window/gui/Gui.h"
#include "resource/GameVersions.h"
#include "resource/ResourceManager.h"
#include "utils/Utils.h"
#include "Context.h"
#include "libultraship/bridge.h"

#include <spdlog/fmt/fmt.h>

uintptr_t gfxFramebuffer;
std::stack<std::string> currentDir;

using namespace std;

#define SEG_ADDR(seg, addr) (addr | (seg << 24) | 1)
#define SUPPORT_CHECK(x) assert(x)

// SCALE_M_N: upscale/downscale M-bit integer to N-bit
#define SCALE_5_8(VAL_) (((VAL_)*0xFF) / 0x1F)
#define SCALE_8_5(VAL_) ((((VAL_) + 4) * 0x1F) / 0xFF)
#define SCALE_4_8(VAL_) ((VAL_)*0x11)
#define SCALE_8_4(VAL_) ((VAL_) / 0x11)
#define SCALE_3_8(VAL_) ((VAL_)*0x24)
#define SCALE_8_3(VAL_) ((VAL_) / 0x24)

// Based off the current set native dimensions
#define HALF_SCREEN_WIDTH (gfx_native_dimensions.width / 2)
#define HALF_SCREEN_HEIGHT (gfx_native_dimensions.height / 2)

#define RATIO_X (gfx_current_dimensions.width / (2.0f * HALF_SCREEN_WIDTH))
#define RATIO_Y (gfx_current_dimensions.height / (2.0f * HALF_SCREEN_HEIGHT))

#define TEXTURE_CACHE_MAX_SIZE 500

static struct {
    TextureCacheMap map;
    list<TextureCacheMapIter> lru;
    vector<uint32_t> free_texture_ids;
} gfx_texture_cache;

struct ColorCombiner {
    uint64_t shader_id0;
    uint32_t shader_id1;
    bool used_textures[2];
    struct ShaderProgram* prg[16];
    uint8_t shader_input_mapping[2][7];
};

static map<ColorCombinerKey, struct ColorCombiner> color_combiner_pool;
static map<ColorCombinerKey, struct ColorCombiner>::iterator prev_combiner = color_combiner_pool.end();

static uint8_t* tex_upload_buffer = nullptr;

RSP g_rsp;
RDP g_rdp;
static struct RenderingState {
    uint8_t depth_test_and_mask; // 1: depth test, 2: depth mask
    bool decal_mode;
    bool alpha_blend;
    struct XYWidthHeight viewport, scissor;
    struct ShaderProgram* shader_program;
    TextureCacheNode* textures[SHADER_MAX_TEXTURES];
} rendering_state;

struct GfxDimensions gfx_current_window_dimensions;
int32_t gfx_current_window_position_x;
int32_t gfx_current_window_position_y;
struct GfxDimensions gfx_current_dimensions;
static struct GfxDimensions gfx_prev_dimensions;
struct XYWidthHeight gfx_current_game_window_viewport;
struct XYWidthHeight gfx_native_dimensions;
struct XYWidthHeight gfx_prev_native_dimensions;

static bool game_renders_to_framebuffer;
static int game_framebuffer;
static int game_framebuffer_msaa_resolved;

uint32_t gfx_msaa_level = 1;

static bool has_drawn_imgui_menu;

static bool dropped_frame;

static const std::unordered_map<Mtx*, MtxF>* current_mtx_replacements;

static float buf_vbo[MAX_BUFFERED * (32 * 3)]; // 3 vertices in a triangle and 32 floats per vtx
static size_t buf_vbo_len;
static size_t buf_vbo_num_tris;

static struct GfxWindowManagerAPI* gfx_wapi;
static struct GfxRenderingAPI* gfx_rapi;

static int markerOn;
uintptr_t gSegmentPointers[16];

struct FBInfo {
    uint32_t orig_width, orig_height;       // Original shape
    uint32_t applied_width, applied_height; // Up-scaled for the viewport
    uint32_t native_width, native_height;   // Max "native" size of the screen, used for up-scaling
    bool resize;                            // Scale to match the viewport
};

static bool fbActive = 0;
static map<int, FBInfo>::iterator active_fb;
static map<int, FBInfo> framebuffers;

static set<pair<float, float>> get_pixel_depth_pending;
static unordered_map<pair<float, float>, uint16_t, hash_pair_ff> get_pixel_depth_cached;

struct MaskedTextureEntry {
    uint8_t* mask;
    uint8_t* replacementData;
};

static map<string, MaskedTextureEntry> masked_textures;

static UcodeHandlers ucode_handler_index = ucode_f3dex2;

const static std::unordered_map<Attribute, std::any> f3dex2AttrHandler = {
    { MTX_PROJECTION, F3DEX2_G_MTX_PROJECTION }, { MTX_LOAD, F3DEX2_G_MTX_LOAD },     { MTX_PUSH, F3DEX2_G_MTX_PUSH },
    { MTX_NOPUSH, F3DEX_G_MTX_NOPUSH },          { CULL_FRONT, F3DEX2_G_CULL_FRONT }, { CULL_BACK, F3DEX2_G_CULL_BACK },
    { CULL_BOTH, F3DEX2_G_CULL_BOTH },
};

const static std::unordered_map<Attribute, std::any> f3dexAttrHandler = { { MTX_PROJECTION, F3DEX_G_MTX_PROJECTION },
                                                                          { MTX_LOAD, F3DEX_G_MTX_LOAD },
                                                                          { MTX_PUSH, F3DEX_G_MTX_PUSH },
                                                                          { MTX_NOPUSH, F3DEX_G_MTX_NOPUSH },
                                                                          { CULL_FRONT, F3DEX_G_CULL_FRONT },
                                                                          { CULL_BACK, F3DEX_G_CULL_BACK },
                                                                          { CULL_BOTH, F3DEX_G_CULL_BOTH } };

static constexpr std::array ucode_attr_handlers = {
    &f3dexAttrHandler,
    &f3dexAttrHandler,
    &f3dex2AttrHandler,
    &f3dex2AttrHandler,
};

template <typename T> static constexpr T get_attr(Attribute attr) {
    const auto ucode_map = ucode_attr_handlers[ucode_handler_index];
    assert(ucode_map->contains(attr) && "Attribute not found in the current ucode handler");
    return std::any_cast<T>(ucode_map->at(attr));
}

static std::string GetPathWithoutFileName(char* filePath) {
    size_t len = strlen(filePath);

    for (size_t i = len - 1; i >= 0; i--) {
        if (filePath[i] == '/' || filePath[i] == '\\') {
            return std::string(filePath).substr(0, i);
        }
    }

    return filePath;
}

static void gfx_flush(void) {
    if (buf_vbo_len > 0) {
        gfx_rapi->draw_triangles(buf_vbo, buf_vbo_len, buf_vbo_num_tris);
        buf_vbo_len = 0;
        buf_vbo_num_tris = 0;
    }
}

static struct ShaderProgram* gfx_lookup_or_create_shader_program(uint64_t shader_id0, uint32_t shader_id1) {
    struct ShaderProgram* prg = gfx_rapi->lookup_shader(shader_id0, shader_id1);
    if (prg == NULL) {
        gfx_rapi->unload_shader(rendering_state.shader_program);
        prg = gfx_rapi->create_and_load_new_shader(shader_id0, shader_id1);
        rendering_state.shader_program = prg;
    }
    return prg;
}

static const char* ccmux_to_string(uint32_t ccmux) {
    static const char* const tbl[] = {
        "G_CCMUX_COMBINED",
        "G_CCMUX_TEXEL0",
        "G_CCMUX_TEXEL1",
        "G_CCMUX_PRIMITIVE",
        "G_CCMUX_SHADE",
        "G_CCMUX_ENVIRONMENT",
        "G_CCMUX_1",
        "G_CCMUX_COMBINED_ALPHA",
        "G_CCMUX_TEXEL0_ALPHA",
        "G_CCMUX_TEXEL1_ALPHA",
        "G_CCMUX_PRIMITIVE_ALPHA",
        "G_CCMUX_SHADE_ALPHA",
        "G_CCMUX_ENV_ALPHA",
        "G_CCMUX_LOD_FRACTION",
        "G_CCMUX_PRIM_LOD_FRAC",
        "G_CCMUX_K5",
    };
    if (ccmux > 15) {
        return "G_CCMUX_0";

    } else {
        return tbl[ccmux];
    }
}

static const char* acmux_to_string(uint32_t acmux) {
    static const char* const tbl[] = {
        "G_ACMUX_COMBINED or G_ACMUX_LOD_FRACTION",
        "G_ACMUX_TEXEL0",
        "G_ACMUX_TEXEL1",
        "G_ACMUX_PRIMITIVE",
        "G_ACMUX_SHADE",
        "G_ACMUX_ENVIRONMENT",
        "G_ACMUX_1 or G_ACMUX_PRIM_LOD_FRAC",
        "G_ACMUX_0",
    };
    return tbl[acmux];
}

static void gfx_generate_cc(struct ColorCombiner* comb, const ColorCombinerKey& key) {
    bool is_2cyc = (key.options & SHADER_OPT(_2CYC)) != 0;

    uint8_t c[2][2][4];
    uint64_t shader_id0 = 0;
    uint32_t shader_id1 = key.options;
    uint8_t shader_input_mapping[2][7] = { { 0 } };
    bool used_textures[2] = { false, false };
    for (uint32_t i = 0; i < 2 && (i == 0 || is_2cyc); i++) {
        uint32_t rgb_a = (key.combine_mode >> (i * 28)) & 0xf;
        uint32_t rgb_b = (key.combine_mode >> (i * 28 + 4)) & 0xf;
        uint32_t rgb_c = (key.combine_mode >> (i * 28 + 8)) & 0x1f;
        uint32_t rgb_d = (key.combine_mode >> (i * 28 + 13)) & 7;
        uint32_t alpha_a = (key.combine_mode >> (i * 28 + 16)) & 7;
        uint32_t alpha_b = (key.combine_mode >> (i * 28 + 16 + 3)) & 7;
        uint32_t alpha_c = (key.combine_mode >> (i * 28 + 16 + 6)) & 7;
        uint32_t alpha_d = (key.combine_mode >> (i * 28 + 16 + 9)) & 7;

        if (rgb_a >= 8) {
            rgb_a = G_CCMUX_0;
        }
        if (rgb_b >= 8) {
            rgb_b = G_CCMUX_0;
        }
        if (rgb_c >= 16) {
            rgb_c = G_CCMUX_0;
        }
        if (rgb_d == 7) {
            rgb_d = G_CCMUX_0;
        }

        if (rgb_a == rgb_b || rgb_c == G_CCMUX_0) {
            // Normalize
            rgb_a = G_CCMUX_0;
            rgb_b = G_CCMUX_0;
            rgb_c = G_CCMUX_0;
        }
        if (alpha_a == alpha_b || alpha_c == G_ACMUX_0) {
            // Normalize
            alpha_a = G_ACMUX_0;
            alpha_b = G_ACMUX_0;
            alpha_c = G_ACMUX_0;
        }
        if (i == 1) {
            if (rgb_a != G_CCMUX_COMBINED && rgb_b != G_CCMUX_COMBINED && rgb_c != G_CCMUX_COMBINED &&
                rgb_d != G_CCMUX_COMBINED) {
                // First cycle RGB not used, so clear it away
                c[0][0][0] = c[0][0][1] = c[0][0][2] = c[0][0][3] = G_CCMUX_0;
            }
            if (rgb_c != G_CCMUX_COMBINED_ALPHA && alpha_a != G_ACMUX_COMBINED && alpha_b != G_ACMUX_COMBINED &&
                alpha_d != G_ACMUX_COMBINED) {
                // First cycle ALPHA not used, so clear it away
                c[0][1][0] = c[0][1][1] = c[0][1][2] = c[0][1][3] = G_ACMUX_0;
            }
        }

        c[i][0][0] = rgb_a;
        c[i][0][1] = rgb_b;
        c[i][0][2] = rgb_c;
        c[i][0][3] = rgb_d;
        c[i][1][0] = alpha_a;
        c[i][1][1] = alpha_b;
        c[i][1][2] = alpha_c;
        c[i][1][3] = alpha_d;
    }
    if (!is_2cyc) {
        for (uint32_t i = 0; i < 2; i++) {
            for (uint32_t k = 0; k < 4; k++) {
                c[1][i][k] = i == 0 ? G_CCMUX_0 : G_ACMUX_0;
            }
        }
    }
    {
        uint8_t input_number[32] = { 0 };
        uint32_t next_input_number = SHADER_INPUT_1;
        for (uint32_t i = 0; i < 2 && (i == 0 || is_2cyc); i++) {
            for (uint32_t j = 0; j < 4; j++) {
                uint32_t val = 0;
                switch (c[i][0][j]) {
                    case G_CCMUX_0:
                        val = SHADER_0;
                        break;
                    case G_CCMUX_1:
                        val = SHADER_1;
                        break;
                    case G_CCMUX_TEXEL0:
                        val = SHADER_TEXEL0;
                        used_textures[0] = true;
                        break;
                    case G_CCMUX_TEXEL1:
                        val = SHADER_TEXEL1;
                        used_textures[1] = true;
                        break;
                    case G_CCMUX_TEXEL0_ALPHA:
                        val = SHADER_TEXEL0A;
                        used_textures[0] = true;
                        break;
                    case G_CCMUX_TEXEL1_ALPHA:
                        val = SHADER_TEXEL1A;
                        used_textures[1] = true;
                        break;
                    case G_CCMUX_NOISE:
                        val = SHADER_NOISE;
                        break;
                    case G_CCMUX_PRIMITIVE:
                    case G_CCMUX_PRIMITIVE_ALPHA:
                    case G_CCMUX_PRIM_LOD_FRAC:
                    case G_CCMUX_SHADE:
                    case G_CCMUX_ENVIRONMENT:
                    case G_CCMUX_ENV_ALPHA:
                    case G_CCMUX_LOD_FRACTION:
                        if (input_number[c[i][0][j]] == 0) {
                            shader_input_mapping[0][next_input_number - 1] = c[i][0][j];
                            input_number[c[i][0][j]] = next_input_number++;
                        }
                        val = input_number[c[i][0][j]];
                        break;
                    case G_CCMUX_COMBINED:
                        val = SHADER_COMBINED;
                        break;
                    default:
                        fprintf(stderr, "Unsupported ccmux: %d\n", c[i][0][j]);
                        break;
                }
                shader_id0 |= (uint64_t)val << (i * 32 + j * 4);
            }
        }
    }
    {
        uint8_t input_number[16] = { 0 };
        uint32_t next_input_number = SHADER_INPUT_1;
        for (uint32_t i = 0; i < 2; i++) {
            for (uint32_t j = 0; j < 4; j++) {
                uint32_t val = 0;
                switch (c[i][1][j]) {
                    case G_ACMUX_0:
                        val = SHADER_0;
                        break;
                    case G_ACMUX_TEXEL0:
                        val = SHADER_TEXEL0;
                        used_textures[0] = true;
                        break;
                    case G_ACMUX_TEXEL1:
                        val = SHADER_TEXEL1;
                        used_textures[1] = true;
                        break;
                    case G_ACMUX_LOD_FRACTION:
                        // case G_ACMUX_COMBINED: same numerical value
                        if (j != 2) {
                            val = SHADER_COMBINED;
                            break;
                        }
                        c[i][1][j] = G_CCMUX_LOD_FRACTION;
                        [[fallthrough]]; // for G_ACMUX_LOD_FRACTION
                    case G_ACMUX_1:
                        // case G_ACMUX_PRIM_LOD_FRAC: same numerical value
                        if (j != 2) {
                            val = SHADER_1;
                            break;
                        }
                        [[fallthrough]]; // for G_ACMUX_PRIM_LOD_FRAC
                    case G_ACMUX_PRIMITIVE:
                    case G_ACMUX_SHADE:
                    case G_ACMUX_ENVIRONMENT:
                        if (input_number[c[i][1][j]] == 0) {
                            shader_input_mapping[1][next_input_number - 1] = c[i][1][j];
                            input_number[c[i][1][j]] = next_input_number++;
                        }
                        val = input_number[c[i][1][j]];
                        break;
                }
                shader_id0 |= (uint64_t)val << (i * 32 + 16 + j * 4);
            }
        }
    }
    comb->shader_id0 = shader_id0;
    comb->shader_id1 = shader_id1;
    comb->used_textures[0] = used_textures[0];
    comb->used_textures[1] = used_textures[1];
    // comb->prg = gfx_lookup_or_create_shader_program(shader_id0, shader_id1);
    memcpy(comb->shader_input_mapping, shader_input_mapping, sizeof(shader_input_mapping));
}

static struct ColorCombiner* gfx_lookup_or_create_color_combiner(const ColorCombinerKey& key) {
    if (prev_combiner != color_combiner_pool.end() && prev_combiner->first == key) {
        return &prev_combiner->second;
    }

    prev_combiner = color_combiner_pool.find(key);
    if (prev_combiner != color_combiner_pool.end()) {
        return &prev_combiner->second;
    }
    gfx_flush();
    prev_combiner = color_combiner_pool.insert(make_pair(key, ColorCombiner())).first;
    gfx_generate_cc(&prev_combiner->second, key);
    return &prev_combiner->second;
}

void gfx_texture_cache_clear() {
    for (const auto& entry : gfx_texture_cache.map) {
        gfx_texture_cache.free_texture_ids.push_back(entry.second.texture_id);
    }
    gfx_texture_cache.map.clear();
    gfx_texture_cache.lru.clear();
}

static bool gfx_texture_cache_lookup(int i, const TextureCacheKey& key) {
    TextureCacheMap::iterator it = gfx_texture_cache.map.find(key);
    TextureCacheNode** n = &rendering_state.textures[i];

    if (it != gfx_texture_cache.map.end()) {
        gfx_rapi->select_texture(i, it->second.texture_id);
        *n = &*it;
        gfx_texture_cache.lru.splice(gfx_texture_cache.lru.end(), gfx_texture_cache.lru,
                                     it->second.lru_location); // move to back
        return true;
    }

    if (gfx_texture_cache.map.size() >= TEXTURE_CACHE_MAX_SIZE) {
        // Remove the texture that was least recently used
        it = gfx_texture_cache.lru.front().it;
        gfx_texture_cache.free_texture_ids.push_back(it->second.texture_id);
        gfx_texture_cache.map.erase(it);
        gfx_texture_cache.lru.pop_front();
    }

    uint32_t texture_id;
    if (!gfx_texture_cache.free_texture_ids.empty()) {
        texture_id = gfx_texture_cache.free_texture_ids.back();
        gfx_texture_cache.free_texture_ids.pop_back();
    } else {
        texture_id = gfx_rapi->new_texture();
    }

    it = gfx_texture_cache.map.insert(make_pair(key, TextureCacheValue())).first;
    TextureCacheNode* node = &*it;
    node->second.texture_id = texture_id;
    node->second.lru_location = gfx_texture_cache.lru.insert(gfx_texture_cache.lru.end(), { it });

    gfx_rapi->select_texture(i, texture_id);
    gfx_rapi->set_sampler_parameters(i, false, 0, 0);
    *n = node;
    return false;
}

static std::string gfx_get_base_texture_path(const std::string& path) {
    if (path.starts_with(Ship::IResource::gAltAssetPrefix)) {
        return path.substr(Ship::IResource::gAltAssetPrefix.length());
    }

    return path;
}

void gfx_texture_cache_delete(const uint8_t* orig_addr) {
    while (gfx_texture_cache.map.bucket_count() > 0) {
        TextureCacheKey key = { orig_addr, { 0 }, 0, 0, 0 }; // bucket index only depends on the address
        size_t bucket = gfx_texture_cache.map.bucket(key);
        bool again = false;
        for (auto it = gfx_texture_cache.map.begin(bucket); it != gfx_texture_cache.map.end(bucket); ++it) {
            if (it->first.texture_addr == orig_addr) {
                gfx_texture_cache.lru.erase(it->second.lru_location);
                gfx_texture_cache.free_texture_ids.push_back(it->second.texture_id);
                gfx_texture_cache.map.erase(it->first);
                again = true;
                break;
            }
        }
        if (!again) {
            break;
        }
    }
}

static void import_texture_rgba16(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? masked_textures.find(gfx_get_base_texture_path(metadata->resource->GetInitData()->Path))
                  ->second.replacementData
            : g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr;
    uint32_t size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes;
    uint32_t full_image_line_size_bytes =
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes;
    // SUPPORT_CHECK(full_image_line_size_bytes == line_size_bytes);

    uint32_t width = g_rdp.texture_tile[tile].line_size_bytes / 2;
    uint32_t height = size_bytes / g_rdp.texture_tile[tile].line_size_bytes;

    // A single line of pixels should not equal the entire image (height == 1 non-withstanding)
    if (full_image_line_size_bytes == size_bytes)
        full_image_line_size_bytes = width * 2;

    uint32_t i = 0;

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t clrIdx = (y * (full_image_line_size_bytes / 2)) + (x);

            uint16_t col16 = (addr[2 * clrIdx] << 8) | addr[2 * clrIdx + 1];
            uint8_t a = col16 & 1;
            uint8_t r = col16 >> 11;
            uint8_t g = (col16 >> 6) & 0x1f;
            uint8_t b = (col16 >> 1) & 0x1f;
            tex_upload_buffer[4 * i + 0] = SCALE_5_8(r);
            tex_upload_buffer[4 * i + 1] = SCALE_5_8(g);
            tex_upload_buffer[4 * i + 2] = SCALE_5_8(b);
            tex_upload_buffer[4 * i + 3] = a ? 255 : 0;

            i++;
        }
    }

    gfx_rapi->upload_texture(tex_upload_buffer, width, height);
}

static void import_texture_rgba32(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? masked_textures.find(gfx_get_base_texture_path(metadata->resource->GetInitData()->Path))
                  ->second.replacementData
            : g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr;
    uint32_t size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes;
    uint32_t full_image_line_size_bytes =
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes;
    SUPPORT_CHECK(full_image_line_size_bytes == line_size_bytes);

    uint32_t width = g_rdp.texture_tile[tile].line_size_bytes / 2;
    uint32_t height = (size_bytes / 2) / g_rdp.texture_tile[tile].line_size_bytes;
    gfx_rapi->upload_texture(addr, width, height);
}

static void import_texture_ia4(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? masked_textures.find(gfx_get_base_texture_path(metadata->resource->GetInitData()->Path))
                  ->second.replacementData
            : g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr;
    uint32_t size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes;
    uint32_t full_image_line_size_bytes =
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes;
    SUPPORT_CHECK(full_image_line_size_bytes == line_size_bytes);

    for (uint32_t i = 0; i < size_bytes * 2; i++) {
        uint8_t byte = addr[i / 2];
        uint8_t part = (byte >> (4 - (i % 2) * 4)) & 0xf;
        uint8_t intensity = part >> 1;
        uint8_t alpha = part & 1;
        uint8_t r = intensity;
        uint8_t g = intensity;
        uint8_t b = intensity;
        tex_upload_buffer[4 * i + 0] = SCALE_3_8(r);
        tex_upload_buffer[4 * i + 1] = SCALE_3_8(g);
        tex_upload_buffer[4 * i + 2] = SCALE_3_8(b);
        tex_upload_buffer[4 * i + 3] = alpha ? 255 : 0;
    }

    uint32_t width = g_rdp.texture_tile[tile].line_size_bytes * 2;
    uint32_t height = size_bytes / g_rdp.texture_tile[tile].line_size_bytes;

    gfx_rapi->upload_texture(tex_upload_buffer, width, height);
}

static void import_texture_ia8(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? masked_textures.find(gfx_get_base_texture_path(metadata->resource->GetInitData()->Path))
                  ->second.replacementData
            : g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr;
    uint32_t size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes;
    uint32_t full_image_line_size_bytes =
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes;
    SUPPORT_CHECK(full_image_line_size_bytes == line_size_bytes);

    for (uint32_t i = 0; i < size_bytes; i++) {
        uint8_t intensity = addr[i] >> 4;
        uint8_t alpha = addr[i] & 0xf;
        uint8_t r = intensity;
        uint8_t g = intensity;
        uint8_t b = intensity;
        tex_upload_buffer[4 * i + 0] = SCALE_4_8(r);
        tex_upload_buffer[4 * i + 1] = SCALE_4_8(g);
        tex_upload_buffer[4 * i + 2] = SCALE_4_8(b);
        tex_upload_buffer[4 * i + 3] = SCALE_4_8(alpha);
    }

    uint32_t width = g_rdp.texture_tile[tile].line_size_bytes;
    uint32_t height = size_bytes / g_rdp.texture_tile[tile].line_size_bytes;

    gfx_rapi->upload_texture(tex_upload_buffer, width, height);
}

static void import_texture_ia16(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? masked_textures.find(gfx_get_base_texture_path(metadata->resource->GetInitData()->Path))
                  ->second.replacementData
            : g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr;
    uint32_t size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes;
    uint32_t full_image_line_size_bytes =
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes;
    SUPPORT_CHECK(full_image_line_size_bytes == line_size_bytes);

    for (uint32_t i = 0; i < size_bytes / 2; i++) {
        uint8_t intensity = addr[2 * i];
        uint8_t alpha = addr[2 * i + 1];
        uint8_t r = intensity;
        uint8_t g = intensity;
        uint8_t b = intensity;
        tex_upload_buffer[4 * i + 0] = r;
        tex_upload_buffer[4 * i + 1] = g;
        tex_upload_buffer[4 * i + 2] = b;
        tex_upload_buffer[4 * i + 3] = alpha;
    }

    uint32_t width = g_rdp.texture_tile[tile].line_size_bytes / 2;
    uint32_t height = size_bytes / g_rdp.texture_tile[tile].line_size_bytes;

    gfx_rapi->upload_texture(tex_upload_buffer, width, height);
}

static void import_texture_i4(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? masked_textures.find(gfx_get_base_texture_path(metadata->resource->GetInitData()->Path))
                  ->second.replacementData
            : g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr;
    uint32_t size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes;
    uint32_t full_image_line_size_bytes =
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes;
    // SUPPORT_CHECK(full_image_line_size_bytes == line_size_bytes);

    for (uint32_t i = 0; i < size_bytes * 2; i++) {
        uint8_t byte = addr[i / 2];
        uint8_t part = (byte >> (4 - (i % 2) * 4)) & 0xf;
        uint8_t intensity = part;
        uint8_t r = intensity;
        uint8_t g = intensity;
        uint8_t b = intensity;
        uint8_t a = intensity;
        tex_upload_buffer[4 * i + 0] = SCALE_4_8(r);
        tex_upload_buffer[4 * i + 1] = SCALE_4_8(g);
        tex_upload_buffer[4 * i + 2] = SCALE_4_8(b);
        tex_upload_buffer[4 * i + 3] = SCALE_4_8(a);
    }

    uint32_t width = g_rdp.texture_tile[tile].line_size_bytes * 2;
    uint32_t height = size_bytes / g_rdp.texture_tile[tile].line_size_bytes;

    gfx_rapi->upload_texture(tex_upload_buffer, width, height);
}

static void import_texture_i8(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? masked_textures.find(gfx_get_base_texture_path(metadata->resource->GetInitData()->Path))
                  ->second.replacementData
            : g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr;
    uint32_t size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes;
    uint32_t full_image_line_size_bytes =
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes;
    // SUPPORT_CHECK(full_image_line_size_bytes == line_size_bytes);

    for (uint32_t i = 0; i < size_bytes; i++) {
        uint8_t intensity = addr[i];
        uint8_t r = intensity;
        uint8_t g = intensity;
        uint8_t b = intensity;
        uint8_t a = intensity;
        tex_upload_buffer[4 * i + 0] = r;
        tex_upload_buffer[4 * i + 1] = g;
        tex_upload_buffer[4 * i + 2] = b;
        tex_upload_buffer[4 * i + 3] = a;
    }

    uint32_t width = g_rdp.texture_tile[tile].line_size_bytes;
    uint32_t height = size_bytes / g_rdp.texture_tile[tile].line_size_bytes;

    gfx_rapi->upload_texture(tex_upload_buffer, width, height);
}

static void import_texture_ci4(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? masked_textures.find(gfx_get_base_texture_path(metadata->resource->GetInitData()->Path))
                  ->second.replacementData
            : g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr;
    uint32_t size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes;
    uint32_t full_image_line_size_bytes =
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes;
    uint32_t pal_idx = g_rdp.texture_tile[tile].palette; // 0-15

    uint8_t* palette = nullptr;
    // const uint8_t* palette = g_rdp.palettes[pal_idx / 8] + (pal_idx % 8) * 16 * 2; // 16 pixel entries, 16 bits each

    if (pal_idx > 7)
        palette = (uint8_t*)g_rdp.palettes[pal_idx / 8]; // 16 pixel entries, 16 bits each
    else
        palette = (uint8_t*)(g_rdp.palettes[pal_idx / 8] + (pal_idx % 8) * 16 * 2);

    SUPPORT_CHECK(full_image_line_size_bytes == line_size_bytes);

    for (uint32_t i = 0; i < size_bytes * 2; i++) {
        uint8_t byte = addr[i / 2];
        uint8_t idx = (byte >> (4 - (i % 2) * 4)) & 0xf;
        uint16_t col16 = (palette[idx * 2] << 8) | palette[idx * 2 + 1]; // Big endian load
        uint8_t a = col16 & 1;
        uint8_t r = col16 >> 11;
        uint8_t g = (col16 >> 6) & 0x1f;
        uint8_t b = (col16 >> 1) & 0x1f;
        tex_upload_buffer[4 * i + 0] = SCALE_5_8(r);
        tex_upload_buffer[4 * i + 1] = SCALE_5_8(g);
        tex_upload_buffer[4 * i + 2] = SCALE_5_8(b);
        tex_upload_buffer[4 * i + 3] = a ? 255 : 0;
    }

    uint32_t result_line_size = g_rdp.texture_tile[tile].line_size_bytes;
    if (metadata->h_byte_scale != 1) {
        result_line_size *= metadata->h_byte_scale;
    }

    uint32_t width = result_line_size * 2;
    uint32_t height = size_bytes / result_line_size;

    gfx_rapi->upload_texture(tex_upload_buffer, width, height);
}

static void import_texture_ci8(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? masked_textures.find(gfx_get_base_texture_path(metadata->resource->GetInitData()->Path))
                  ->second.replacementData
            : g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr;
    uint32_t size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes;
    uint32_t full_image_line_size_bytes =
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes;

    for (uint32_t i = 0, j = 0; i < size_bytes; j += full_image_line_size_bytes - line_size_bytes) {
        for (uint32_t k = 0; k < line_size_bytes; i++, k++, j++) {
            uint8_t idx = addr[j];
            uint16_t col16 = (g_rdp.palettes[idx / 128][(idx % 128) * 2] << 8) |
                             g_rdp.palettes[idx / 128][(idx % 128) * 2 + 1]; // Big endian load
            uint8_t a = col16 & 1;
            uint8_t r = col16 >> 11;
            uint8_t g = (col16 >> 6) & 0x1f;
            uint8_t b = (col16 >> 1) & 0x1f;
            tex_upload_buffer[4 * i + 0] = SCALE_5_8(r);
            tex_upload_buffer[4 * i + 1] = SCALE_5_8(g);
            tex_upload_buffer[4 * i + 2] = SCALE_5_8(b);
            tex_upload_buffer[4 * i + 3] = a ? 255 : 0;
        }
    }

    uint32_t result_line_size = g_rdp.texture_tile[tile].line_size_bytes;
    if (metadata->h_byte_scale != 1) {
        result_line_size *= metadata->h_byte_scale;
    }

    uint32_t width = result_line_size;
    uint32_t height = size_bytes / result_line_size;

    gfx_rapi->upload_texture(tex_upload_buffer, width, height);
}

static void import_texture_raw(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? masked_textures.find(gfx_get_base_texture_path(metadata->resource->GetInitData()->Path))
                  ->second.replacementData
            : g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr;

    uint16_t width = metadata->width;
    uint16_t height = metadata->height;
    LUS::TextureType type = metadata->type;
    std::shared_ptr<LUS::Texture> resource = metadata->resource;

    // if texture type is CI4 or CI8 we need to apply tlut to it
    switch (type) {
        case LUS::TextureType::Palette4bpp:
            import_texture_ci4(tile, importReplacement);
            return;
        case LUS::TextureType::Palette8bpp:
            import_texture_ci8(tile, importReplacement);
            return;
        default:
            break;
    }

    uint32_t num_loaded_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes;
    uint32_t num_originally_loaded_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].orig_size_bytes;

    uint32_t result_orig_line_size = g_rdp.texture_tile[tile].line_size_bytes;
    switch (g_rdp.texture_tile[tile].siz) {
        case G_IM_SIZ_32b:
            result_orig_line_size *= 2;
            break;
    }
    uint32_t result_orig_height = num_originally_loaded_bytes / result_orig_line_size;
    uint32_t result_new_line_size = result_orig_line_size * metadata->h_byte_scale;
    uint32_t result_new_height = result_orig_height * metadata->v_pixel_scale;

    if (result_new_line_size == 4 * width && result_new_height == height) {
        // Can use the texture directly since it has the correct dimensions
        gfx_rapi->upload_texture(addr, width, height);
        return;
    }

    uint32_t full_image_line_size_bytes =
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes;

    // Get the resource's true image size
    uint32_t resource_image_size_bytes = resource->ImageDataSize;
    uint32_t safe_full_image_line_size_bytes = full_image_line_size_bytes;
    uint32_t safe_line_size_bytes = line_size_bytes;
    uint32_t safe_loaded_bytes = num_loaded_bytes;

    // Sometimes the texture load commands will specify a size larger than the authentic texture
    // Normally the OOB info is read as garbage, but will cause a crash on some platforms
    // Restrict the bytes to a safe amount
    if (num_loaded_bytes > resource_image_size_bytes) {
        safe_loaded_bytes = resource_image_size_bytes;
        safe_line_size_bytes = resource_image_size_bytes;
        safe_full_image_line_size_bytes = resource_image_size_bytes;
    }

    // Safely only copy the amount of bytes the resource can allow
    for (uint32_t i = 0, j = 0; i < safe_loaded_bytes;
         i += safe_line_size_bytes, j += safe_full_image_line_size_bytes) {
        memcpy(tex_upload_buffer + i, addr + j, safe_line_size_bytes);
    }

    // Set the remaining bytes to load as 0
    if (num_loaded_bytes > resource_image_size_bytes) {
        memset(tex_upload_buffer + resource_image_size_bytes, 0, num_loaded_bytes - resource_image_size_bytes);
    }

    gfx_rapi->upload_texture(tex_upload_buffer, result_new_line_size / 4, result_new_height);
}

static void import_texture(int i, int tile, bool importReplacement) {
    uint8_t fmt = g_rdp.texture_tile[tile].fmt;
    uint8_t siz = g_rdp.texture_tile[tile].siz;
    uint32_t texFlags = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].tex_flags;
    uint32_t tmem_index = g_rdp.texture_tile[tile].tmem_index;
    uint8_t palette_index = g_rdp.texture_tile[tile].palette;
    uint32_t orig_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].orig_size_bytes;

    const RawTexMetadata* metadata = &g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* orig_addr =
        importReplacement && (metadata->resource != nullptr)
            ? masked_textures.find(gfx_get_base_texture_path(metadata->resource->GetInitData()->Path))
                  ->second.replacementData
            : g_rdp.loaded_texture[tmem_index].addr;

    TextureCacheKey key;
    if (fmt == G_IM_FMT_CI) {
        key = { orig_addr, { g_rdp.palettes[0], g_rdp.palettes[1] }, fmt, siz, palette_index, orig_size_bytes };
    } else {
        key = { orig_addr, {}, fmt, siz, palette_index, orig_size_bytes };
    }

    if (gfx_texture_cache_lookup(i, key)) {
        return;
    }

    // if load as raw is set then we load_raw();
    if ((texFlags & TEX_FLAG_LOAD_AS_RAW) != 0) {
        import_texture_raw(tile, importReplacement);
        return;
    }

    if (fmt == G_IM_FMT_RGBA) {
        if (siz == G_IM_SIZ_16b) {
            import_texture_rgba16(tile, importReplacement);
        } else if (siz == G_IM_SIZ_32b) {
            import_texture_rgba32(tile, importReplacement);
        } else {
            // abort(); // OTRTODO: Sometimes, seemingly randomly, we end up here. Could be a bad dlist, could be
            // something F3D does not have supported. Further investigation is needed.
        }
    } else if (fmt == G_IM_FMT_IA) {
        if (siz == G_IM_SIZ_4b) {
            import_texture_ia4(tile, importReplacement);
        } else if (siz == G_IM_SIZ_8b) {
            import_texture_ia8(tile, importReplacement);
        } else if (siz == G_IM_SIZ_16b) {
            import_texture_ia16(tile, importReplacement);
        } else {
            abort();
        }
    } else if (fmt == G_IM_FMT_CI) {
        if (siz == G_IM_SIZ_4b) {
            import_texture_ci4(tile, importReplacement);
        } else if (siz == G_IM_SIZ_8b) {
            import_texture_ci8(tile, importReplacement);
        } else {
            abort();
        }
    } else if (fmt == G_IM_FMT_I) {
        if (siz == G_IM_SIZ_4b) {
            import_texture_i4(tile, importReplacement);
        } else if (siz == G_IM_SIZ_8b) {
            import_texture_i8(tile, importReplacement);
        } else {
            abort();
        }
    } else {
        abort();
    }
}

static void import_texture_mask(int i, int tile) {
    uint32_t tmem_index = g_rdp.texture_tile[tile].tmem_index;
    RawTexMetadata metadata = g_rdp.loaded_texture[tmem_index].raw_tex_metadata;

    if (metadata.resource == nullptr) {
        return;
    }

    auto maskIter = masked_textures.find(gfx_get_base_texture_path(metadata.resource->GetInitData()->Path));
    if (maskIter == masked_textures.end()) {
        return;
    }

    const uint8_t* orig_addr = maskIter->second.mask;

    if (orig_addr == nullptr) {
        return;
    }

    TextureCacheKey key = { orig_addr, {}, 0, 0, 0, 0 };

    if (gfx_texture_cache_lookup(i, key)) {
        return;
    }

    uint32_t width = g_rdp.texture_tile[tile].line_size_bytes;
    uint32_t height = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].orig_size_bytes /
                      g_rdp.texture_tile[tile].line_size_bytes;
    switch (g_rdp.texture_tile[tile].siz) {
        case G_IM_SIZ_4b:
            width *= 2;
            break;
        case G_IM_SIZ_8b:
        default:
            break;
        case G_IM_SIZ_16b:
            width /= 2;
            break;
        case G_IM_SIZ_32b:
            width /= 4;
            break;
    }

    for (uint32_t texIndex = 0; texIndex < width * height; texIndex++) {
        uint8_t masked = orig_addr[texIndex];
        if (masked) {
            tex_upload_buffer[4 * texIndex + 0] = 0;
            tex_upload_buffer[4 * texIndex + 1] = 0;
            tex_upload_buffer[4 * texIndex + 2] = 0;
            tex_upload_buffer[4 * texIndex + 3] = 0xFF;
        } else {
            tex_upload_buffer[4 * texIndex + 0] = 0;
            tex_upload_buffer[4 * texIndex + 1] = 0;
            tex_upload_buffer[4 * texIndex + 2] = 0;
            tex_upload_buffer[4 * texIndex + 3] = 0;
        }
    }

    gfx_rapi->upload_texture(tex_upload_buffer, width, height);
}

static void gfx_normalize_vector(float v[3]) {
    float s = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= s;
    v[1] /= s;
    v[2] /= s;
}

static void gfx_transposed_matrix_mul(float res[3], const float a[3], const float b[4][4]) {
    res[0] = a[0] * b[0][0] + a[1] * b[0][1] + a[2] * b[0][2];
    res[1] = a[0] * b[1][0] + a[1] * b[1][1] + a[2] * b[1][2];
    res[2] = a[0] * b[2][0] + a[1] * b[2][1] + a[2] * b[2][2];
}

static void calculate_normal_dir(const F3DLight_t* light, float coeffs[3]) {
    float light_dir[3] = { light->dir[0] / 127.0f, light->dir[1] / 127.0f, light->dir[2] / 127.0f };

    gfx_transposed_matrix_mul(coeffs, light_dir, g_rsp.modelview_matrix_stack[g_rsp.modelview_matrix_stack_size - 1]);
    gfx_normalize_vector(coeffs);
}

static void gfx_matrix_mul(float res[4][4], const float a[4][4], const float b[4][4]) {
    float tmp[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            tmp[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
        }
    }
    memcpy(res, tmp, sizeof(tmp));
}

static void gfx_sp_matrix(uint8_t parameters, const int32_t* addr) {
    float matrix[4][4];

    if (auto it = current_mtx_replacements->find((Mtx*)addr); it != current_mtx_replacements->end()) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                float v = it->second.mf[i][j];
                int as_int = (int)(v * 65536.0f);
                matrix[i][j] = as_int * (1.0f / 65536.0f);
            }
        }
    } else {
#ifndef GBI_FLOATS
        // Original GBI where fixed point matrices are used
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j += 2) {
                int32_t int_part = addr[i * 2 + j / 2];
                uint32_t frac_part = addr[8 + i * 2 + j / 2];
                matrix[i][j] = (int32_t)((int_part & 0xffff0000) | (frac_part >> 16)) / 65536.0f;
                matrix[i][j + 1] = (int32_t)((int_part << 16) | (frac_part & 0xffff)) / 65536.0f;
            }
        }
#else
        // For a modified GBI where fixed point values are replaced with floats
        memcpy(matrix, addr, sizeof(matrix));
#endif
    }

    const auto mtx_projection = get_attr<int8_t>(MTX_PROJECTION);
    const auto mtx_load = get_attr<int8_t>(MTX_LOAD);
    const auto mtx_push = get_attr<int8_t>(MTX_PUSH);

    if (parameters & mtx_projection) {
        if (parameters & mtx_load) {
            memcpy(g_rsp.P_matrix, matrix, sizeof(matrix));
        } else {
            gfx_matrix_mul(g_rsp.P_matrix, matrix, g_rsp.P_matrix);
        }
    } else { // G_MTX_MODELVIEW
        if ((parameters & mtx_push) && g_rsp.modelview_matrix_stack_size < 11) {
            ++g_rsp.modelview_matrix_stack_size;
            memcpy(g_rsp.modelview_matrix_stack[g_rsp.modelview_matrix_stack_size - 1],
                   g_rsp.modelview_matrix_stack[g_rsp.modelview_matrix_stack_size - 2], sizeof(matrix));
        }
        if (parameters & mtx_load) {
            if (g_rsp.modelview_matrix_stack_size == 0)
                ++g_rsp.modelview_matrix_stack_size;
            memcpy(g_rsp.modelview_matrix_stack[g_rsp.modelview_matrix_stack_size - 1], matrix, sizeof(matrix));
        } else {
            gfx_matrix_mul(g_rsp.modelview_matrix_stack[g_rsp.modelview_matrix_stack_size - 1], matrix,
                           g_rsp.modelview_matrix_stack[g_rsp.modelview_matrix_stack_size - 1]);
        }
        g_rsp.lights_changed = 1;
    }
    gfx_matrix_mul(g_rsp.MP_matrix, g_rsp.modelview_matrix_stack[g_rsp.modelview_matrix_stack_size - 1],
                   g_rsp.P_matrix);
}

static void gfx_sp_pop_matrix(uint32_t count) {
    while (count--) {
        if (g_rsp.modelview_matrix_stack_size > 0) {
            --g_rsp.modelview_matrix_stack_size;
            if (g_rsp.modelview_matrix_stack_size > 0) {
                gfx_matrix_mul(g_rsp.MP_matrix, g_rsp.modelview_matrix_stack[g_rsp.modelview_matrix_stack_size - 1],
                               g_rsp.P_matrix);
            }
        }
    }
    g_rsp.lights_changed = true;
}

static float gfx_adjust_x_for_aspect_ratio(float x) {
    if (fbActive) {
        return x;
    } else {
        return x * (4.0f / 3.0f) / ((float)gfx_current_dimensions.width / (float)gfx_current_dimensions.height);
    }
}

// Scale the width and height value based on the ratio of the viewport to the native size
static void gfx_adjust_width_height_for_scale(uint32_t& width, uint32_t& height, uint32_t native_width,
                                              uint32_t native_height) {
    width = round(width * (gfx_current_dimensions.width / (2.0f * (native_width / 2))));
    height = round(height * (gfx_current_dimensions.height / (2.0f * (native_height / 2))));

    if (width == 0) {
        width = 1;
    }
    if (height == 0) {
        height = 1;
    }
}

static void gfx_sp_vertex(size_t n_vertices, size_t dest_index, const F3DVtx* vertices) {
    for (size_t i = 0; i < n_vertices; i++, dest_index++) {
        const F3DVtx_t* v = &vertices[i].v;
        const F3DVtx_tn* vn = &vertices[i].n;
        struct LoadedVertex* d = &g_rsp.loaded_vertices[dest_index];

        if (v == NULL) {
            return;
        }

        float x = v->ob[0] * g_rsp.MP_matrix[0][0] + v->ob[1] * g_rsp.MP_matrix[1][0] +
                  v->ob[2] * g_rsp.MP_matrix[2][0] + g_rsp.MP_matrix[3][0];
        float y = v->ob[0] * g_rsp.MP_matrix[0][1] + v->ob[1] * g_rsp.MP_matrix[1][1] +
                  v->ob[2] * g_rsp.MP_matrix[2][1] + g_rsp.MP_matrix[3][1];
        float z = v->ob[0] * g_rsp.MP_matrix[0][2] + v->ob[1] * g_rsp.MP_matrix[1][2] +
                  v->ob[2] * g_rsp.MP_matrix[2][2] + g_rsp.MP_matrix[3][2];
        float w = v->ob[0] * g_rsp.MP_matrix[0][3] + v->ob[1] * g_rsp.MP_matrix[1][3] +
                  v->ob[2] * g_rsp.MP_matrix[2][3] + g_rsp.MP_matrix[3][3];

        float world_pos[3];
        if (g_rsp.geometry_mode & G_LIGHTING_POSITIONAL) {
            float(*mtx)[4] = g_rsp.modelview_matrix_stack[g_rsp.modelview_matrix_stack_size - 1];
            world_pos[0] = v->ob[0] * mtx[0][0] + v->ob[1] * mtx[1][0] + v->ob[2] * mtx[2][0] + mtx[3][0];
            world_pos[1] = v->ob[0] * mtx[0][1] + v->ob[1] * mtx[1][1] + v->ob[2] * mtx[2][1] + mtx[3][1];
            world_pos[2] = v->ob[0] * mtx[0][2] + v->ob[1] * mtx[1][2] + v->ob[2] * mtx[2][2] + mtx[3][2];
        }

        x = gfx_adjust_x_for_aspect_ratio(x);

        short U = v->tc[0] * g_rsp.texture_scaling_factor.s >> 16;
        short V = v->tc[1] * g_rsp.texture_scaling_factor.t >> 16;

        if (g_rsp.geometry_mode & G_LIGHTING) {
            if (g_rsp.lights_changed) {
                for (int i = 0; i < g_rsp.current_num_lights - 1; i++) {
                    calculate_normal_dir(&g_rsp.current_lights[i].l, g_rsp.current_lights_coeffs[i]);
                }
                /*static const Light_t lookat_x = {{0, 0, 0}, 0, {0, 0, 0}, 0, {127, 0, 0}, 0};
                static const Light_t lookat_y = {{0, 0, 0}, 0, {0, 0, 0}, 0, {0, 127, 0}, 0};*/
                calculate_normal_dir(&g_rsp.lookat[0], g_rsp.current_lookat_coeffs[0]);
                calculate_normal_dir(&g_rsp.lookat[1], g_rsp.current_lookat_coeffs[1]);
                g_rsp.lights_changed = false;
            }

            int r = g_rsp.current_lights[g_rsp.current_num_lights - 1].l.col[0];
            int g = g_rsp.current_lights[g_rsp.current_num_lights - 1].l.col[1];
            int b = g_rsp.current_lights[g_rsp.current_num_lights - 1].l.col[2];

            for (int i = 0; i < g_rsp.current_num_lights - 1; i++) {
                float intensity = 0;
                if ((g_rsp.geometry_mode & G_LIGHTING_POSITIONAL) && (g_rsp.current_lights[i].p.unk3 != 0)) {
                    // Calculate distance from the light to the vertex
                    float dist_vec[3] = { g_rsp.current_lights[i].p.pos[0] - world_pos[0],
                                          g_rsp.current_lights[i].p.pos[1] - world_pos[1],
                                          g_rsp.current_lights[i].p.pos[2] - world_pos[2] };
                    float dist_sq =
                        dist_vec[0] * dist_vec[0] + dist_vec[1] * dist_vec[1] +
                        dist_vec[2] * dist_vec[2] * 2; // The *2 comes from GLideN64, unsure of why it does it
                    float dist = sqrt(dist_sq);

                    // Transform distance vector (which acts as a direction light vector) into model's space
                    float light_model[3];
                    gfx_transposed_matrix_mul(light_model, dist_vec,
                                              g_rsp.modelview_matrix_stack[g_rsp.modelview_matrix_stack_size - 1]);

                    // Calculate intensity for each axis using standard formula for intensity
                    float light_intensity[3];
                    for (int light_i = 0; light_i < 3; light_i++) {
                        light_intensity[light_i] = 4.0f * light_model[light_i] / dist_sq;
                        light_intensity[light_i] = clamp(light_intensity[light_i], -1.0f, 1.0f);
                    }

                    // Adjust intensity based on surface normal and sum up total
                    float total_intensity =
                        light_intensity[0] * vn->n[0] + light_intensity[1] * vn->n[1] + light_intensity[2] * vn->n[2];
                    total_intensity = clamp(total_intensity, -1.0f, 1.0f);

                    // Attenuate intensity based on attenuation values.
                    // Example formula found at https://ogldev.org/www/tutorial20/tutorial20.html
                    // Specific coefficients for MM's microcode sourced from GLideN64
                    // https://github.com/gonetz/GLideN64/blob/3b43a13a80dfc2eb6357673440b335e54eaa3896/src/gSP.cpp#L636
                    float distf = floorf(dist);
                    float attenuation = (distf * g_rsp.current_lights[i].p.unk7 * 2.0f +
                                         distf * distf * g_rsp.current_lights[i].p.unkE / 8.0f) /
                                            (float)0xFFFF +
                                        1.0f;
                    intensity = total_intensity / attenuation;
                } else {
                    intensity += vn->n[0] * g_rsp.current_lights_coeffs[i][0];
                    intensity += vn->n[1] * g_rsp.current_lights_coeffs[i][1];
                    intensity += vn->n[2] * g_rsp.current_lights_coeffs[i][2];
                    intensity /= 127.0f;
                }
                if (intensity > 0.0f) {
                    r += intensity * g_rsp.current_lights[i].l.col[0];
                    g += intensity * g_rsp.current_lights[i].l.col[1];
                    b += intensity * g_rsp.current_lights[i].l.col[2];
                }
            }

            d->color.r = r > 255 ? 255 : r;
            d->color.g = g > 255 ? 255 : g;
            d->color.b = b > 255 ? 255 : b;

            if (g_rsp.geometry_mode & G_TEXTURE_GEN) {
                float dotx = 0, doty = 0;
                dotx += vn->n[0] * g_rsp.current_lookat_coeffs[0][0];
                dotx += vn->n[1] * g_rsp.current_lookat_coeffs[0][1];
                dotx += vn->n[2] * g_rsp.current_lookat_coeffs[0][2];
                doty += vn->n[0] * g_rsp.current_lookat_coeffs[1][0];
                doty += vn->n[1] * g_rsp.current_lookat_coeffs[1][1];
                doty += vn->n[2] * g_rsp.current_lookat_coeffs[1][2];

                dotx /= 127.0f;
                doty /= 127.0f;

                dotx = Ship::Math::clamp(dotx, -1.0f, 1.0f);
                doty = Ship::Math::clamp(doty, -1.0f, 1.0f);

                if (g_rsp.geometry_mode & G_TEXTURE_GEN_LINEAR) {
                    // Not sure exactly what formula we should use to get accurate values
                    /*dotx = (2.906921f * dotx * dotx + 1.36114f) * dotx;
                    doty = (2.906921f * doty * doty + 1.36114f) * doty;
                    dotx = (dotx + 1.0f) / 4.0f;
                    doty = (doty + 1.0f) / 4.0f;*/
                    dotx = acosf(-dotx) /* M_PI */ / 4.0f;
                    doty = acosf(-doty) /* M_PI */ / 4.0f;
                } else {
                    dotx = (dotx + 1.0f) / 4.0f;
                    doty = (doty + 1.0f) / 4.0f;
                }

                U = (int32_t)(dotx * g_rsp.texture_scaling_factor.s);
                V = (int32_t)(doty * g_rsp.texture_scaling_factor.t);
            }
        } else {
            d->color.r = v->cn[0];
            d->color.g = v->cn[1];
            d->color.b = v->cn[2];
        }

        d->u = U;
        d->v = V;

        // trivial clip rejection
        d->clip_rej = 0;
        if (x < -w) {
            d->clip_rej |= 1; // CLIP_LEFT
        }
        if (x > w) {
            d->clip_rej |= 2; // CLIP_RIGHT
        }
        if (y < -w) {
            d->clip_rej |= 4; // CLIP_BOTTOM
        }
        if (y > w) {
            d->clip_rej |= 8; // CLIP_TOP
        }
        // if (z < -w) d->clip_rej |= 16; // CLIP_NEAR
        if (z > w) {
            d->clip_rej |= 32; // CLIP_FAR
        }

        d->x = x;
        d->y = y;
        d->z = z;
        d->w = w;

        if (g_rsp.geometry_mode & G_FOG) {
            if (fabsf(w) < 0.001f) {
                // To avoid division by zero
                w = 0.001f;
            }

            float winv = 1.0f / w;
            if (winv < 0.0f) {
                winv = std::numeric_limits<int16_t>::max();
            }

            float fog_z = z * winv * g_rsp.fog_mul + g_rsp.fog_offset;
            fog_z = Ship::Math::clamp(fog_z, 0.0f, 255.0f);
            d->color.a = fog_z; // Use alpha variable to store fog factor
        } else {
            d->color.a = v->cn[3];
        }
    }
}

static void gfx_sp_modify_vertex(uint16_t vtx_idx, uint8_t where, uint32_t val) {
    SUPPORT_CHECK(where == G_MWO_POINT_ST);

    int16_t s = (int16_t)(val >> 16);
    int16_t t = (int16_t)val;

    struct LoadedVertex* v = &g_rsp.loaded_vertices[vtx_idx];
    v->u = s;
    v->v = t;
}

static void gfx_sp_tri1(uint8_t vtx1_idx, uint8_t vtx2_idx, uint8_t vtx3_idx, bool is_rect) {
    struct LoadedVertex* v1 = &g_rsp.loaded_vertices[vtx1_idx];
    struct LoadedVertex* v2 = &g_rsp.loaded_vertices[vtx2_idx];
    struct LoadedVertex* v3 = &g_rsp.loaded_vertices[vtx3_idx];
    struct LoadedVertex* v_arr[3] = { v1, v2, v3 };

    // if (rand()%2) return;

    if (v1->clip_rej & v2->clip_rej & v3->clip_rej) {
        // The whole triangle lies outside the visible area
        return;
    }

    const auto cull_both = get_attr<uint32_t>(CULL_BOTH);
    const auto cull_front = get_attr<uint32_t>(CULL_FRONT);
    const auto cull_back = get_attr<uint32_t>(CULL_BACK);

    if ((g_rsp.geometry_mode & cull_both) != 0) {
        float dx1 = v1->x / (v1->w) - v2->x / (v2->w);
        float dy1 = v1->y / (v1->w) - v2->y / (v2->w);
        float dx2 = v3->x / (v3->w) - v2->x / (v2->w);
        float dy2 = v3->y / (v3->w) - v2->y / (v2->w);
        float cross = dx1 * dy2 - dy1 * dx2;

        if ((v1->w < 0) ^ (v2->w < 0) ^ (v3->w < 0)) {
            // If one vertex lies behind the eye, negating cross will give the correct result.
            // If all vertices lie behind the eye, the triangle will be rejected anyway.
            cross = -cross;
        }

        // If inverted culling is requested, negate the cross
        if (ucode_handler_index == UcodeHandlers::ucode_f3dex2 &&
            (g_rsp.extra_geometry_mode & G_EX_INVERT_CULLING) == 1) {
            cross = -cross;
        }

        auto cull_type = g_rsp.geometry_mode & cull_both;

        if (cull_type == cull_front) {
            if (cross <= 0) {
                return;
            }
        } else if (cull_type == cull_back) {
            if (cross >= 0) {
                return;
            }
        } else if (cull_type == cull_both) {
            // Why is this even an option?
            return;
        }
    }

    bool depth_test = (g_rsp.geometry_mode & G_ZBUFFER) == G_ZBUFFER;
    bool depth_mask = (g_rdp.other_mode_l & Z_UPD) == Z_UPD;
    uint8_t depth_test_and_mask = (depth_test ? 1 : 0) | (depth_mask ? 2 : 0);
    if (depth_test_and_mask != rendering_state.depth_test_and_mask) {
        gfx_flush();
        gfx_rapi->set_depth_test_and_mask(depth_test, depth_mask);
        rendering_state.depth_test_and_mask = depth_test_and_mask;
    }

    bool zmode_decal = (g_rdp.other_mode_l & ZMODE_DEC) == ZMODE_DEC;
    if (zmode_decal != rendering_state.decal_mode) {
        gfx_flush();
        gfx_rapi->set_zmode_decal(zmode_decal);
        rendering_state.decal_mode = zmode_decal;
    }

    if (g_rdp.viewport_or_scissor_changed) {
        if (memcmp(&g_rdp.viewport, &rendering_state.viewport, sizeof(g_rdp.viewport)) != 0) {
            gfx_flush();
            gfx_rapi->set_viewport(g_rdp.viewport.x, g_rdp.viewport.y, g_rdp.viewport.width, g_rdp.viewport.height);
            rendering_state.viewport = g_rdp.viewport;
        }
        if (memcmp(&g_rdp.scissor, &rendering_state.scissor, sizeof(g_rdp.scissor)) != 0) {
            gfx_flush();
            gfx_rapi->set_scissor(g_rdp.scissor.x, g_rdp.scissor.y, g_rdp.scissor.width, g_rdp.scissor.height);
            rendering_state.scissor = g_rdp.scissor;
        }
        g_rdp.viewport_or_scissor_changed = false;
    }

    uint64_t cc_id = g_rdp.combine_mode;
    uint64_t cc_options = 0;
    bool use_alpha = (g_rdp.other_mode_l & (3 << 20)) == (G_BL_CLR_MEM << 20) &&
                     (g_rdp.other_mode_l & (3 << 16)) == (G_BL_1MA << 16);
    bool use_fog = (g_rdp.other_mode_l >> 30) == G_BL_CLR_FOG;
    bool texture_edge = (g_rdp.other_mode_l & CVG_X_ALPHA) == CVG_X_ALPHA;
    bool use_noise = (g_rdp.other_mode_l & (3U << G_MDSFT_ALPHACOMPARE)) == G_AC_DITHER;
    bool use_2cyc = (g_rdp.other_mode_h & (3U << G_MDSFT_CYCLETYPE)) == G_CYC_2CYCLE;
    bool alpha_threshold = (g_rdp.other_mode_l & (3U << G_MDSFT_ALPHACOMPARE)) == G_AC_THRESHOLD;
    bool invisible =
        (g_rdp.other_mode_l & (3 << 24)) == (G_BL_0 << 24) && (g_rdp.other_mode_l & (3 << 20)) == (G_BL_CLR_MEM << 20);
    bool use_grayscale = g_rdp.grayscale;

    if (texture_edge) {
        use_alpha = true;
    }

    if (use_alpha) {
        cc_options |= SHADER_OPT(ALPHA);
    }
    if (use_fog) {
        cc_options |= SHADER_OPT(FOG);
    }
    if (texture_edge) {
        cc_options |= SHADER_OPT(TEXTURE_EDGE);
    }
    if (use_noise) {
        cc_options |= SHADER_OPT(NOISE);
    }
    if (use_2cyc) {
        cc_options |= SHADER_OPT(_2CYC);
    }
    if (alpha_threshold) {
        cc_options |= SHADER_OPT(ALPHA_THRESHOLD);
    }
    if (invisible) {
        cc_options |= SHADER_OPT(INVISIBLE);
    }
    if (use_grayscale) {
        cc_options |= SHADER_OPT(GRAYSCALE);
    }
    if (g_rdp.loaded_texture[0].masked) {
        cc_options |= SHADER_OPT(TEXEL0_MASK);
    }
    if (g_rdp.loaded_texture[1].masked) {
        cc_options |= SHADER_OPT(TEXEL1_MASK);
    }
    if (g_rdp.loaded_texture[0].blended) {
        cc_options |= SHADER_OPT(TEXEL0_BLEND);
    }
    if (g_rdp.loaded_texture[1].blended) {
        cc_options |= SHADER_OPT(TEXEL1_BLEND);
    }

    ColorCombinerKey key;
    key.combine_mode = g_rdp.combine_mode;
    key.options = cc_options;

    // If we are not using alpha, clear the alpha components of the combiner as they have no effect
    if (!use_alpha) {
        key.combine_mode &= ~((0xfff << 16) | ((uint64_t)0xfff << 44));
    }

    ColorCombiner* comb = gfx_lookup_or_create_color_combiner(key);

    uint32_t tm = 0;
    uint32_t tex_width[2], tex_height[2], tex_width2[2], tex_height2[2];

    for (int i = 0; i < 2; i++) {
        uint32_t tile = g_rdp.first_tile_index + i;
        if (comb->used_textures[i]) {
            if (g_rdp.textures_changed[i]) {
                gfx_flush();
                import_texture(i, tile, false);
                if (g_rdp.loaded_texture[i].masked) {
                    import_texture_mask(SHADER_FIRST_MASK_TEXTURE + i, tile);
                }
                if (g_rdp.loaded_texture[i].blended) {
                    import_texture(SHADER_FIRST_REPLACEMENT_TEXTURE + i, tile, true);
                }
                g_rdp.textures_changed[i] = false;
            }

            uint8_t cms = g_rdp.texture_tile[tile].cms;
            uint8_t cmt = g_rdp.texture_tile[tile].cmt;

            uint32_t tex_size_bytes = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].orig_size_bytes;
            uint32_t line_size = g_rdp.texture_tile[tile].line_size_bytes;

            if (line_size == 0) {
                line_size = 1;
            }

            tex_height[i] = tex_size_bytes / line_size;
            switch (g_rdp.texture_tile[tile].siz) {
                case G_IM_SIZ_4b:
                    line_size <<= 1;
                    break;
                case G_IM_SIZ_8b:
                    break;
                case G_IM_SIZ_16b:
                    line_size /= G_IM_SIZ_16b_LINE_BYTES;
                    break;
                case G_IM_SIZ_32b:
                    line_size /= G_IM_SIZ_32b_LINE_BYTES; // this is 2!
                    tex_height[i] /= 2;
                    break;
            }
            tex_width[i] = line_size;

            tex_width2[i] = (g_rdp.texture_tile[tile].lrs - g_rdp.texture_tile[tile].uls + 4) / 4;
            tex_height2[i] = (g_rdp.texture_tile[tile].lrt - g_rdp.texture_tile[tile].ult + 4) / 4;

            uint32_t tex_width1 = tex_width[i] << (cms & G_TX_MIRROR);
            uint32_t tex_height1 = tex_height[i] << (cmt & G_TX_MIRROR);

            if ((cms & G_TX_CLAMP) && ((cms & G_TX_MIRROR) || tex_width1 != tex_width2[i])) {
                tm |= 1 << 2 * i;
                cms &= ~G_TX_CLAMP;
            }
            if ((cmt & G_TX_CLAMP) && ((cmt & G_TX_MIRROR) || tex_height1 != tex_height2[i])) {
                tm |= 1 << 2 * i + 1;
                cmt &= ~G_TX_CLAMP;
            }

            bool linear_filter = (g_rdp.other_mode_h & (3U << G_MDSFT_TEXTFILT)) != G_TF_POINT;
            if (linear_filter != rendering_state.textures[i]->second.linear_filter ||
                cms != rendering_state.textures[i]->second.cms || cmt != rendering_state.textures[i]->second.cmt) {
                gfx_flush();

                // Set the same sampler params on the blended texture. Needed for opengl.
                if (g_rdp.loaded_texture[i].blended) {
                    gfx_rapi->set_sampler_parameters(SHADER_FIRST_REPLACEMENT_TEXTURE + i, linear_filter, cms, cmt);
                }

                gfx_rapi->set_sampler_parameters(i, linear_filter, cms, cmt);
                rendering_state.textures[i]->second.linear_filter = linear_filter;
                rendering_state.textures[i]->second.cms = cms;
                rendering_state.textures[i]->second.cmt = cmt;
            }
        }
    }

    struct ShaderProgram* prg = comb->prg[tm];
    if (prg == NULL) {
        comb->prg[tm] = prg =
            gfx_lookup_or_create_shader_program(comb->shader_id0, comb->shader_id1 | tm * SHADER_OPT(TEXEL0_CLAMP_S));
    }
    if (prg != rendering_state.shader_program) {
        gfx_flush();
        gfx_rapi->unload_shader(rendering_state.shader_program);
        gfx_rapi->load_shader(prg);
        rendering_state.shader_program = prg;
    }
    if (use_alpha != rendering_state.alpha_blend) {
        gfx_flush();
        gfx_rapi->set_use_alpha(use_alpha);
        rendering_state.alpha_blend = use_alpha;
    }
    uint8_t num_inputs;
    bool used_textures[2];

    gfx_rapi->shader_get_info(prg, &num_inputs, used_textures);

    struct GfxClipParameters clip_parameters = gfx_rapi->get_clip_parameters();

    for (int i = 0; i < 3; i++) {
        float z = v_arr[i]->z, w = v_arr[i]->w;
        if (clip_parameters.z_is_from_0_to_1) {
            z = (z + w) / 2.0f;
        }

        buf_vbo[buf_vbo_len++] = v_arr[i]->x;
        buf_vbo[buf_vbo_len++] = clip_parameters.invert_y ? -v_arr[i]->y : v_arr[i]->y;
        buf_vbo[buf_vbo_len++] = z;
        buf_vbo[buf_vbo_len++] = w;

        for (int t = 0; t < 2; t++) {
            if (!used_textures[t]) {
                continue;
            }
            float u = v_arr[i]->u / 32.0f;
            float v = v_arr[i]->v / 32.0f;

            int shifts = g_rdp.texture_tile[g_rdp.first_tile_index + t].shifts;
            int shiftt = g_rdp.texture_tile[g_rdp.first_tile_index + t].shiftt;
            if (shifts != 0) {
                if (shifts <= 10) {
                    u /= 1 << shifts;
                } else {
                    u *= 1 << (16 - shifts);
                }
            }
            if (shiftt != 0) {
                if (shiftt <= 10) {
                    v /= 1 << shiftt;
                } else {
                    v *= 1 << (16 - shiftt);
                }
            }

            u -= g_rdp.texture_tile[g_rdp.first_tile_index + t].uls / 4.0f;
            v -= g_rdp.texture_tile[g_rdp.first_tile_index + t].ult / 4.0f;

            if ((g_rdp.other_mode_h & (3U << G_MDSFT_TEXTFILT)) != G_TF_POINT) {
                // Linear filter adds 0.5f to the coordinates
                if (!is_rect) {
                    u += 0.5f;
                    v += 0.5f;
                }
            }

            buf_vbo[buf_vbo_len++] = u / tex_width[t];
            buf_vbo[buf_vbo_len++] = v / tex_height[t];

            bool clampS = tm & (1 << 2 * t);
            bool clampT = tm & (1 << 2 * t + 1);

            if (clampS) {
                buf_vbo[buf_vbo_len++] = (tex_width2[t] - 0.5f) / tex_width[t];
            }

            if (clampT) {
                buf_vbo[buf_vbo_len++] = (tex_height2[t] - 0.5f) / tex_height[t];
            }
        }

        if (use_fog) {
            buf_vbo[buf_vbo_len++] = g_rdp.fog_color.r / 255.0f;
            buf_vbo[buf_vbo_len++] = g_rdp.fog_color.g / 255.0f;
            buf_vbo[buf_vbo_len++] = g_rdp.fog_color.b / 255.0f;
            buf_vbo[buf_vbo_len++] = v_arr[i]->color.a / 255.0f; // fog factor (not alpha)
        }

        if (use_grayscale) {
            buf_vbo[buf_vbo_len++] = g_rdp.grayscale_color.r / 255.0f;
            buf_vbo[buf_vbo_len++] = g_rdp.grayscale_color.g / 255.0f;
            buf_vbo[buf_vbo_len++] = g_rdp.grayscale_color.b / 255.0f;
            buf_vbo[buf_vbo_len++] = g_rdp.grayscale_color.a / 255.0f; // lerp interpolation factor (not alpha)
        }

        for (int j = 0; j < num_inputs; j++) {
            struct RGBA* color = 0;
            struct RGBA tmp;
            for (int k = 0; k < 1 + (use_alpha ? 1 : 0); k++) {
                switch (comb->shader_input_mapping[k][j]) {
                        // Note: CCMUX constants and ACMUX constants used here have same value, which is why this works
                        // (except LOD fraction).
                    case G_CCMUX_PRIMITIVE:
                        color = &g_rdp.prim_color;
                        break;
                    case G_CCMUX_SHADE:
                        color = &v_arr[i]->color;
                        break;
                    case G_CCMUX_ENVIRONMENT:
                        color = &g_rdp.env_color;
                        break;
                    case G_CCMUX_PRIMITIVE_ALPHA: {
                        tmp.r = tmp.g = tmp.b = g_rdp.prim_color.a;
                        color = &tmp;
                        break;
                    }
                    case G_CCMUX_ENV_ALPHA: {
                        tmp.r = tmp.g = tmp.b = g_rdp.env_color.a;
                        color = &tmp;
                        break;
                    }
                    case G_CCMUX_PRIM_LOD_FRAC: {
                        tmp.r = tmp.g = tmp.b = g_rdp.prim_lod_fraction;
                        color = &tmp;
                        break;
                    }
                    case G_CCMUX_LOD_FRACTION: {
                        if (g_rdp.other_mode_l & G_TL_LOD) {
                            // "Hack" that works for Bowser - Peach painting
                            float distance_frac = (v1->w - 3000.0f) / 3000.0f;
                            if (distance_frac < 0.0f) {
                                distance_frac = 0.0f;
                            }
                            if (distance_frac > 1.0f) {
                                distance_frac = 1.0f;
                            }
                            tmp.r = tmp.g = tmp.b = tmp.a = distance_frac * 255.0f;
                        } else {
                            tmp.r = tmp.g = tmp.b = tmp.a = 255.0f;
                        }
                        color = &tmp;
                        break;
                    }
                    case G_ACMUX_PRIM_LOD_FRAC:
                        tmp.a = g_rdp.prim_lod_fraction;
                        color = &tmp;
                        break;
                    default:
                        memset(&tmp, 0, sizeof(tmp));
                        color = &tmp;
                        break;
                }
                if (k == 0) {
                    buf_vbo[buf_vbo_len++] = color->r / 255.0f;
                    buf_vbo[buf_vbo_len++] = color->g / 255.0f;
                    buf_vbo[buf_vbo_len++] = color->b / 255.0f;
                } else {
                    if (use_fog && color == &v_arr[i]->color) {
                        // Shade alpha is 100% for fog
                        buf_vbo[buf_vbo_len++] = 1.0f;
                    } else {
                        buf_vbo[buf_vbo_len++] = color->a / 255.0f;
                    }
                }
            }
        }

        // struct RGBA *color = &v_arr[i]->color;
        // buf_vbo[buf_vbo_len++] = color->r / 255.0f;
        // buf_vbo[buf_vbo_len++] = color->g / 255.0f;
        // buf_vbo[buf_vbo_len++] = color->b / 255.0f;
        // buf_vbo[buf_vbo_len++] = color->a / 255.0f;
    }

    if (++buf_vbo_num_tris == MAX_BUFFERED) {
        // if (++buf_vbo_num_tris == 1) {
        gfx_flush();
    }
}

static void gfx_sp_geometry_mode(uint32_t clear, uint32_t set) {
    g_rsp.geometry_mode &= ~clear;
    g_rsp.geometry_mode |= set;
}

static void gfx_sp_extra_geometry_mode(uint32_t clear, uint32_t set) {
    g_rsp.extra_geometry_mode &= ~clear;
    g_rsp.extra_geometry_mode |= set;
}

static void gfx_adjust_viewport_or_scissor(XYWidthHeight* area) {
    if (!fbActive) {
        // Adjust the y origin based on the y-inversion for the active framebuffer
        GfxClipParameters clipParameters = gfx_rapi->get_clip_parameters();
        if (clipParameters.invert_y) {
            area->y -= area->height;
        } else {
            area->y = gfx_native_dimensions.height - area->y;
        }

        area->width *= RATIO_X;
        area->height *= RATIO_Y;
        area->x *= RATIO_X;
        area->y *= RATIO_Y;

        if (!game_renders_to_framebuffer ||
            (gfx_msaa_level > 1 && gfx_current_dimensions.width == gfx_current_game_window_viewport.width &&
             gfx_current_dimensions.height == gfx_current_game_window_viewport.height)) {
            area->x += gfx_current_game_window_viewport.x;
            area->y += gfx_current_window_dimensions.height -
                       (gfx_current_game_window_viewport.y + gfx_current_game_window_viewport.height);
        }
    } else {
        area->y = active_fb->second.orig_height - area->y;

        if (active_fb->second.resize) {
            area->width *= RATIO_X;
            area->height *= RATIO_Y;
            area->x *= RATIO_X;
            area->y *= RATIO_Y;
        }
    }
}

static void gfx_calc_and_set_viewport(const F3DVp_t* viewport) {
    // 2 bits fraction
    float width = 2.0f * viewport->vscale[0] / 4.0f;
    float height = 2.0f * viewport->vscale[1] / 4.0f;
    float x = (viewport->vtrans[0] / 4.0f) - width / 2.0f;
    float y = ((viewport->vtrans[1] / 4.0f) + height / 2.0f);

    g_rdp.viewport.x = x;
    g_rdp.viewport.y = y;
    g_rdp.viewport.width = width;
    g_rdp.viewport.height = height;

    gfx_adjust_viewport_or_scissor(&g_rdp.viewport);

    g_rdp.viewport_or_scissor_changed = true;
}

static void gfx_sp_movemem_f3dex2(uint8_t index, uint8_t offset, const void* data) {
    switch (index) {
        case F3DEX2_G_MV_VIEWPORT:
            gfx_calc_and_set_viewport((const F3DVp_t*)data);
            break;
        case F3DEX2_G_MV_LIGHT: {
            int lightidx = offset / 24 - 2;
            if (lightidx >= 0 && lightidx <= MAX_LIGHTS) { // skip lookat
                // NOTE: reads out of bounds if it is an ambient light
                memcpy(g_rsp.current_lights + lightidx, data, sizeof(F3DLight));
            } else if (lightidx < 0) {
                memcpy(g_rsp.lookat + offset / 24, data, sizeof(F3DLight_t)); // TODO Light?
            }
            break;
        }
    }
}

static void gfx_sp_movemem_f3d(uint8_t index, uint8_t offset, const void* data) {
    switch (index) {
        case F3DEX_G_MV_VIEWPORT:
            gfx_calc_and_set_viewport((const F3DVp_t*)data);
            break;
        case F3DEX_G_MV_LOOKATY:
        case F3DEX_G_MV_LOOKATX:
            memcpy(g_rsp.lookat + (index - F3DEX_G_MV_LOOKATY) / 2, data, sizeof(F3DLight_t));
            break;
        case F3DEX_G_MV_L0:
        case F3DEX_G_MV_L1:
        case F3DEX_G_MV_L2:
        case F3DEX_G_MV_L3:
        case F3DEX_G_MV_L4:
        case F3DEX_G_MV_L5:
        case F3DEX_G_MV_L6:
        case F3DEX_G_MV_L7:
            // NOTE: reads out of bounds if it is an ambient light
            memcpy(g_rsp.current_lights + (index - F3DEX_G_MV_L0) / 2, data, sizeof(F3DLight_t));
            break;
    }
}

static void gfx_sp_moveword_f3dex2(uint8_t index, uint16_t offset, uintptr_t data) {
    switch (index) {
        case G_MW_NUMLIGHT:
            g_rsp.current_num_lights = data / 24 + 1; // add ambient light
            g_rsp.lights_changed = 1;
            break;
        case G_MW_FOG:
            g_rsp.fog_mul = (int16_t)(data >> 16);
            g_rsp.fog_offset = (int16_t)data;
            break;
        case G_MW_SEGMENT:
            int segNumber = offset / 4;
            gSegmentPointers[segNumber] = data;
            break;
    }
}

static void gfx_sp_moveword_f3d(uint8_t index, uint16_t offset, uintptr_t data) {
    switch (index) {
        case G_MW_NUMLIGHT:
            // Ambient light is included
            // The 31th bit is a flag that lights should be recalculated
            g_rsp.current_num_lights = (data - 0x80000000U) / 32;
            g_rsp.lights_changed = 1;
            break;
        case G_MW_FOG:
            g_rsp.fog_mul = (int16_t)(data >> 16);
            g_rsp.fog_offset = (int16_t)data;
            break;
        case G_MW_SEGMENT:
            int segNumber = offset / 4;
            gSegmentPointers[segNumber] = data;
            break;
    }
}

static void gfx_sp_texture(uint16_t sc, uint16_t tc, uint8_t level, uint8_t tile, uint8_t on) {
    g_rsp.texture_scaling_factor.s = sc;
    g_rsp.texture_scaling_factor.t = tc;
    if (g_rdp.first_tile_index != tile) {
        g_rdp.textures_changed[0] = true;
        g_rdp.textures_changed[1] = true;
    }

    g_rdp.first_tile_index = tile;
}

static void gfx_dp_set_scissor(uint32_t mode, uint32_t ulx, uint32_t uly, uint32_t lrx, uint32_t lry) {
    float x = ulx / 4.0f;
    float y = lry / 4.0f;
    float width = (lrx - ulx) / 4.0f;
    float height = (lry - uly) / 4.0f;

    g_rdp.scissor.x = x;
    g_rdp.scissor.y = y;
    g_rdp.scissor.width = width;
    g_rdp.scissor.height = height;

    gfx_adjust_viewport_or_scissor(&g_rdp.scissor);

    g_rdp.viewport_or_scissor_changed = true;
}

static void gfx_dp_set_texture_image(uint32_t format, uint32_t size, uint32_t width, const char* texPath,
                                     uint32_t texFlags, RawTexMetadata rawTexMetdata, const void* addr) {
    // fprintf(stderr, "gfx_dp_set_texture_image: %s (width=%d; size=0x%X)\n",
    //         rawTexMetdata.resource ? rawTexMetdata.resource->GetInitData()->Path.c_str() : nullptr, width, size);
    g_rdp.texture_to_load.addr = (const uint8_t*)addr;
    g_rdp.texture_to_load.siz = size;
    g_rdp.texture_to_load.width = width;
    g_rdp.texture_to_load.tex_flags = texFlags;
    g_rdp.texture_to_load.raw_tex_metadata = rawTexMetdata;
}

static void gfx_dp_set_tile(uint8_t fmt, uint32_t siz, uint32_t line, uint32_t tmem, uint8_t tile, uint32_t palette,
                            uint32_t cmt, uint32_t maskt, uint32_t shiftt, uint32_t cms, uint32_t masks,
                            uint32_t shifts) {
    // OTRTODO:
    // SUPPORT_CHECK(tmem == 0 || tmem == 256);

    if (cms == G_TX_WRAP && masks == G_TX_NOMASK) {
        cms = G_TX_CLAMP;
    }
    if (cmt == G_TX_WRAP && maskt == G_TX_NOMASK) {
        cmt = G_TX_CLAMP;
    }

    g_rdp.texture_tile[tile].palette = palette; // palette should set upper 4 bits of color index in 4b mode
    g_rdp.texture_tile[tile].fmt = fmt;
    g_rdp.texture_tile[tile].siz = siz;
    g_rdp.texture_tile[tile].cms = cms;
    g_rdp.texture_tile[tile].cmt = cmt;
    g_rdp.texture_tile[tile].shifts = shifts;
    g_rdp.texture_tile[tile].shiftt = shiftt;
    g_rdp.texture_tile[tile].line_size_bytes = line * 8;

    g_rdp.texture_tile[tile].tmem = tmem;
    // g_rdp.texture_tile[tile].tmem_index = tmem / 256; // tmem is the 64-bit word offset, so 256 words means 2 kB

    g_rdp.texture_tile[tile].tmem_index =
        tmem != 0; // assume one texture is loaded at address 0 and another texture at any other address

    g_rdp.textures_changed[0] = true;
    g_rdp.textures_changed[1] = true;
}

static void gfx_dp_set_tile_size(uint8_t tile, uint16_t uls, uint16_t ult, uint16_t lrs, uint16_t lrt) {
    g_rdp.texture_tile[tile].uls = uls;
    g_rdp.texture_tile[tile].ult = ult;
    g_rdp.texture_tile[tile].lrs = lrs;
    g_rdp.texture_tile[tile].lrt = lrt;
    g_rdp.textures_changed[0] = true;
    g_rdp.textures_changed[1] = true;
}

static void gfx_dp_load_tlut(uint8_t tile, uint32_t high_index) {
    SUPPORT_CHECK(tile == G_TX_LOADTILE);
    SUPPORT_CHECK(g_rdp.texture_to_load.siz == G_IM_SIZ_16b);
    // BENTODO
    // SUPPORT_CHECK((g_rdp.texture_tile[tile].tmem == 256 && (high_index <= 127 || high_index == 255)) ||
    //              (g_rdp.texture_tile[tile].tmem == 384 && high_index == 127));

    if (g_rdp.texture_tile[tile].tmem == 256) {
        g_rdp.palettes[0] = g_rdp.texture_to_load.addr;
        if (high_index == 255) {
            g_rdp.palettes[1] = g_rdp.texture_to_load.addr + 2 * 128;
        }
    } else {
        g_rdp.palettes[1] = g_rdp.texture_to_load.addr;
    }
}

static void gfx_dp_load_block(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t dxt) {
    SUPPORT_CHECK(tile == G_TX_LOADTILE);
    SUPPORT_CHECK(uls == 0);
    SUPPORT_CHECK(ult == 0);

    // The lrs field rather seems to be number of pixels to load
    uint32_t word_size_shift = 0;
    switch (g_rdp.texture_to_load.siz) {
        case G_IM_SIZ_4b:
            word_size_shift = -1;
            break;
        case G_IM_SIZ_8b:
            word_size_shift = 0;
            break;
        case G_IM_SIZ_16b:
            word_size_shift = 1;
            break;
        case G_IM_SIZ_32b:
            word_size_shift = 2;
            break;
    }
    uint32_t orig_size_bytes =
        word_size_shift > 0 ? (lrs + 1) << word_size_shift : (lrs + 1) >> (-(int64_t)word_size_shift);
    uint32_t size_bytes = orig_size_bytes;
    if (g_rdp.texture_to_load.raw_tex_metadata.h_byte_scale != 1 ||
        g_rdp.texture_to_load.raw_tex_metadata.v_pixel_scale != 1) {
        size_bytes *= g_rdp.texture_to_load.raw_tex_metadata.h_byte_scale;
        size_bytes *= g_rdp.texture_to_load.raw_tex_metadata.v_pixel_scale;
    }
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].orig_size_bytes = orig_size_bytes;
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes = size_bytes;
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes = size_bytes;
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes = size_bytes;
    // assert(size_bytes <= 4096 && "bug: too big texture");
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].tex_flags = g_rdp.texture_to_load.tex_flags;
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata = g_rdp.texture_to_load.raw_tex_metadata;
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr = g_rdp.texture_to_load.addr;
    // fprintf(stderr, "gfx_dp_load_block: line_size = 0x%x; orig = 0x%x; bpp=%d; lrs=%d\n", size_bytes,
    // orig_size_bytes,
    //         g_rdp.texture_to_load.siz, lrs);

    const std::string& texPath =
        g_rdp.texture_to_load.raw_tex_metadata.resource != nullptr
            ? gfx_get_base_texture_path(g_rdp.texture_to_load.raw_tex_metadata.resource->GetInitData()->Path)
            : "";
    auto maskedTextureIter = masked_textures.find(texPath);
    if (maskedTextureIter != masked_textures.end()) {
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].masked = true;
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].blended =
            maskedTextureIter->second.replacementData != nullptr;
    } else {
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].masked = false;
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].blended = false;
    }

    g_rdp.textures_changed[g_rdp.texture_tile[tile].tmem_index] = true;
}

static void gfx_dp_load_tile(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt) {
    SUPPORT_CHECK(tile == G_TX_LOADTILE);

    uint32_t word_size_shift = 0;
    switch (g_rdp.texture_to_load.siz) {
        case G_IM_SIZ_4b:
            word_size_shift = 0;
            break;
        case G_IM_SIZ_8b:
            word_size_shift = 0;
            break;
        case G_IM_SIZ_16b:
            word_size_shift = 1;
            break;
        case G_IM_SIZ_32b:
            word_size_shift = 2;
            break;
    }

    uint32_t offset_x = uls >> G_TEXTURE_IMAGE_FRAC;
    uint32_t offset_y = ult >> G_TEXTURE_IMAGE_FRAC;
    uint32_t tile_width = ((lrs - uls) >> G_TEXTURE_IMAGE_FRAC) + 1;
    uint32_t tile_height = ((lrt - ult) >> G_TEXTURE_IMAGE_FRAC) + 1;
    uint32_t full_image_width = g_rdp.texture_to_load.width + 1;

    uint32_t offset_x_in_bytes = offset_x << word_size_shift;
    uint32_t tile_line_size_bytes = tile_width << word_size_shift;
    uint32_t full_image_line_size_bytes = full_image_width << word_size_shift;

    uint32_t orig_size_bytes = tile_line_size_bytes * tile_height;
    uint32_t size_bytes = orig_size_bytes;
    uint32_t start_offset_bytes = full_image_line_size_bytes * offset_y + offset_x_in_bytes;

    float h_byte_scale = g_rdp.texture_to_load.raw_tex_metadata.h_byte_scale;
    float v_pixel_scale = g_rdp.texture_to_load.raw_tex_metadata.v_pixel_scale;

    if (h_byte_scale != 1 || v_pixel_scale != 1) {
        start_offset_bytes = h_byte_scale * (v_pixel_scale * offset_y * full_image_line_size_bytes + offset_x_in_bytes);
        size_bytes *= h_byte_scale * v_pixel_scale;
        full_image_line_size_bytes *= h_byte_scale;
        tile_line_size_bytes *= h_byte_scale;
    }

    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].orig_size_bytes = orig_size_bytes;
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].size_bytes = size_bytes;
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].full_image_line_size_bytes = full_image_line_size_bytes;
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].line_size_bytes = tile_line_size_bytes;

    //    assert(size_bytes <= 4096 && "bug: too big texture");
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].tex_flags = g_rdp.texture_to_load.tex_flags;
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].raw_tex_metadata = g_rdp.texture_to_load.raw_tex_metadata;
    g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].addr = g_rdp.texture_to_load.addr + start_offset_bytes;

    const std::string& texPath =
        g_rdp.texture_to_load.raw_tex_metadata.resource != nullptr
            ? gfx_get_base_texture_path(g_rdp.texture_to_load.raw_tex_metadata.resource->GetInitData()->Path)
            : "";
    auto maskedTextureIter = masked_textures.find(texPath);
    if (maskedTextureIter != masked_textures.end()) {
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].masked = true;
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].blended =
            maskedTextureIter->second.replacementData != nullptr;
    } else {
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].masked = false;
        g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index].blended = false;
    }

    g_rdp.texture_tile[tile].uls = uls;
    g_rdp.texture_tile[tile].ult = ult;
    g_rdp.texture_tile[tile].lrs = lrs;
    g_rdp.texture_tile[tile].lrt = lrt;

    g_rdp.textures_changed[g_rdp.texture_tile[tile].tmem_index] = true;
}

/*static uint8_t color_comb_component(uint32_t v) {
    switch (v) {
        case G_CCMUX_TEXEL0:
            return CC_TEXEL0;
        case G_CCMUX_TEXEL1:
            return CC_TEXEL1;
        case G_CCMUX_PRIMITIVE:
            return CC_PRIM;
        case G_CCMUX_SHADE:
            return CC_SHADE;
        case G_CCMUX_ENVIRONMENT:
            return CC_ENV;
        case G_CCMUX_TEXEL0_ALPHA:
            return CC_TEXEL0A;
        case G_CCMUX_LOD_FRACTION:
            return CC_LOD;
        default:
            return CC_0;
    }
}

static inline uint32_t color_comb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return color_comb_component(a) |
           (color_comb_component(b) << 3) |
           (color_comb_component(c) << 6) |
           (color_comb_component(d) << 9);
}

static void gfx_dp_set_combine_mode(uint32_t rgb, uint32_t alpha) {
    g_rdp.combine_mode = rgb | (alpha << 12);
}*/

static void gfx_dp_set_combine_mode(uint32_t rgb, uint32_t alpha, uint32_t rgb_cyc2, uint32_t alpha_cyc2) {
    g_rdp.combine_mode = rgb | (alpha << 16) | ((uint64_t)rgb_cyc2 << 28) | ((uint64_t)alpha_cyc2 << 44);
}

static inline uint32_t color_comb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 0xf) | ((b & 0xf) << 4) | ((c & 0x1f) << 8) | ((d & 7) << 13);
}

static inline uint32_t alpha_comb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 7) | ((b & 7) << 3) | ((c & 7) << 6) | ((d & 7) << 9);
}

static void gfx_dp_set_grayscale_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    g_rdp.grayscale_color.r = r;
    g_rdp.grayscale_color.g = g;
    g_rdp.grayscale_color.b = b;
    g_rdp.grayscale_color.a = a;
}

static void gfx_dp_set_env_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    g_rdp.env_color.r = r;
    g_rdp.env_color.g = g;
    g_rdp.env_color.b = b;
    g_rdp.env_color.a = a;
}

static void gfx_dp_set_prim_color(uint8_t m, uint8_t l, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    g_rdp.prim_lod_fraction = l;
    g_rdp.prim_color.r = r;
    g_rdp.prim_color.g = g;
    g_rdp.prim_color.b = b;
    g_rdp.prim_color.a = a;
}

static void gfx_dp_set_fog_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    g_rdp.fog_color.r = r;
    g_rdp.fog_color.g = g;
    g_rdp.fog_color.b = b;
    g_rdp.fog_color.a = a;
}

static void gfx_dp_set_blend_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    // TODO: Implement this command..
}

static void gfx_dp_set_fill_color(uint32_t packed_color) {
    uint16_t col16 = (uint16_t)packed_color;
    uint32_t r = col16 >> 11;
    uint32_t g = (col16 >> 6) & 0x1f;
    uint32_t b = (col16 >> 1) & 0x1f;
    uint32_t a = col16 & 1;
    g_rdp.fill_color.r = SCALE_5_8(r);
    g_rdp.fill_color.g = SCALE_5_8(g);
    g_rdp.fill_color.b = SCALE_5_8(b);
    g_rdp.fill_color.a = a * 255;
}

static void gfx_draw_rectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry) {
    uint32_t saved_other_mode_h = g_rdp.other_mode_h;
    uint32_t cycle_type = (g_rdp.other_mode_h & (3U << G_MDSFT_CYCLETYPE));

    if (cycle_type == G_CYC_COPY) {
        g_rdp.other_mode_h = (g_rdp.other_mode_h & ~(3U << G_MDSFT_TEXTFILT)) | G_TF_POINT;
    }

    // U10.2 coordinates
    float ulxf = ulx;
    float ulyf = uly;
    float lrxf = lrx;
    float lryf = lry;

    ulxf = ulxf / (4.0f * HALF_SCREEN_WIDTH) - 1.0f;
    ulyf = -(ulyf / (4.0f * HALF_SCREEN_HEIGHT)) + 1.0f;
    lrxf = lrxf / (4.0f * HALF_SCREEN_WIDTH) - 1.0f;
    lryf = -(lryf / (4.0f * HALF_SCREEN_HEIGHT)) + 1.0f;

    ulxf = gfx_adjust_x_for_aspect_ratio(ulxf);
    lrxf = gfx_adjust_x_for_aspect_ratio(lrxf);

    struct LoadedVertex* ul = &g_rsp.loaded_vertices[MAX_VERTICES + 0];
    struct LoadedVertex* ll = &g_rsp.loaded_vertices[MAX_VERTICES + 1];
    struct LoadedVertex* lr = &g_rsp.loaded_vertices[MAX_VERTICES + 2];
    struct LoadedVertex* ur = &g_rsp.loaded_vertices[MAX_VERTICES + 3];

    ul->x = ulxf;
    ul->y = ulyf;
    ul->z = -1.0f;
    ul->w = 1.0f;

    ll->x = ulxf;
    ll->y = lryf;
    ll->z = -1.0f;
    ll->w = 1.0f;

    lr->x = lrxf;
    lr->y = lryf;
    lr->z = -1.0f;
    lr->w = 1.0f;

    ur->x = lrxf;
    ur->y = ulyf;
    ur->z = -1.0f;
    ur->w = 1.0f;

    // The coordinates for texture rectangle shall bypass the viewport setting
    struct XYWidthHeight default_viewport = { 0, gfx_native_dimensions.height, gfx_native_dimensions.width,
                                              gfx_native_dimensions.height };
    struct XYWidthHeight viewport_saved = g_rdp.viewport;
    uint32_t geometry_mode_saved = g_rsp.geometry_mode;

    gfx_adjust_viewport_or_scissor(&default_viewport);

    g_rdp.viewport = default_viewport;
    g_rdp.viewport_or_scissor_changed = true;
    g_rsp.geometry_mode = 0;

    gfx_sp_tri1(MAX_VERTICES + 0, MAX_VERTICES + 1, MAX_VERTICES + 3, true);
    gfx_sp_tri1(MAX_VERTICES + 1, MAX_VERTICES + 2, MAX_VERTICES + 3, true);

    g_rsp.geometry_mode = geometry_mode_saved;
    g_rdp.viewport = viewport_saved;
    g_rdp.viewport_or_scissor_changed = true;

    if (cycle_type == G_CYC_COPY) {
        g_rdp.other_mode_h = saved_other_mode_h;
    }
}

static void gfx_dp_texture_rectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, uint8_t tile, int16_t uls,
                                     int16_t ult, int16_t dsdx, int16_t dtdy, bool flip) {
    // printf("render %d at %d\n", tile, lrx);
    uint64_t saved_combine_mode = g_rdp.combine_mode;
    if ((g_rdp.other_mode_h & (3U << G_MDSFT_CYCLETYPE)) == G_CYC_COPY) {
        // Per RDP Command Summary Set Tile's shift s and this dsdx should be set to 4 texels
        // Divide by 4 to get 1 instead
        dsdx >>= 2;

        // Color combiner is turned off in copy mode
        gfx_dp_set_combine_mode(color_comb(0, 0, 0, G_CCMUX_TEXEL0), alpha_comb(0, 0, 0, G_ACMUX_TEXEL0), 0, 0);

        // Per documentation one extra pixel is added in this modes to each edge
        lrx += 1 << 2;
        lry += 1 << 2;
    }

    // uls and ult are S10.5
    // dsdx and dtdy are S5.10
    // lrx, lry, ulx, uly are U10.2
    // lrs, lrt are S10.5
    if (flip) {
        dsdx = -dsdx;
        dtdy = -dtdy;
    }
    int16_t width = !flip ? lrx - ulx : lry - uly;
    int16_t height = !flip ? lry - uly : lrx - ulx;
    float lrs = ((uls << 7) + dsdx * width) >> 7;
    float lrt = ((ult << 7) + dtdy * height) >> 7;

    struct LoadedVertex* ul = &g_rsp.loaded_vertices[MAX_VERTICES + 0];
    struct LoadedVertex* ll = &g_rsp.loaded_vertices[MAX_VERTICES + 1];
    struct LoadedVertex* lr = &g_rsp.loaded_vertices[MAX_VERTICES + 2];
    struct LoadedVertex* ur = &g_rsp.loaded_vertices[MAX_VERTICES + 3];
    ul->u = uls;
    ul->v = ult;
    lr->u = lrs;
    lr->v = lrt;
    if (!flip) {
        ll->u = uls;
        ll->v = lrt;
        ur->u = lrs;
        ur->v = ult;
    } else {
        ll->u = lrs;
        ll->v = ult;
        ur->u = uls;
        ur->v = lrt;
    }

    uint8_t saved_tile = g_rdp.first_tile_index;
    if (saved_tile != tile) {
        g_rdp.textures_changed[0] = true;
        g_rdp.textures_changed[1] = true;
    }
    g_rdp.first_tile_index = tile;

    gfx_draw_rectangle(ulx, uly, lrx, lry);
    if (saved_tile != tile) {
        g_rdp.textures_changed[0] = true;
        g_rdp.textures_changed[1] = true;
    }
    g_rdp.first_tile_index = saved_tile;
    g_rdp.combine_mode = saved_combine_mode;
}

static void gfx_dp_image_rectangle(int32_t tile, int32_t w, int32_t h, int32_t ulx, int32_t uly, int16_t uls,
                                   int16_t ult, int32_t lrx, int32_t lry, int16_t lrs, int16_t lrt) {

    struct LoadedVertex* ul = &g_rsp.loaded_vertices[MAX_VERTICES + 0];
    struct LoadedVertex* ll = &g_rsp.loaded_vertices[MAX_VERTICES + 1];
    struct LoadedVertex* lr = &g_rsp.loaded_vertices[MAX_VERTICES + 2];
    struct LoadedVertex* ur = &g_rsp.loaded_vertices[MAX_VERTICES + 3];
    ul->u = uls * 32;
    ul->v = ult * 32;
    lr->u = lrs * 32;
    lr->v = lrt * 32;
    ll->u = uls * 32;
    ll->v = lrt * 32;
    ur->u = lrs * 32;
    ur->v = ult * 32;

    // ensure we have the correct texture size, format and starting position
    g_rdp.texture_tile[tile].siz = G_IM_SIZ_8b;
    g_rdp.texture_tile[tile].fmt = G_IM_FMT_RGBA;
    g_rdp.texture_tile[tile].cms = 0;
    g_rdp.texture_tile[tile].cmt = 0;
    g_rdp.texture_tile[tile].shifts = 0;
    g_rdp.texture_tile[tile].shiftt = 0;
    g_rdp.texture_tile[tile].uls = 0 * 4;
    g_rdp.texture_tile[tile].ult = 0 * 4;
    g_rdp.texture_tile[tile].lrs = w * 4;
    g_rdp.texture_tile[tile].lrt = h * 4;
    g_rdp.texture_tile[tile].line_size_bytes = w << (g_rdp.texture_tile[tile].siz >> 1);

    auto& loadtex = g_rdp.loaded_texture[g_rdp.texture_tile[tile].tmem_index];
    loadtex.full_image_line_size_bytes = loadtex.line_size_bytes = g_rdp.texture_tile[tile].line_size_bytes;
    loadtex.size_bytes = loadtex.orig_size_bytes = loadtex.line_size_bytes * h;

    uint8_t saved_tile = g_rdp.first_tile_index;
    if (saved_tile != tile) {
        g_rdp.textures_changed[0] = true;
        g_rdp.textures_changed[1] = true;
    }
    g_rdp.first_tile_index = tile;

    gfx_draw_rectangle(ulx, uly, lrx, lry);
    if (saved_tile != tile) {
        g_rdp.textures_changed[0] = true;
        g_rdp.textures_changed[1] = true;
    }
    g_rdp.first_tile_index = saved_tile;
}

static void gfx_dp_fill_rectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry) {
    if (g_rdp.color_image_address == g_rdp.z_buf_address) {
        // Don't clear Z buffer here since we already did it with glClear
        return;
    }
    uint32_t mode = (g_rdp.other_mode_h & (3U << G_MDSFT_CYCLETYPE));

    // OTRTODO: This is a bit of a hack for widescreen screen fades, but it'll work for now...
    if (ulx == 0 && uly == 0 && lrx == (319 * 4) && lry == (239 * 4)) {
        ulx = -1024;
        uly = -1024;
        lrx = 2048;
        lry = 2048;
    }

    if (mode == G_CYC_COPY || mode == G_CYC_FILL) {
        // Per documentation one extra pixel is added in this modes to each edge
        lrx += 1 << 2;
        lry += 1 << 2;
    }

    for (int i = MAX_VERTICES; i < MAX_VERTICES + 4; i++) {
        struct LoadedVertex* v = &g_rsp.loaded_vertices[i];
        v->color = g_rdp.fill_color;
    }

    uint64_t saved_combine_mode = g_rdp.combine_mode;

    if (mode == G_CYC_FILL) {
        gfx_dp_set_combine_mode(color_comb(0, 0, 0, G_CCMUX_SHADE), alpha_comb(0, 0, 0, G_ACMUX_SHADE), 0, 0);
    }

    gfx_draw_rectangle(ulx, uly, lrx, lry);
    g_rdp.combine_mode = saved_combine_mode;
}

static void gfx_dp_set_z_image(void* z_buf_address) {
    g_rdp.z_buf_address = z_buf_address;
}

static void gfx_dp_set_color_image(uint32_t format, uint32_t size, uint32_t width, void* address) {
    g_rdp.color_image_address = address;
}

static void gfx_sp_set_other_mode(uint32_t shift, uint32_t num_bits, uint64_t mode) {
    uint64_t mask = (((uint64_t)1 << num_bits) - 1) << shift;
    uint64_t om = g_rdp.other_mode_l | ((uint64_t)g_rdp.other_mode_h << 32);
    om = (om & ~mask) | mode;
    g_rdp.other_mode_l = (uint32_t)om;
    g_rdp.other_mode_h = (uint32_t)(om >> 32);
}

static void gfx_dp_set_other_mode(uint32_t h, uint32_t l) {
    g_rdp.other_mode_h = h;
    g_rdp.other_mode_l = l;
}

static void gfx_s2dex_bg_copy(F3DuObjBg* bg) {
    /*
    bg->b.imageX = 0;
    bg->b.imageW = width * 4;
    bg->b.frameX = frameX * 4;
    bg->b.imageY = 0;
    bg->b.imageH = height * 4;
    bg->b.frameY = frameY * 4;
    bg->b.imagePtr = source;
    bg->b.imageLoad = G_BGLT_LOADTILE;
    bg->b.imageFmt = fmt;
    bg->b.imageSiz = siz;
    bg->b.imagePal = 0;
    bg->b.imageFlip = 0;
    */

    uintptr_t data = (uintptr_t)bg->b.imagePtr;

    uint32_t texFlags = 0;
    RawTexMetadata rawTexMetadata = {};

    if ((bool)gfx_check_image_signature((char*)data)) {
        std::shared_ptr<LUS::Texture> tex = std::static_pointer_cast<LUS::Texture>(
            Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess((char*)data));
        texFlags = tex->Flags;
        rawTexMetadata.width = tex->Width;
        rawTexMetadata.height = tex->Height;
        rawTexMetadata.h_byte_scale = tex->HByteScale;
        rawTexMetadata.v_pixel_scale = tex->VPixelScale;
        rawTexMetadata.type = tex->Type;
        rawTexMetadata.resource = tex;
        data = (uintptr_t) reinterpret_cast<char*>(tex->ImageData);
    }

    s16 dsdx = 4 << 10;
    s16 uls = bg->b.imageX << 3;
    // Flip flag only flips horizontally
    if (bg->b.imageFlip == G_BG_FLAG_FLIPS) {
        dsdx = -dsdx;
        uls = (bg->b.imageW - bg->b.imageX) << 3;
    }

    SUPPORT_CHECK(bg->b.imageSiz == G_IM_SIZ_16b);
    gfx_dp_set_texture_image(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, nullptr, texFlags, rawTexMetadata, (void*)data);
    gfx_dp_set_tile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    gfx_dp_load_block(G_TX_LOADTILE, 0, 0, (bg->b.imageW * bg->b.imageH >> 4) - 1, 0);
    gfx_dp_set_tile(bg->b.imageFmt, G_IM_SIZ_16b, bg->b.imageW >> 4, 0, G_TX_RENDERTILE, bg->b.imagePal, 0, 0, 0, 0, 0,
                    0);
    gfx_dp_set_tile_size(G_TX_RENDERTILE, 0, 0, bg->b.imageW, bg->b.imageH);
    gfx_dp_texture_rectangle(bg->b.frameX, bg->b.frameY, bg->b.frameX + bg->b.imageW - 4,
                             bg->b.frameY + bg->b.imageH - 4, G_TX_RENDERTILE, uls, bg->b.imageY << 3, dsdx, 1 << 10,
                             false);
}

static void gfx_s2dex_bg_1cyc(F3DuObjBg* bg) {
    uintptr_t data = (uintptr_t)bg->b.imagePtr;

    uint32_t texFlags = 0;
    RawTexMetadata rawTexMetadata = {};

    if ((bool)gfx_check_image_signature((char*)data)) {
        std::shared_ptr<LUS::Texture> tex = std::static_pointer_cast<LUS::Texture>(
            Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess((char*)data));
        texFlags = tex->Flags;
        rawTexMetadata.width = tex->Width;
        rawTexMetadata.height = tex->Height;
        rawTexMetadata.h_byte_scale = tex->HByteScale;
        rawTexMetadata.v_pixel_scale = tex->VPixelScale;
        rawTexMetadata.type = tex->Type;
        rawTexMetadata.resource = tex;
        data = (uintptr_t) reinterpret_cast<char*>(tex->ImageData);
    }

    // TODO: Implement bg scaling correctly
    s16 uls = bg->b.imageX >> 2;
    s16 lrs = bg->b.imageW >> 2;

    s16 dsdxRect = 1 << 10;
    s16 ulsRect = bg->b.imageX << 3;
    // Flip flag only flips horizontally
    if (bg->b.imageFlip == G_BG_FLAG_FLIPS) {
        dsdxRect = -dsdxRect;
        ulsRect = (bg->b.imageW - bg->b.imageX) << 3;
    }

    gfx_dp_set_texture_image(bg->b.imageFmt, bg->b.imageSiz, bg->b.imageW >> 2, nullptr, texFlags, rawTexMetadata,
                             (void*)data);
    gfx_dp_set_tile(bg->b.imageFmt, bg->b.imageSiz, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    gfx_dp_load_block(G_TX_LOADTILE, 0, 0, (bg->b.imageW * bg->b.imageH >> 4) - 1, 0);
    gfx_dp_set_tile(bg->b.imageFmt, bg->b.imageSiz, (((lrs - uls) * bg->b.imageSiz) + 7) >> 3, 0, G_TX_RENDERTILE,
                    bg->b.imagePal, 0, 0, 0, 0, 0, 0);
    gfx_dp_set_tile_size(G_TX_RENDERTILE, 0, 0, bg->b.imageW, bg->b.imageH);

    gfx_dp_texture_rectangle(bg->b.frameX, bg->b.frameY, bg->b.frameW, bg->b.frameH, G_TX_RENDERTILE, ulsRect,
                             bg->b.imageY << 3, dsdxRect, 1 << 10, false);
}

static void gfx_s2dex_rect_copy(F3DuObjSprite* spr) {
    s16 dsdx = 4 << 10;
    s16 uls = spr->s.objX << 3;
    // Flip flag only flips horizontally
    if (spr->s.imageFlags == G_BG_FLAG_FLIPS) {
        dsdx = -dsdx;
        uls = (spr->s.imageW - spr->s.objX) << 3;
    }

    int realX = spr->s.objX >> 2;
    int realY = spr->s.objY >> 2;
    int realW = (((spr->s.imageW)) >> 5);
    int realH = (((spr->s.imageH)) >> 5);
    float realSW = spr->s.scaleW / 1024.0f;
    float realSH = spr->s.scaleH / 1024.0f;

    int testX = (realX + (realW / realSW));
    int testY = (realY + (realH / realSH));

    gfx_dp_texture_rectangle(realX << 2, realY << 2, testX << 2, testY << 2, G_TX_RENDERTILE,
                             g_rdp.texture_tile[0].uls << 3, g_rdp.texture_tile[0].ult << 3, (float)(1 << 10) * realSW,
                             (float)(1 << 10) * realSH, false);
}

static inline void* seg_addr(uintptr_t w1) {
    // Segmented?
    if (w1 & 1) {
        uint32_t segNum = (uint32_t)(w1 >> 24);

        uint32_t offset = w1 & 0x00FFFFFE;

        if (gSegmentPointers[segNum] != 0) {
            return (void*)(gSegmentPointers[segNum] + offset);
        } else {
            return (void*)w1;
        }
    } else {
        return (void*)w1;
    }
}

#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << width) - 1))
#define C1(pos, width) ((cmd->words.w1 >> (pos)) & ((1U << width) - 1))

uintptr_t clearMtx;

void GfxExecStack::start(F3DGfx* dlist) {
    while (!cmd_stack.empty())
        cmd_stack.pop();
    gfx_path.clear();
    cmd_stack.push(dlist);
    disp_stack.clear();
}

void GfxExecStack::stop() {
    while (!cmd_stack.empty())
        cmd_stack.pop();
    gfx_path.clear();
}

F3DGfx*& GfxExecStack::currCmd() {
    return cmd_stack.top();
}

void GfxExecStack::openDisp(const char* file, int line) {
    disp_stack.push_back({ file, line });
}
void GfxExecStack::closeDisp() {
    disp_stack.pop_back();
}
const std::vector<GfxExecStack::CodeDisp>& GfxExecStack::getDisp() const {
    return disp_stack;
}

void GfxExecStack::branch(F3DGfx* caller) {
    F3DGfx* old = cmd_stack.top();
    cmd_stack.pop();
    cmd_stack.push(nullptr);
    cmd_stack.push(old);

    gfx_path.push_back(caller);
}

void GfxExecStack::call(F3DGfx* caller, F3DGfx* callee) {
    cmd_stack.push(callee);
    gfx_path.push_back(caller);
}

F3DGfx* GfxExecStack::ret() {
    F3DGfx* cmd = cmd_stack.top();

    cmd_stack.pop();
    if (!gfx_path.empty()) {
        gfx_path.pop_back();
    }

    while (cmd_stack.size() > 0 && cmd_stack.top() == nullptr) {
        cmd_stack.pop();
        if (!gfx_path.empty()) {
            gfx_path.pop_back();
        }
    }
    return cmd;
}

void gfx_set_framebuffer(int fb, float noise_scale);
void gfx_reset_framebuffer();
void gfx_copy_framebuffer(int fb_dst_id, int fb_src_id, bool copyOnce, bool* hasCopiedPtr);

// The main type of the handler function. These function will take a pointer to a pointer to a Gfx. It needs to be a
// double pointer because we sometimes need to increment and decrement the underlying pointer Returns false if the
// current opcode should be incremented after the handler ends.
typedef bool (*GfxOpcodeHandlerFunc)(F3DGfx** gfx);

bool gfx_load_ucode_handler_f3dex2(F3DGfx** cmd) {
    g_rsp.fog_mul = 0;
    g_rsp.fog_offset = 0;
    return false;
}

bool gfx_cull_dl_handler_f3dex2(F3DGfx** cmd) {
    // TODO:
    return false;
}

bool gfx_marker_handler_otr(F3DGfx** cmd0) {
    (*cmd0)++;
    F3DGfx* cmd = (*cmd0);
    const uint64_t hash = ((uint64_t)(cmd)->words.w0 << 32) + (cmd)->words.w1;
    std::string dlName = ResourceGetNameByCrc(hash);
    markerOn = true;
    return false;
}

bool gfx_invalidate_tex_cache_handler_f3dex2(F3DGfx** cmd) {
    const uintptr_t texAddr = (*cmd)->words.w1;

    if (texAddr == 0) {
        gfx_texture_cache_clear();
    } else {
        gfx_texture_cache_delete((const uint8_t*)texAddr);
    }
    return false;
}

bool gfx_noop_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    const char* filename = (const char*)(cmd)->words.w1;
    uint32_t p = C0(16, 8);
    uint32_t l = C0(0, 16);
    if (p == 7) {
        g_exec_stack.openDisp(filename, l);
    } else if (p == 8) {
        if (g_exec_stack.disp_stack.size() == 0) {
            SPDLOG_WARN("CLOSE_DISPS without matching open {}:{}", p, l);
        } else {
            g_exec_stack.closeDisp();
        }
    }
    return false;
}

bool gfx_mtx_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    uintptr_t mtxAddr = cmd->words.w1;

    if (mtxAddr == SEG_ADDR(0, 0x12DB20) || // GC MQ D
        mtxAddr == SEG_ADDR(0, 0x12DB40) || // GC NMQ D
        mtxAddr == SEG_ADDR(0, 0xFBC20) ||  // GC PAL
        mtxAddr == SEG_ADDR(0, 0xFBC01) ||  // GC MQ PAL
        mtxAddr == SEG_ADDR(0, 0xFCD00) ||  // PAL1.0
        mtxAddr == SEG_ADDR(0, 0xFCD40)     // PAL1.1
    ) {
        mtxAddr = clearMtx;
    }

    gfx_sp_matrix(C0(0, 8) ^ F3DEX2_G_MTX_PUSH, (const int32_t*)seg_addr(mtxAddr));

    return false;
}
// Seems to be the same for all other non F3DEX2 microcodes...
bool gfx_mtx_handler_f3d(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    uintptr_t mtxAddr = cmd->words.w1;

    if (mtxAddr == SEG_ADDR(0, 0x12DB20) || // GC MQ D
        mtxAddr == SEG_ADDR(0, 0x12DB40) || // GC NMQ D
        mtxAddr == SEG_ADDR(0, 0xFBC20) ||  // GC PAL
        mtxAddr == SEG_ADDR(0, 0xFBC01) ||  // GC MQ PAL
        mtxAddr == SEG_ADDR(0, 0xFCD00) ||  // PAL1.0
        mtxAddr == SEG_ADDR(0, 0xFCD40)     // PAL1.1
    ) {
        mtxAddr = clearMtx;
    }

    gfx_sp_matrix(C0(16, 8), (const int32_t*)seg_addr(cmd->words.w1));
    return false;
}

bool gfx_mtx_otr_handler_custom_f3dex2(F3DGfx** cmd0) {
    (*cmd0)++;
    F3DGfx* cmd = *cmd0;

    const uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;

    const int32_t* mtx = (const int32_t*)ResourceGetDataByCrc(hash);

    if (mtx != NULL) {
        cmd--;
        gfx_sp_matrix(C0(0, 8) ^ F3DEX2_G_MTX_PUSH, mtx);
        cmd++;
    }

    return false;
}

bool gfx_mtx_otr_handler_custom_f3d(F3DGfx** cmd0) {
    (*cmd0)++;
    F3DGfx* cmd = *cmd0;

    const uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;

    const int32_t* mtx = (const int32_t*)ResourceGetDataByCrc(hash);

    gfx_sp_matrix(C0(16, 8), (const int32_t*)seg_addr(cmd->words.w1));

    return false;
}

bool gfx_pop_mtx_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_pop_matrix((uint32_t)(cmd->words.w1 / 64));

    return false;
}

bool gfx_pop_mtx_handler_f3d(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_pop_matrix(1);

    return false;
}

bool gfx_movemem_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_movemem_f3dex2(C0(0, 8), C0(8, 8) * 8, seg_addr(cmd->words.w1));

    return false;
}

bool gfx_movemem_handler_f3d(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_movemem_f3d(C0(16, 8), 0, seg_addr(cmd->words.w1));

    return false;
}

bool gfx_moveword_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_moveword_f3dex2(C0(16, 8), C0(0, 16), cmd->words.w1);

    return false;
}

bool gfx_moveword_handler_f3d(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_moveword_f3d(C0(0, 8), C0(8, 16), cmd->words.w1);

    return false;
}

bool gfx_texture_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_texture(C1(16, 16), C1(0, 16), C0(11, 3), C0(8, 3), C0(1, 7));

    return false;
}

// Seems to be the same for all other non F3DEX2 microcodes...
bool gfx_texture_handler_f3d(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_texture(C1(16, 16), C1(0, 16), C0(11, 3), C0(8, 3), C0(0, 8));

    return false;
}

// Almost all versions of the microcode have their own version of this opcode
bool gfx_vtx_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_vertex(C0(12, 8), C0(1, 7) - C0(12, 8), (const F3DVtx*)seg_addr(cmd->words.w1));

    return false;
}

bool gfx_vtx_handler_f3dex(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    gfx_sp_vertex(C0(10, 6), C0(17, 7), (const F3DVtx*)seg_addr(cmd->words.w1));

    return false;
}

bool gfx_vtx_handler_f3d(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_vertex((C0(0, 16)) / sizeof(F3DVtx), C0(16, 4), (const F3DVtx*)seg_addr(cmd->words.w1));

    return false;
}

bool gfx_vtx_hash_handler_custom(F3DGfx** cmd0) {
    // Offset added to the start of the vertices
    const uintptr_t offset = (*cmd0)->words.w1;
    // This is a two-part display list command, so increment the instruction pointer so we can get the CRC64
    // hash from the second
    (*cmd0)++;
    const uint64_t hash = ((uint64_t)(*cmd0)->words.w0 << 32) + (*cmd0)->words.w1;

    // We need to know if the offset is a cached pointer or not. An offset greater than one million is not a
    // real offset, so it must be a real pointer
    if (offset > 0xFFFFF) {
        (*cmd0)--;
        F3DGfx* cmd = *cmd0;
        gfx_sp_vertex(C0(12, 8), C0(1, 7) - C0(12, 8), (F3DVtx*)offset);
        (*cmd0)++;
    } else {
        F3DVtx* vtx = (F3DVtx*)ResourceGetDataByCrc(hash);

        if (vtx != NULL) {
            vtx = (F3DVtx*)((char*)vtx + offset);

            (*cmd0)--;
            F3DGfx* cmd = *cmd0;

            // TODO: WTF??
            cmd->words.w1 = (uintptr_t)vtx;

            gfx_sp_vertex(C0(12, 8), C0(1, 7) - C0(12, 8), vtx);
            (*cmd0)++;
        }
    }
    return false;
}

bool gfx_vtx_otr_filepath_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    char* fileName = (char*)cmd->words.w1;
    cmd++;
    size_t vtxCnt = cmd->words.w0;
    size_t vtxIdxOff = cmd->words.w1 >> 16;
    size_t vtxDataOff = cmd->words.w1 & 0xFFFF;
    F3DVtx* vtx = (F3DVtx*)ResourceGetDataByName((const char*)fileName);
    vtx += vtxDataOff;
    cmd--;

    gfx_sp_vertex(vtxCnt, vtxIdxOff, vtx);
    return false;
}

bool gfx_dl_otr_filepath_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    char* fileName = (char*)cmd->words.w1;
    F3DGfx* nDL = (F3DGfx*)ResourceGetDataByName((const char*)fileName);

    if (C0(16, 1) == 0 && nDL != nullptr) {
        g_exec_stack.call(*cmd0, nDL);
    } else {
        if (nDL != nullptr) {
            (*cmd0) = nDL;
            g_exec_stack.branch(cmd);
            return true; // shortcut cmd increment
        } else {
            assert(0 && "???");
            // gfx_path.pop_back();
            // cmd = cmd_stack.top();
            // cmd_stack.pop();
        }
    }
    return false;
}

// The original F3D microcode doesn't seem to have this opcode. Glide handles it as part of moveword
bool gfx_modify_vtx_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    gfx_sp_modify_vertex(C0(1, 15), C0(16, 8), (uint32_t)cmd->words.w1);
    return false;
}

// F3D, F3DEX, and F3DEX2 do the same thing but F3DEX2 has its own opcode number
bool gfx_dl_handler_common(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    F3DGfx* subGFX = (F3DGfx*)seg_addr(cmd->words.w1);
    if (C0(16, 1) == 0) {
        // Push return address
        if (subGFX != nullptr) {
            g_exec_stack.call(*cmd0, subGFX);
        }
    } else {
        (*cmd0) = subGFX;
        g_exec_stack.branch(cmd);
        return true; // shortcut cmd increment
    }
    return false;
}

bool gfx_dl_otr_hash_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    if (C0(16, 1) == 0) {
        // Push return address
        (*cmd0)++;

        uint64_t hash = ((uint64_t)(*cmd0)->words.w0 << 32) + (*cmd0)->words.w1;

        F3DGfx* gfx = (F3DGfx*)ResourceGetDataByCrc(hash);

        if (gfx != 0) {
            g_exec_stack.call(cmd, gfx);
        }
    } else {
        assert(0 && "????");
        (*cmd0) = (F3DGfx*)seg_addr((*cmd0)->words.w1);
        return true;
    }
    return false;
}
bool gfx_dl_index_handler(F3DGfx** cmd0) {
    // Compute seg addr by converting an index value to a offset value
    // handling 32 vs 64 bit size differences for Gfx
    // adding 1 to trigger the segaddr flow
    F3DGfx* cmd = (*cmd0);
    uint8_t segNum = (uint8_t)(cmd->words.w1 >> 24);
    uint32_t index = (uint32_t)(cmd->words.w1 & 0x00FFFFFF);
    uintptr_t segAddr = (segNum << 24) | (index * sizeof(F3DGfx)) + 1;

    F3DGfx* subGFX = (F3DGfx*)seg_addr(segAddr);
    if (C0(16, 1) == 0) {
        // Push return address
        if (subGFX != nullptr) {
            g_exec_stack.call((*cmd0), subGFX);
        }
    } else {
        (*cmd0) = subGFX;
        g_exec_stack.branch(cmd);
        return true; // shortcut cmd increment
    }
    return false;
}

// TODO handle special OTR opcodes later...
bool gfx_pushcd_handler_custom(F3DGfx** cmd0) {
    gfx_push_current_dir((char*)(*cmd0)->words.w1);
    return false;
}

// TODO handle special OTR opcodes later...
bool gfx_branch_z_otr_handler_f3dex2(F3DGfx** cmd0) {
    // Push return address
    F3DGfx* cmd = (*cmd0);

    uint8_t vbidx = (uint8_t)((*cmd0)->words.w0 & 0x00000FFF);
    uint32_t zval = (uint32_t)((*cmd0)->words.w1);

    (*cmd0)++;

    if (g_rsp.loaded_vertices[vbidx].z <= zval || (g_rsp.extra_geometry_mode & G_EX_ALWAYS_EXECUTE_BRANCH) != 0) {
        uint64_t hash = ((uint64_t)(*cmd0)->words.w0 << 32) + (*cmd0)->words.w1;

        F3DGfx* gfx = (F3DGfx*)ResourceGetDataByCrc(hash);

        if (gfx != 0) {
            (*cmd0) = gfx;
            g_exec_stack.branch(cmd);
            return true; // shortcut cmd increment
        }
    }
    return false;
}

// F3D, F3DEX, and F3DEX2 do the same thing but F3DEX2 has its own opcode number
bool gfx_end_dl_handler_common(F3DGfx** cmd0) {
    markerOn = false;
    *cmd0 = g_exec_stack.ret();
    return true;
}

bool gfx_set_prim_depth_handler_rdp(F3DGfx** cmd) {
    // TODO Implement this command...
    return false;
}

// Only on F3DEX2
bool gfx_geometry_mode_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    gfx_sp_geometry_mode(~C0(0, 24), (uint32_t)cmd->words.w1);
    return false;
}

// Only on F3DEX and older
bool gfx_set_geometry_mode_handler_f3d(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    gfx_sp_geometry_mode(0, (uint32_t)cmd->words.w1);
    return false;
}

// Only on F3DEX and older
bool gfx_clear_geometry_mode_handler_f3d(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    gfx_sp_geometry_mode((uint32_t)cmd->words.w1, 0);
    return false;
}

bool gfx_tri1_otr_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    uint8_t v00 = (uint8_t)(cmd->words.w0 & 0x0000FFFF);
    uint8_t v01 = (uint8_t)(cmd->words.w1 >> 16);
    uint8_t v02 = (uint8_t)(cmd->words.w1 & 0x0000FFFF);
    gfx_sp_tri1(v00, v01, v02, false);

    return false;
}

bool gfx_tri1_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_tri1(C0(16, 8) / 2, C0(8, 8) / 2, C0(0, 8) / 2, false);

    return false;
}

bool gfx_tri1_handler_f3dex(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_tri1(C1(17, 7), C1(9, 7), C1(1, 7), false);

    return false;
}

bool gfx_tri1_handler_f3d(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_tri1(C1(16, 8) / 10, C1(8, 8) / 10, C1(0, 8) / 10, false);

    return false;
}

// F3DEX, and F3DEX2 share a tri2 function, however F3DEX has a different quad function.
bool gfx_tri2_handler_f3dex(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_tri1(C0(17, 7), C0(9, 7), C0(1, 7), false);
    gfx_sp_tri1(C1(17, 7), C1(9, 7), C1(1, 7), false);
    return false;
}

bool gfx_quad_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_tri1(C0(16, 8) / 2, C0(8, 8) / 2, C0(0, 8) / 2, false);
    gfx_sp_tri1(C1(16, 8) / 2, C1(8, 8) / 2, C1(0, 8) / 2, false);
    return false;
}

bool gfx_quad_handler_f3dex(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    gfx_sp_tri1(C0(16, 8) / 2, C0(8, 8) / 2, C0(0, 8) / 2, false);
    gfx_sp_tri1(C0(8, 8) / 2, C0(0, 8) / 2, C0(24, 0) / 2, false);
    gfx_sp_tri1(C1(0, 8) / 2, C1(24, 8) / 2, C1(16, 8) / 2, false);
    gfx_sp_tri1(C1(24, 8) / 2, C1(16, 8) / 2, C1(8, 8) / 2, false);
    return false;
}

bool gfx_othermode_l_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_set_other_mode(31 - C0(8, 8) - C0(0, 8), C0(0, 8) + 1, cmd->words.w1);

    return false;
}

bool gfx_othermode_l_handler_f3d(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_set_other_mode(C0(8, 8), C0(0, 8), cmd->words.w1);

    return false;
}

bool gfx_othermode_h_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_set_other_mode(63 - C0(8, 8) - C0(0, 8), C0(0, 8) + 1, (uint64_t)cmd->words.w1 << 32);

    return false;
}

// Only on F3DEX and older
bool gfx_set_geometry_mode_handler_f3dex(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    gfx_sp_geometry_mode(0, (uint32_t)cmd->words.w1);
    return false;
}

// Only on F3DEX and older
bool gfx_clear_geometry_mode_handler_f3dex(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    gfx_sp_geometry_mode((uint32_t)cmd->words.w1, 0);
    return false;
}

bool gfx_othermode_h_handler_f3d(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_sp_set_other_mode(C0(8, 8) + 32, C0(0, 8), (uint64_t)cmd->words.w1 << 32);

    return false;
}

bool gfx_set_timg_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    uintptr_t i = (uintptr_t)seg_addr(cmd->words.w1);

    char* imgData = (char*)i;
    uint32_t texFlags = 0;
    RawTexMetadata rawTexMetdata = {};

    if ((i & 1) != 1) {
        if (gfx_check_image_signature(imgData) == 1) {
            std::shared_ptr<LUS::Texture> tex = std::static_pointer_cast<LUS::Texture>(
                Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess(imgData));

            i = (uintptr_t) reinterpret_cast<char*>(tex->ImageData);
            texFlags = tex->Flags;
            rawTexMetdata.width = tex->Width;
            rawTexMetdata.height = tex->Height;
            rawTexMetdata.h_byte_scale = tex->HByteScale;
            rawTexMetdata.v_pixel_scale = tex->VPixelScale;
            rawTexMetdata.type = tex->Type;
            rawTexMetdata.resource = tex;
        }
    }

    gfx_dp_set_texture_image(C0(21, 3), C0(19, 2), C0(0, 10), imgData, texFlags, rawTexMetdata, (void*)i);

    return false;
}

bool gfx_set_timg_otr_hash_handler_custom(F3DGfx** cmd0) {
    uintptr_t addr = (*cmd0)->words.w1;
    (*cmd0)++;
    uint64_t hash = ((uint64_t)(*cmd0)->words.w0 << 32) + (uint64_t)(*cmd0)->words.w1;

    const char* fileName = ResourceGetNameByCrc(hash);
    uint32_t texFlags = 0;
    RawTexMetadata rawTexMetadata = {};

    if (fileName == nullptr) {
        (*cmd0)++;
        return false;
    }

    std::shared_ptr<LUS::Texture> texture = std::static_pointer_cast<LUS::Texture>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess(ResourceGetNameByCrc(hash)));
    if (texture != nullptr) {
        texFlags = texture->Flags;
        rawTexMetadata.width = texture->Width;
        rawTexMetadata.height = texture->Height;
        rawTexMetadata.h_byte_scale = texture->HByteScale;
        rawTexMetadata.v_pixel_scale = texture->VPixelScale;
        rawTexMetadata.type = texture->Type;
        rawTexMetadata.resource = texture;

        // #if _DEBUG && 0
        // tex = reinterpret_cast<char*>(texture->imageData);
        // ResourceMgr_GetNameByCRC(hash, fileName);
        // fprintf(stderr, "G_SETTIMG_OTR_HASH: %s (%dx%d) size=0x%x, %08X\n", fileName, texture->Width,
        //         texture->Height, texture->ImageDataSize, hash);
        // #else
        char* tex = NULL;
        // #endif

        // OTRTODO: We have disabled caching for now to fix a texture corruption issue with HD texture
        // support. In doing so, there is a potential performance hit since we are not caching lookups. We
        // need to do proper profiling to see whether or not it is worth it to keep the caching system.

        tex = reinterpret_cast<char*>(texture->ImageData);

        if (tex != nullptr) {
            (*cmd0)--;
            uintptr_t oldData = (*cmd0)->words.w1;
            // TODO: wtf??
            (*cmd0)->words.w1 = (uintptr_t)tex;

            // if (ourHash != (uint64_t)-1) {
            //     auto res = ResourceLoad(ourHash);
            // }

            (*cmd0)++;
        }

        (*cmd0)--;
        F3DGfx* cmd = (*cmd0);
        uint32_t fmt = C0(21, 3);
        uint32_t size = C0(19, 2);
        uint32_t width = C0(0, 10);

        if (tex != NULL) {
            gfx_dp_set_texture_image(fmt, size, width, fileName, texFlags, rawTexMetadata, tex);
        }
    } else {
        SPDLOG_ERROR("G_SETTIMG_OTR_HASH: Texture is null");
    }

    (*cmd0)++;
    return false;
}

bool gfx_set_timg_otr_filepath_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    const char* fileName = (char*)cmd->words.w1;

    uint32_t texFlags = 0;
    RawTexMetadata rawTexMetadata = {};

    std::shared_ptr<LUS::Texture> texture = std::static_pointer_cast<LUS::Texture>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess(fileName));
    if (texture != nullptr) {
        texFlags = texture->Flags;
        rawTexMetadata.width = texture->Width;
        rawTexMetadata.height = texture->Height;
        rawTexMetadata.h_byte_scale = texture->HByteScale;
        rawTexMetadata.v_pixel_scale = texture->VPixelScale;
        rawTexMetadata.type = texture->Type;
        rawTexMetadata.resource = texture;

        uint32_t fmt = C0(21, 3);
        uint32_t size = C0(19, 2);
        uint32_t width = C0(0, 10);

        gfx_dp_set_texture_image(fmt, size, width, fileName, texFlags, rawTexMetadata,
                                 reinterpret_cast<char*>(texture->ImageData));
    } else {
        SPDLOG_ERROR("G_SETTIMG_OTR_FILEPATH: Texture is null");
    }
    return false;
}

bool gfx_set_fb_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    gfx_flush();

    if (cmd->words.w1) {
        gfx_set_framebuffer((int32_t)cmd->words.w1, 1.0f);
        active_fb = framebuffers.find((int32_t)cmd->words.w1);
        fbActive = true;
    } else {
        gfx_reset_framebuffer();
        fbActive = false;
        active_fb = framebuffers.end();
    }
    return false;
}

bool gfx_reset_fb_handler_custom(F3DGfx** cmd0) {
    gfx_flush();
    fbActive = 0;
    gfx_rapi->start_draw_to_framebuffer(game_renders_to_framebuffer ? game_framebuffer : 0,
                                        (float)gfx_current_dimensions.height / gfx_native_dimensions.height);
    // Force viewport and scissor to reapply against the main framebuffer, in case a previous smaller
    // framebuffer truncated the values
    g_rdp.viewport_or_scissor_changed = true;
    rendering_state.viewport = {};
    rendering_state.scissor = {};
    return false;
}

bool gfx_copy_fb_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    bool* hasCopiedPtr = (bool*)cmd->words.w1;

    gfx_flush();
    gfx_copy_framebuffer(C0(11, 11), C0(0, 11), (bool)C0(22, 1), hasCopiedPtr);
    return false;
}

bool gfx_read_fb_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    int32_t width, height, ulx, uly;
    uint16_t* rgba16Buffer = (uint16_t*)cmd->words.w1;
    int fbId = C0(0, 8);
    bool bswap = C0(8, 1);
    ++(*cmd0);
    cmd = *cmd0;
    // Specifying the upper left origin value is unused and unsupported at the renderer level
    ulx = C0(0, 16);
    uly = C0(16, 16);
    width = C1(0, 16);
    height = C1(16, 16);

    gfx_flush();
    gfx_rapi->read_framebuffer_to_cpu(fbId, width, height, rgba16Buffer);

#ifndef IS_BIGENDIAN
    // byteswap the output to BE
    if (bswap) {
        for (size_t i = 0; i < width * height; i++) {
            rgba16Buffer[i] = BE16SWAP(rgba16Buffer[i]);
        }
    }
#endif

    return false;
}

bool gfx_set_timg_fb_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_flush();
    gfx_rapi->select_texture_fb((uint32_t)cmd->words.w1);
    g_rdp.textures_changed[0] = false;
    g_rdp.textures_changed[1] = false;
    return false;
}

bool gfx_set_grayscale_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    g_rdp.grayscale = cmd->words.w1;
    return false;
}

bool gfx_load_block_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_load_block(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
    return false;
}

bool gfx_load_tile_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_load_tile(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
    return false;
}

bool gfx_set_tile_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_set_tile(C0(21, 3), C0(19, 2), C0(9, 9), C0(0, 9), C1(24, 3), C1(20, 4), C1(18, 2), C1(14, 4), C1(10, 4),
                    C1(8, 2), C1(4, 4), C1(0, 4));
    return false;
}

bool gfx_set_tile_size_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_set_tile_size(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
    return false;
}

bool gfx_load_tlut_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_load_tlut(C1(24, 3), C1(14, 10));
    return false;
}

bool gfx_set_env_color_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_set_env_color(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
    return false;
}

bool gfx_set_prim_color_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_set_prim_color(C0(8, 8), C0(0, 8), C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
    return false;
}

bool gfx_set_fog_color_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_set_fog_color(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
    return false;
}

bool gfx_set_blend_color_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_set_blend_color(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
    return false;
}

bool gfx_set_fill_color_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_set_fill_color((uint32_t)cmd->words.w1);
    return false;
}

bool gfx_set_intensity_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_set_grayscale_color(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
    return false;
}

bool gfx_set_combine_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;

    gfx_dp_set_combine_mode(
        color_comb(C0(20, 4), C1(28, 4), C0(15, 5), C1(15, 3)), alpha_comb(C0(12, 3), C1(12, 3), C0(9, 3), C1(9, 3)),
        color_comb(C0(5, 4), C1(24, 4), C0(0, 5), C1(6, 3)), alpha_comb(C1(21, 3), C1(3, 3), C1(18, 3), C1(0, 3)));
    return false;
}

bool gfx_tex_rect_and_flip_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    int8_t opcode = (int8_t)(cmd->words.w0 >> 24);
    int32_t lrx, lry, tile, ulx, uly;
    uint32_t uls, ult, dsdx, dtdy;

    lrx = C0(12, 12);
    lry = C0(0, 12);
    tile = C1(24, 3);
    ulx = C1(12, 12);
    uly = C1(0, 12);
    // TODO make sure I don't need to increment cmd0
    ++(*cmd0);
    cmd = *cmd0;
    uls = C1(16, 16);
    ult = C1(0, 16);
    ++(*cmd0);
    cmd = *cmd0;
    dsdx = C1(16, 16);
    dtdy = C1(0, 16);

    gfx_dp_texture_rectangle(ulx, uly, lrx, lry, tile, uls, ult, dsdx, dtdy, opcode == RDP_G_TEXRECTFLIP);
    return false;
}

bool gfx_tex_rect_wide_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    int8_t opcode = (int8_t)(cmd->words.w0 >> 24);
    int32_t lrx, lry, tile, ulx, uly;
    uint32_t uls, ult, dsdx, dtdy;

    lrx = static_cast<int32_t>((C0(0, 24) << 8)) >> 8;
    lry = static_cast<int32_t>((C1(0, 24) << 8)) >> 8;
    tile = C1(24, 3);
    ++(*cmd0);
    cmd = *cmd0;
    ulx = static_cast<int32_t>((C0(0, 24) << 8)) >> 8;
    uly = static_cast<int32_t>((C1(0, 24) << 8)) >> 8;
    ++(*cmd0);
    cmd = *cmd0;
    uls = C0(16, 16);
    ult = C0(0, 16);
    dsdx = C1(16, 16);
    dtdy = C1(0, 16);
    gfx_dp_texture_rectangle(ulx, uly, lrx, lry, tile, uls, ult, dsdx, dtdy, opcode == RDP_G_TEXRECTFLIP);
    return false;
}

bool gfx_image_rect_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    int16_t tile, iw, ih;
    int16_t x0, y0, s0, t0;
    int16_t x1, y1, s1, t1;
    tile = C0(0, 3);
    iw = C1(16, 16);
    ih = C1(0, 16);
    cmd = ++(*cmd0);
    x0 = C0(16, 16);
    y0 = C0(0, 16);
    s0 = C1(16, 16);
    t0 = C1(0, 16);
    cmd = ++(*cmd0);
    x1 = C0(16, 16);
    y1 = C0(0, 16);
    s1 = C1(16, 16);
    t1 = C1(0, 16);
    gfx_dp_image_rectangle(tile, iw, ih, x0, y0, s0, t0, x1, y1, s1, t1);

    return false;
}

bool gfx_fill_rect_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *(cmd0);

    gfx_dp_fill_rectangle(C1(12, 12), C1(0, 12), C0(12, 12), C0(0, 12));
    return false;
}

bool gfx_fill_wide_rect_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *(cmd0);
    int32_t lrx, lry, ulx, uly;

    lrx = (int32_t)(C0(0, 24) << 8) >> 8;
    lry = (int32_t)(C1(0, 24) << 8) >> 8;
    cmd = ++(*cmd0);
    ulx = (int32_t)(C0(0, 24) << 8) >> 8;
    uly = (int32_t)(C1(0, 24) << 8) >> 8;
    gfx_dp_fill_rectangle(ulx, uly, lrx, lry);

    return false;
}

bool gfx_set_scissor_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *(cmd0);

    gfx_dp_set_scissor(C1(24, 2), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
    return false;
}

bool gfx_set_z_img_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *(cmd0);

    gfx_dp_set_z_image(seg_addr(cmd->words.w1));
    return false;
}

bool gfx_set_c_img_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *(cmd0);

    gfx_dp_set_color_image(C0(21, 3), C0(19, 2), C0(0, 11), seg_addr(cmd->words.w1));
    return false;
}

bool gfx_rdp_set_other_mode_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *(cmd0);

    gfx_dp_set_other_mode(C0(0, 24), (uint32_t)cmd->words.w1);
    return false;
}

bool gfx_bg_copy_handler_s2dex(F3DGfx** cmd0) {
    F3DGfx* cmd = *(cmd0);

    if (!markerOn) {
        gfx_s2dex_bg_copy((F3DuObjBg*)cmd->words.w1); // not seg_addr here it seems
    }
    return false;
}

bool gfx_bg_1cyc_handler_s2dex(F3DGfx** cmd0) {
    F3DGfx* cmd = *(cmd0);

    gfx_s2dex_bg_1cyc((F3DuObjBg*)cmd->words.w1);
    return false;
}

bool gfx_obj_rectangle_handler_s2dex(F3DGfx** cmd0) {
    F3DGfx* cmd = *(cmd0);

    if (!markerOn) {
        gfx_s2dex_rect_copy((F3DuObjSprite*)cmd->words.w1); // not seg_addr here it seems
    }
    return false;
}

bool gfx_extra_geometry_mode_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *(cmd0);

    gfx_sp_extra_geometry_mode(~C0(0, 24), (uint32_t)cmd->words.w1);
    return false;
}

bool gfx_stubbed_command_handler(F3DGfx** cmd0) {
    return false;
}

bool gfx_spnoop_command_handler_f3dex2(F3DGfx** cmd0) {
    return false;
}

class UcodeHandler {
  public:
    inline constexpr UcodeHandler(
        std::initializer_list<std::pair<int8_t, std::pair<const char*, GfxOpcodeHandlerFunc>>> initializer) {
        std::fill(std::begin(mHandlers), std::end(mHandlers),
                  std::pair<const char*, GfxOpcodeHandlerFunc>(nullptr, nullptr));

        for (const auto& [opcode, handler] : initializer) {
            mHandlers[static_cast<uint8_t>(opcode)] = handler;
        }
    }

    inline bool contains(int8_t opcode) const {
        return mHandlers[static_cast<uint8_t>(opcode)].first != nullptr;
    }

    inline std::pair<const char*, GfxOpcodeHandlerFunc> at(int8_t opcode) const {
        return mHandlers[static_cast<uint8_t>(opcode)];
    }

  private:
    std::pair<const char*, GfxOpcodeHandlerFunc> mHandlers[std::numeric_limits<uint8_t>::max() + 1];
};

static constexpr UcodeHandler rdpHandlers = {
    { RDP_G_TEXRECT, { "G_TEXRECT", gfx_tex_rect_and_flip_handler_rdp } },           // G_TEXRECT (-28)
    { RDP_G_TEXRECTFLIP, { "G_TEXRECTFLIP", gfx_tex_rect_and_flip_handler_rdp } },   // G_TEXRECTFLIP (-27)
    { RDP_G_RDPLOADSYNC, { "G_RDPLOADSYNC", gfx_stubbed_command_handler } },         // G_RDPLOADSYNC (-26)
    { RDP_G_RDPPIPESYNC, { "G_RDPPIPESYNC", gfx_stubbed_command_handler } },         // G_RDPPIPESYNC (-25)
    { RDP_G_RDPTILESYNC, { "G_RDPTILESYNC", gfx_stubbed_command_handler } },         // G_RDPPIPESYNC (-24)
    { RDP_G_RDPFULLSYNC, { "G_RDPFULLSYNC", gfx_stubbed_command_handler } },         // G_RDPFULLSYNC (-23)
    { RDP_G_SETSCISSOR, { "G_SETSCISSOR", gfx_set_scissor_handler_rdp } },           // G_SETSCISSOR (-19)
    { RDP_G_SETPRIMDEPTH, { "G_SETPRIMDEPTH", gfx_set_prim_depth_handler_rdp } },    // G_SETPRIMDEPTH (-18)
    { RDP_G_RDPSETOTHERMODE, { "G_RDPSETOTHERMODE", gfx_rdp_set_other_mode_rdp } },  // G_RDPSETOTHERMODE (-17)
    { RDP_G_LOADTLUT, { "G_LOADTLUT", gfx_load_tlut_handler_rdp } },                 // G_LOADTLUT (-16)
    { RDP_G_SETTILESIZE, { "G_SETTILESIZE", gfx_set_tile_size_handler_rdp } },       // G_SETTILESIZE (-14)
    { RDP_G_LOADBLOCK, { "G_LOADBLOCK", gfx_load_block_handler_rdp } },              // G_LOADBLOCK (-13)
    { RDP_G_LOADTILE, { "G_LOADTILE", gfx_load_tile_handler_rdp } },                 // G_LOADTILE (-12)
    { RDP_G_SETTILE, { "G_SETTILE", gfx_set_tile_handler_rdp } },                    // G_SETTILE (-11)
    { RDP_G_FILLRECT, { "G_FILLRECT", gfx_fill_rect_handler_rdp } },                 // G_FILLRECT (-10)
    { RDP_G_SETFILLCOLOR, { "G_SETFILLCOLOR", gfx_set_fill_color_handler_rdp } },    // G_SETFILLCOLOR (-9)
    { RDP_G_SETFOGCOLOR, { "G_SETFOGCOLOR", gfx_set_fog_color_handler_rdp } },       // G_SETFOGCOLOR (-8)
    { RDP_G_SETBLENDCOLOR, { "G_SETBLENDCOLOR", gfx_set_blend_color_handler_rdp } }, // G_SETBLENDCOLOR (-7)
    { RDP_G_SETPRIMCOLOR, { "G_SETPRIMCOLOR", gfx_set_prim_color_handler_rdp } },    // G_SETPRIMCOLOR (-6)
    { RDP_G_SETENVCOLOR, { "G_SETENVCOLOR", gfx_set_env_color_handler_rdp } },       // G_SETENVCOLOR (-5)
    { RDP_G_SETCOMBINE, { "G_SETCOMBINE", gfx_set_combine_handler_rdp } },           // G_SETCOMBINE (-4)
    { RDP_G_SETTIMG, { "G_SETTIMG", gfx_set_timg_handler_rdp } },                    // G_SETTIMG (-3)
    { RDP_G_SETZIMG, { "G_SETZIMG", gfx_set_z_img_handler_rdp } },                   // G_SETZIMG (-2)
    { RDP_G_SETCIMG, { "G_SETCIMG", gfx_set_c_img_handler_rdp } },                   // G_SETCIMG (-1)
};

static constexpr UcodeHandler otrHandlers = {
    { OTR_G_SETTIMG_OTR_HASH,
      { "G_SETTIMG_OTR_HASH", gfx_set_timg_otr_hash_handler_custom } },       // G_SETTIMG_OTR_HASH (0x20)
    { OTR_G_SETFB, { "G_SETFB", gfx_set_fb_handler_custom } },                // G_SETFB (0x21)
    { OTR_G_RESETFB, { "G_RESETFB", gfx_reset_fb_handler_custom } },          // G_RESETFB (0x22)
    { OTR_G_SETTIMG_FB, { "G_SETTIMG_FB", gfx_set_timg_fb_handler_custom } }, // G_SETTIMG_FB (0x23)
    { OTR_G_VTX_OTR_FILEPATH,
      { "G_VTX_OTR_FILEPATH", gfx_vtx_otr_filepath_handler_custom } }, // G_VTX_OTR_FILEPATH (0x24)
    { OTR_G_SETTIMG_OTR_FILEPATH,
      { "G_SETTIMG_OTR_FILEPATH", gfx_set_timg_otr_filepath_handler_custom } }, // G_SETTIMG_OTR_FILEPATH (0x25)
    { OTR_G_TRI1_OTR, { "G_TRI1_OTR", gfx_tri1_otr_handler_f3dex2 } },          // G_TRI1_OTR (0x26)
    { OTR_G_DL_OTR_FILEPATH, { "G_DL_OTR_FILEPATH", gfx_dl_otr_filepath_handler_custom } },  // G_DL_OTR_FILEPATH (0x27)
    { OTR_G_PUSHCD, { "G_PUSHCD", gfx_pushcd_handler_custom } },                             // G_PUSHCD (0x28)
    { OTR_G_DL_OTR_HASH, { "G_DL_OTR_HASH", gfx_dl_otr_hash_handler_custom } },              // G_DL_OTR_HASH (0x31)
    { OTR_G_VTX_OTR_HASH, { "G_VTX_OTR_HASH", gfx_vtx_hash_handler_custom } },               // G_VTX_OTR_HASH (0x32)
    { OTR_G_MARKER, { "G_MARKER", gfx_marker_handler_otr } },                                // G_MARKER (0X33)
    { OTR_G_INVALTEXCACHE, { "G_INVALTEXCACHE", gfx_invalidate_tex_cache_handler_f3dex2 } }, // G_INVALTEXCACHE (0X34)
    { OTR_G_BRANCH_Z_OTR, { "G_BRANCH_Z_OTR", gfx_branch_z_otr_handler_f3dex2 } },           // G_BRANCH_Z_OTR (0x35)
    { OTR_G_TEXRECT_WIDE, { "G_TEXRECT_WIDE", gfx_tex_rect_wide_handler_custom } },          // G_TEXRECT_WIDE (0x37)
    { OTR_G_FILLWIDERECT, { "G_FILLWIDERECT", gfx_fill_wide_rect_handler_custom } },         // G_FILLWIDERECT (0x38)
    { OTR_G_SETGRAYSCALE, { "G_SETGRAYSCALE", gfx_set_grayscale_handler_custom } },          // G_SETGRAYSCALE (0x39)
    { OTR_G_EXTRAGEOMETRYMODE,
      { "G_EXTRAGEOMETRYMODE", gfx_extra_geometry_mode_handler_custom } },          // G_EXTRAGEOMETRYMODE (0x3a)
    { OTR_G_COPYFB, { "G_COPYFB", gfx_copy_fb_handler_custom } },                   // G_COPYFB (0x3b)
    { OTR_G_IMAGERECT, { "G_IMAGERECT", gfx_image_rect_handler_custom } },          // G_IMAGERECT (0x3c)
    { OTR_G_DL_INDEX, { "G_DL_INDEX", gfx_dl_index_handler } },                     // G_DL_INDEX (0x3d)
    { OTR_G_READFB, { "G_READFB", gfx_read_fb_handler_custom } },                   // G_READFB (0x3e)
    { OTR_G_SETINTENSITY, { "G_SETINTENSITY", gfx_set_intensity_handler_custom } }, // G_SETINTENSITY (0x40)
};

static constexpr UcodeHandler f3dex2Handlers = {
    { F3DEX2_G_NOOP, { "G_NOOP", gfx_noop_handler_f3dex2 } },
    { F3DEX2_G_SPNOOP, { "G_SPNOOP", gfx_noop_handler_f3dex2 } },
    { F3DEX2_G_CULLDL, { "G_CULLDL", gfx_cull_dl_handler_f3dex2 } },
    { F3DEX2_G_MTX, { "G_MTX", gfx_mtx_handler_f3dex2 } },
    { F3DEX2_G_POPMTX, { "G_POPMTX", gfx_pop_mtx_handler_f3dex2 } },
    { F3DEX2_G_MOVEMEM, { "G_MOVEMEM", gfx_movemem_handler_f3dex2 } },
    { F3DEX2_G_MOVEWORD, { "G_MOVEWORD", gfx_moveword_handler_f3dex2 } },
    { F3DEX2_G_TEXTURE, { "G_TEXTURE", gfx_texture_handler_f3dex2 } },
    { F3DEX2_G_VTX, { "G_VTX", gfx_vtx_handler_f3dex2 } },
    { F3DEX2_G_MODIFYVTX, { "G_MODIFYVTX", gfx_modify_vtx_handler_f3dex2 } },
    { F3DEX2_G_DL, { "G_DL", gfx_dl_handler_common } },
    { F3DEX2_G_ENDDL, { "G_ENDDL", gfx_end_dl_handler_common } },
    { F3DEX2_G_GEOMETRYMODE, { "G_GEOMETRYMODE", gfx_geometry_mode_handler_f3dex2 } },
    { F3DEX2_G_TRI1, { "G_TRI1", gfx_tri1_handler_f3dex2 } },
    { F3DEX2_G_TRI2, { "G_TRI2", gfx_tri2_handler_f3dex } },
    { F3DEX2_G_QUAD, { "G_QUAD", gfx_quad_handler_f3dex2 } },
    { F3DEX2_G_SETOTHERMODE_L, { "G_SETOTHERMODE_L", gfx_othermode_l_handler_f3dex2 } },
    { F3DEX2_G_SETOTHERMODE_H, { "G_SETOTHERMODE_H", gfx_othermode_h_handler_f3dex2 } },
    { OTR_G_MTX_OTR, { "G_MTX_OTR", gfx_mtx_otr_handler_custom_f3dex2 } },
};

static constexpr UcodeHandler f3dexHandlers = {
    { F3DEX_G_NOOP, { "G_NOOP", gfx_noop_handler_f3dex2 } },
    { F3DEX_G_CULLDL, { "G_CULLDL", gfx_cull_dl_handler_f3dex2 } },
    { F3DEX_G_MTX, { "G_MTX", gfx_mtx_handler_f3d } },
    { F3DEX_G_POPMTX, { "G_POPMTX", gfx_pop_mtx_handler_f3d } },
    { F3DEX_G_MOVEMEM, { "G_POPMEM", gfx_movemem_handler_f3d } },
    { F3DEX_G_MOVEWORD, { "G_MOVEWORD", gfx_moveword_handler_f3d } },
    { F3DEX_G_TEXTURE, { "G_TEXTURE", gfx_texture_handler_f3d } },
    { F3DEX_G_SETOTHERMODE_L, { "G_SETOTHERMODE_L", gfx_othermode_l_handler_f3d } },
    { F3DEX_G_SETOTHERMODE_H, { "G_SETOTHERMODE_H", gfx_othermode_h_handler_f3d } },
    { F3DEX_G_SETGEOMETRYMODE, { "G_SETGEOMETRYMODE", gfx_set_geometry_mode_handler_f3dex } },
    { F3DEX_G_CLEARGEOMETRYMODE, { "G_CLEARGEOMETRYMODE", gfx_clear_geometry_mode_handler_f3dex } },
    { F3DEX_G_VTX, { "G_VTX", gfx_vtx_handler_f3dex } },
    { F3DEX_G_TRI1, { "G_TRI1", gfx_tri1_handler_f3dex } },
    { F3DEX_G_MODIFYVTX, { "G_MODIFYVTX", gfx_modify_vtx_handler_f3dex2 } },
    { F3DEX_G_DL, { "G_DL", gfx_dl_handler_common } },
    { F3DEX_G_ENDDL, { "G_ENDDL", gfx_end_dl_handler_common } },
    { F3DEX_G_TRI2, { "G_TRI2", gfx_tri2_handler_f3dex } },
    { F3DEX_G_SPNOOP, { "G_SPNOOP", gfx_spnoop_command_handler_f3dex2 } },
    { F3DEX_G_RDPHALF_1, { "G_RDPHALF_1", gfx_stubbed_command_handler } },
    { OTR_G_MTX_OTR2, { "G_MTX_OTR2", gfx_mtx_otr_handler_custom_f3d } }, // G_MTX_OTR2 (0x29) Is this the right code?
    { F3DEX_G_QUAD, { "G_QUAD", gfx_quad_handler_f3dex } }
};

static constexpr UcodeHandler f3dHandlers = {
    { F3DEX_G_NOOP, { "G_NOOP", gfx_noop_handler_f3dex2 } },
    { F3DEX_G_CULLDL, { "G_CULLDL", gfx_cull_dl_handler_f3dex2 } },
    { F3DEX_G_MTX, { "G_MTX", gfx_mtx_handler_f3d } },
    { F3DEX_G_POPMTX, { "G_POPMTX", gfx_pop_mtx_handler_f3d } },
    { F3DEX_G_MOVEMEM, { "G_POPMEM", gfx_movemem_handler_f3d } },
    { F3DEX_G_MOVEWORD, { "G_MOVEWORD", gfx_moveword_handler_f3d } },
    { F3DEX_G_TEXTURE, { "G_TEXTURE", gfx_texture_handler_f3d } },
    { F3DEX_G_SETOTHERMODE_L, { "G_SETOTHERMODE_L", gfx_othermode_l_handler_f3d } },
    { F3DEX_G_SETOTHERMODE_H, { "G_SETOTHERMODE_H", gfx_othermode_h_handler_f3d } },
    { F3DEX_G_SETGEOMETRYMODE, { "G_SETGEOMETRYMODE", gfx_set_geometry_mode_handler_f3dex } },
    { F3DEX_G_CLEARGEOMETRYMODE, { "G_CLEARGEOMETRYMODE", gfx_clear_geometry_mode_handler_f3dex } },
    { F3DEX_G_VTX, { "G_VTX", gfx_vtx_handler_f3d } },
    { F3DEX_G_TRI1, { "G_TRI1", gfx_tri1_handler_f3d } },
    { F3DEX_G_MODIFYVTX, { "G_MODIFYVTX", gfx_modify_vtx_handler_f3dex2 } },
    { F3DEX_G_DL, { "G_DL", gfx_dl_handler_common } },
    { F3DEX_G_ENDDL, { "G_ENDDL", gfx_end_dl_handler_common } },
    { F3DEX_G_TRI2, { "G_TRI2", gfx_tri2_handler_f3dex } },
    { F3DEX_G_SPNOOP, { "G_SPNOOP", gfx_spnoop_command_handler_f3dex2 } },
    { F3DEX_G_RDPHALF_1, { "G_RDPHALF_1", gfx_stubbed_command_handler } }
};

// LUSTODO: These S2DEX commands have different opcode numbers on F3DEX2 vs other ucodes. More research needs to be done
// to see if the implementations are different.
static constexpr UcodeHandler s2dexHandlers = {
    { F3DEX2_G_BG_COPY, { "G_BG_COPY", gfx_bg_copy_handler_s2dex } },
    { F3DEX2_G_BG_1CYC, { "G_BG_1CYC", gfx_bg_1cyc_handler_s2dex } },
    { F3DEX2_G_OBJ_RENDERMODE, { "G_OBJ_RENDERMODE", gfx_stubbed_command_handler } },
    { F3DEX2_G_OBJ_RECTANGLE_R, { "G_OBJ_RECTANGLE_R", gfx_stubbed_command_handler } },
    { F3DEX2_G_OBJ_RECTANGLE, { "G_OBJ_RECTANGLE", gfx_obj_rectangle_handler_s2dex } },
    { F3DEX2_G_DL, { "G_DL", gfx_dl_handler_common } },
    { F3DEX2_G_ENDDL, { "G_ENDDL", gfx_end_dl_handler_common } },
};

static constexpr std::array ucode_handlers = {
    &f3dHandlers,
    &f3dexHandlers,
    &f3dex2Handlers,
    &s2dexHandlers,
};

const char* GfxGetOpcodeName(int8_t opcode) {
    if (otrHandlers.contains(opcode)) {
        return otrHandlers.at(opcode).first;
    }

    if (rdpHandlers.contains(opcode)) {
        return rdpHandlers.at(opcode).first;
    }

    if (ucode_handler_index < ucode_handlers.size()) {
        if (ucode_handlers[ucode_handler_index]->contains(opcode)) {
            return ucode_handlers[ucode_handler_index]->at(opcode).first;
        } else {
            SPDLOG_CRITICAL("Unhandled OP code: 0x{:X}, for loaded ucode: {}", opcode, (uint32_t)ucode_handler_index);
            return nullptr;
        }
    }
}

// TODO, implement a system where we can get the current opcode handler by writing to the GWords. If the powers that be
// are OK with that...
static void gfx_set_ucode_handler(UcodeHandlers ucode) {
    // Loaded ucode must be in range of the supported ucode_handlers
    assert(ucode < ucode_max);
    ucode_handler_index = ucode;
}

static void gfx_step() {
    auto& cmd = g_exec_stack.currCmd();
    auto cmd0 = cmd;
    int8_t opcode = (int8_t)(cmd->words.w0 >> 24);

    if (opcode == F3DEX2_G_LOAD_UCODE) {
        gfx_set_ucode_handler((UcodeHandlers)(cmd->words.w0 & 0xFFFFFF));
        ++cmd;
        return;
        // Instead of having a handler for each ucode for switching ucode, just check for it early and return.
    }

    if (otrHandlers.contains(opcode)) {
        if (otrHandlers.at(opcode).second(&cmd)) {
            return;
        }
    } else if (rdpHandlers.contains(opcode)) {
        if (rdpHandlers.at(opcode).second(&cmd)) {
            return;
        }
    } else if (ucode_handler_index < ucode_handlers.size()) {
        if (ucode_handlers[ucode_handler_index]->contains(opcode)) {
            if (ucode_handlers[ucode_handler_index]->at(opcode).second(&cmd)) {
                return;
            }
        } else {
            SPDLOG_CRITICAL("Unhandled OP code: 0x{:X}, for loaded ucode: {}", opcode, (uint32_t)ucode_handler_index);
        }
    }

    ++cmd;
}

static void gfx_sp_reset() {
    g_rsp.modelview_matrix_stack_size = 1;
    g_rsp.current_num_lights = 2;
    g_rsp.lights_changed = true;
}

void gfx_get_dimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY) {
    gfx_wapi->get_dimensions(width, height, posX, posY);
}

void gfx_init(struct GfxWindowManagerAPI* wapi, struct GfxRenderingAPI* rapi, const char* game_name,
              bool start_in_fullscreen, uint32_t width, uint32_t height, uint32_t posX, uint32_t posY) {
    gfx_wapi = wapi;
    gfx_rapi = rapi;
    gfx_wapi->init(game_name, rapi->get_name(), start_in_fullscreen, width, height, posX, posY);
    gfx_rapi->init();
    gfx_rapi->update_framebuffer_parameters(0, width, height, 1, false, true, true, true);
#ifdef __APPLE__
    gfx_current_dimensions.internal_mul = 1;
#else
    gfx_current_dimensions.internal_mul = CVarGetFloat(CVAR_INTERNAL_RESOLUTION, 1);
#endif
    gfx_msaa_level = CVarGetInteger(CVAR_MSAA_VALUE, 1);

    gfx_current_dimensions.width = width;
    gfx_current_dimensions.height = height;

    game_framebuffer = gfx_rapi->create_framebuffer();
    game_framebuffer_msaa_resolved = gfx_rapi->create_framebuffer();

    gfx_native_dimensions.width = SCREEN_WIDTH;
    gfx_native_dimensions.height = SCREEN_HEIGHT;

    for (int i = 0; i < 16; i++) {
        gSegmentPointers[i] = 0;
    }

    if (tex_upload_buffer == nullptr) {
        // We cap texture max to 8k, because why would you need more?
        int max_tex_size = min(8192, gfx_rapi->get_max_texture_size());
        tex_upload_buffer = (uint8_t*)malloc(max_tex_size * max_tex_size * 4);
    }

    ucode_handler_index = UcodeHandlers::ucode_f3dex2;
}

void gfx_destroy(void) {
    // TODO: should also destroy rapi, and any other resources acquired in fast3d
    free(tex_upload_buffer);
    gfx_wapi->destroy();

    // Texture cache and loaded textures store references to Resources which need to be unreferenced.
    gfx_texture_cache_clear();
    g_rdp.texture_to_load.raw_tex_metadata.resource = nullptr;
    g_rdp.loaded_texture[0].raw_tex_metadata.resource = nullptr;
    g_rdp.loaded_texture[1].raw_tex_metadata.resource = nullptr;
}

struct GfxRenderingAPI* gfx_get_current_rendering_api(void) {
    return gfx_rapi;
}

void gfx_start_frame(void) {
    gfx_wapi->handle_events();
    gfx_wapi->get_dimensions(&gfx_current_window_dimensions.width, &gfx_current_window_dimensions.height,
                             &gfx_current_window_position_x, &gfx_current_window_position_y);
    Ship::Context::GetInstance()->GetWindow()->GetGui()->DrawMenu();
    has_drawn_imgui_menu = true;
    if (gfx_current_dimensions.height == 0) {
        // Avoid division by zero
        gfx_current_dimensions.height = 1;
    }
    gfx_current_dimensions.aspect_ratio = (float)gfx_current_dimensions.width / (float)gfx_current_dimensions.height;

    // Update the framebuffer sizes when the viewport or native dimension changes
    if (gfx_current_dimensions.width != gfx_prev_dimensions.width ||
        gfx_current_dimensions.height != gfx_prev_dimensions.height ||
        gfx_native_dimensions.width != gfx_prev_native_dimensions.width ||
        gfx_native_dimensions.height != gfx_prev_native_dimensions.height) {

        for (auto& fb : framebuffers) {
            uint32_t width = fb.second.orig_width, height = fb.second.orig_height;
            if (fb.second.resize) {
                gfx_adjust_width_height_for_scale(width, height, fb.second.native_width, fb.second.native_height);
            }
            if (width != fb.second.applied_width || height != fb.second.applied_height) {
                gfx_rapi->update_framebuffer_parameters(fb.first, width, height, 1, true, true, true, true);
                fb.second.applied_width = width;
                fb.second.applied_height = height;
            }
        }
    }

    gfx_prev_dimensions = gfx_current_dimensions;
    gfx_prev_native_dimensions = gfx_native_dimensions;

    bool different_size = gfx_current_dimensions.width != gfx_current_game_window_viewport.width ||
                          gfx_current_dimensions.height != gfx_current_game_window_viewport.height;
    if (different_size || gfx_msaa_level > 1) {
        game_renders_to_framebuffer = true;
        if (different_size) {
            gfx_rapi->update_framebuffer_parameters(game_framebuffer, gfx_current_dimensions.width,
                                                    gfx_current_dimensions.height, gfx_msaa_level, true, true, true,
                                                    true);
        } else {
            // MSAA framebuffer needs to be resolved to an equally sized target when complete, which must therefore
            // match the window size
            gfx_rapi->update_framebuffer_parameters(game_framebuffer, gfx_current_window_dimensions.width,
                                                    gfx_current_window_dimensions.height, gfx_msaa_level, false, true,
                                                    true, true);
        }
        if (gfx_msaa_level > 1 && different_size) {
            gfx_rapi->update_framebuffer_parameters(game_framebuffer_msaa_resolved, gfx_current_dimensions.width,
                                                    gfx_current_dimensions.height, 1, false, false, false, false);
        }
    } else {
        game_renders_to_framebuffer = false;
    }

    fbActive = 0;
}

GfxExecStack g_exec_stack = {};

void gfx_run(Gfx* commands, const std::unordered_map<Mtx*, MtxF>& mtx_replacements) {
    gfx_sp_reset();

    // puts("New frame");
    get_pixel_depth_pending.clear();
    get_pixel_depth_cached.clear();

    if (!gfx_wapi->start_frame()) {
        dropped_frame = true;
        if (has_drawn_imgui_menu) {
            Ship::Context::GetInstance()->GetWindow()->GetGui()->StartFrame();
            Ship::Context::GetInstance()->GetWindow()->GetGui()->EndFrame();
            has_drawn_imgui_menu = false;
        }
        return;
    }
    dropped_frame = false;

    if (!has_drawn_imgui_menu) {
        Ship::Context::GetInstance()->GetWindow()->GetGui()->DrawMenu();
    }

    current_mtx_replacements = &mtx_replacements;

    gfx_rapi->update_framebuffer_parameters(0, gfx_current_window_dimensions.width,
                                            gfx_current_window_dimensions.height, 1, false, true, true,
                                            !game_renders_to_framebuffer);
    gfx_rapi->start_frame();
    gfx_rapi->start_draw_to_framebuffer(game_renders_to_framebuffer ? game_framebuffer : 0,
                                        (float)gfx_current_dimensions.height / gfx_native_dimensions.height);
    gfx_rapi->clear_framebuffer();
    g_rdp.viewport_or_scissor_changed = true;
    rendering_state.viewport = {};
    rendering_state.scissor = {};

    auto dbg = Ship::Context::GetInstance()->GetGfxDebugger();
    g_exec_stack.start((F3DGfx*)commands);
    while (!g_exec_stack.cmd_stack.empty()) {
        auto cmd = g_exec_stack.cmd_stack.top();

        if (dbg->IsDebugging()) {
            g_exec_stack.gfx_path.push_back(cmd);
            if (dbg->HasBreakPoint(g_exec_stack.gfx_path)) {
                break;
            }
            g_exec_stack.gfx_path.pop_back();
        }
        gfx_step();
    }
    gfx_flush();
    gfxFramebuffer = 0;
    currentDir = std::stack<std::string>();

    if (game_renders_to_framebuffer) {
        gfx_rapi->start_draw_to_framebuffer(0, 1);
        gfx_rapi->clear_framebuffer();

        if (gfx_msaa_level > 1) {
            bool different_size = gfx_current_dimensions.width != gfx_current_game_window_viewport.width ||
                                  gfx_current_dimensions.height != gfx_current_game_window_viewport.height;

            if (different_size) {
                gfx_rapi->resolve_msaa_color_buffer(game_framebuffer_msaa_resolved, game_framebuffer);
                gfxFramebuffer = (uintptr_t)gfx_rapi->get_framebuffer_texture_id(game_framebuffer_msaa_resolved);
            } else {
                gfx_rapi->resolve_msaa_color_buffer(0, game_framebuffer);
            }
        } else {
            gfxFramebuffer = (uintptr_t)gfx_rapi->get_framebuffer_texture_id(game_framebuffer);
        }
    }
    Ship::Context::GetInstance()->GetWindow()->GetGui()->StartFrame();
    Ship::Context::GetInstance()->GetWindow()->GetGui()->RenderViewports();
    gfx_rapi->end_frame();
    gfx_wapi->swap_buffers_begin();
    has_drawn_imgui_menu = false;
}

void gfx_end_frame(void) {
    if (!dropped_frame) {
        gfx_rapi->finish_render();
        gfx_wapi->swap_buffers_end();
    }
}

void gfx_set_target_ucode(UcodeHandlers ucode) {
    ucode_handler_index = ucode;
}

void gfx_set_target_fps(int fps) {
    gfx_wapi->set_target_fps(fps);
}

void gfx_set_maximum_frame_latency(int latency) {
    gfx_wapi->set_maximum_frame_latency(latency);
}

extern "C" int gfx_create_framebuffer(uint32_t width, uint32_t height, uint32_t native_width, uint32_t native_height,
                                      uint8_t resize) {
    uint32_t orig_width = width, orig_height = height;
    if (resize) {
        gfx_adjust_width_height_for_scale(width, height, native_width, native_height);
    }

    int fb = gfx_rapi->create_framebuffer();
    gfx_rapi->update_framebuffer_parameters(fb, width, height, 1, true, true, true, true);

    framebuffers[fb] = {
        orig_width, orig_height, width, height, native_width, native_height, static_cast<bool>(resize)
    };
    return fb;
}

void gfx_set_framebuffer(int fb, float noise_scale) {
    gfx_rapi->start_draw_to_framebuffer(fb, noise_scale);
    gfx_rapi->clear_framebuffer();
}

void gfx_copy_framebuffer(int fb_dst_id, int fb_src_id, bool copyOnce, bool* hasCopiedPtr) {
    // Do not copy again if we have already copied before
    if (copyOnce && hasCopiedPtr != nullptr && *hasCopiedPtr) {
        return;
    }

    if (fb_src_id == 0 && game_renders_to_framebuffer) {
        // read from the framebuffer we've been rendering to
        fb_src_id = game_framebuffer;
    }

    int srcX0, srcY0, srcX1, srcY1;
    int dstX0, dstY0, dstX1, dstY1;

    // When rendering to the main window buffer or MSAA is enabled with a buffer size equal to the view port,
    // then the source coordinates must account for any docked ImGui elements
    if (fb_src_id == 0 ||
        (gfx_msaa_level > 1 && gfx_current_dimensions.width == gfx_current_game_window_viewport.width &&
         gfx_current_dimensions.height == gfx_current_game_window_viewport.height)) {
        srcX0 = gfx_current_game_window_viewport.x;
        srcY0 = gfx_current_game_window_viewport.y;
        srcX1 = gfx_current_game_window_viewport.x + gfx_current_game_window_viewport.width;
        srcY1 = gfx_current_game_window_viewport.y + gfx_current_game_window_viewport.height;
    } else {
        srcX0 = 0;
        srcY0 = 0;
        srcX1 = gfx_current_dimensions.width;
        srcY1 = gfx_current_dimensions.height;
    }

    dstX0 = 0;
    dstY0 = 0;
    dstX1 = gfx_current_dimensions.width;
    dstY1 = gfx_current_dimensions.height;

    gfx_rapi->copy_framebuffer(fb_dst_id, fb_src_id, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1);

    // Set the copied pointer if we have one
    if (hasCopiedPtr != nullptr) {
        *hasCopiedPtr = true;
    }
}

void gfx_reset_framebuffer() {
    gfx_rapi->start_draw_to_framebuffer(0, (float)gfx_current_dimensions.height / gfx_native_dimensions.height);
    gfx_rapi->clear_framebuffer();
}

static void adjust_pixel_depth_coordinates(float& x, float& y) {
    x = x * RATIO_X - (gfx_native_dimensions.width * RATIO_X - gfx_current_dimensions.width) / 2;
    y *= RATIO_Y;
    if (!game_renders_to_framebuffer ||
        (gfx_msaa_level > 1 && gfx_current_dimensions.width == gfx_current_game_window_viewport.width &&
         gfx_current_dimensions.height == gfx_current_game_window_viewport.height)) {
        x += gfx_current_game_window_viewport.x;
        y += gfx_current_window_dimensions.height -
             (gfx_current_game_window_viewport.y + gfx_current_game_window_viewport.height);
    }
}

void gfx_get_pixel_depth_prepare(float x, float y) {
    adjust_pixel_depth_coordinates(x, y);
    get_pixel_depth_pending.emplace(x, y);
}

uint16_t gfx_get_pixel_depth(float x, float y) {
    adjust_pixel_depth_coordinates(x, y);

    if (auto it = get_pixel_depth_cached.find(make_pair(x, y)); it != get_pixel_depth_cached.end()) {
        return it->second;
    }

    get_pixel_depth_pending.emplace(x, y);

    unordered_map<pair<float, float>, uint16_t, hash_pair_ff> res =
        gfx_rapi->get_pixel_depth(game_renders_to_framebuffer ? game_framebuffer : 0, get_pixel_depth_pending);
    get_pixel_depth_cached.merge(res);
    get_pixel_depth_pending.clear();

    return get_pixel_depth_cached.find(make_pair(x, y))->second;
}

void gfx_push_current_dir(char* path) {
    if (gfx_check_image_signature(path) == 1)
        path = &path[7];

    currentDir.push(GetPathWithoutFileName(path));
}

int32_t gfx_check_image_signature(const char* imgData) {
    uintptr_t i = (uintptr_t)(imgData);

    if ((i & 1) == 1) {
        return 0;
    }

    if (i != 0) {
        return Ship::Context::GetInstance()->GetResourceManager()->OtrSignatureCheck(imgData);
    }

    return 0;
}

void gfx_register_blended_texture(const char* name, uint8_t* mask, uint8_t* replacement) {
    if (gfx_check_image_signature(name)) {
        name += 7;
    }

    if (gfx_check_image_signature(reinterpret_cast<char*>(replacement))) {
        LUS::Texture* tex = std::static_pointer_cast<LUS::Texture>(
                                Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess(
                                    reinterpret_cast<char*>(replacement)))
                                .get();

        replacement = tex->ImageData;
    }

    masked_textures[name] = MaskedTextureEntry{ mask, replacement };
}

void gfx_unregister_blended_texture(const char* name) {
    if (gfx_check_image_signature(name)) {
        name += 7;
    }

    masked_textures.erase(name);
}
