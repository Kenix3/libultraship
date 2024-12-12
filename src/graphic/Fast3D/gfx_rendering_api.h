#ifndef GFX_RENDERING_API_H
#define GFX_RENDERING_API_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <map>
#include <unordered_map>
#include <set>

struct ShaderProgram;

struct GfxClipParameters {
    bool z_is_from_0_to_1;
    bool invert_y;
};

enum FilteringMode { FILTER_THREE_POINT, FILTER_LINEAR, FILTER_NONE };

// A hash function used to hash a: pair<float, float>
struct hash_pair_ff {
    size_t operator()(const std::pair<float, float>& p) const {
        auto hash1 = std::hash<float>{}(p.first);
        auto hash2 = std::hash<float>{}(p.second);

        // If hash1 == hash2, their XOR is zero.
        return (hash1 != hash2) ? hash1 ^ hash2 : hash1;
    }
};

struct GfxRenderingAPI {
    const char* (*get_name)();
    int (*get_max_texture_size)();
    struct GfxClipParameters (*get_clip_parameters)();
    void (*unload_shader)(struct ShaderProgram* old_prg);
    void (*load_shader)(struct ShaderProgram* new_prg);
    struct ShaderProgram* (*create_and_load_new_shader)(uint64_t shader_id0, uint32_t shader_id1);
    struct ShaderProgram* (*lookup_shader)(uint64_t shader_id0, uint32_t shader_id1);
    void (*shader_get_info)(struct ShaderProgram* prg, uint8_t* num_inputs, bool used_textures[2]);
    uint32_t (*new_texture)();
    void (*select_texture)(int tile, uint32_t texture_id);
    void (*upload_texture)(const uint8_t* rgba32_buf, uint32_t width, uint32_t height);
    void (*set_sampler_parameters)(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt);
    void (*set_depth_test_and_mask)(bool depth_test, bool z_upd);
    void (*set_zmode_decal)(bool zmode_decal);
    void (*set_viewport)(int x, int y, int width, int height);
    void (*set_scissor)(int x, int y, int width, int height);
    void (*set_use_alpha)(bool use_alpha);
    void (*draw_triangles)(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris);
    void (*init)();
    void (*on_resize)();
    void (*start_frame)();
    void (*end_frame)();
    void (*finish_render)();
    int (*create_framebuffer)();
    void (*update_framebuffer_parameters)(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                          bool opengl_invert_y, bool render_target, bool has_depth_buffer,
                                          bool can_extract_depth);
    void (*start_draw_to_framebuffer)(int fb_id, float noise_scale);
    void (*copy_framebuffer)(int fb_dst_id, int fb_src_id, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0,
                             int dstY0, int dstX1, int dstY1);
    void (*clear_framebuffer)(bool color, bool depth);
    void (*read_framebuffer_to_cpu)(int fb_id, uint32_t width, uint32_t height, uint16_t* rgba16_buf);
    void (*resolve_msaa_color_buffer)(int fb_id_target, int fb_id_source);
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> (*get_pixel_depth)(
        int fb_id, const std::set<std::pair<float, float>>& coordinates);
    void* (*get_framebuffer_texture_id)(int fb_id);
    void (*select_texture_fb)(int fb_id);
    void (*delete_texture)(uint32_t texID);
    void (*set_texture_filter)(FilteringMode mode);
    FilteringMode (*get_texture_filter)();
    void (*enable_srgb_mode)();
};

#endif
