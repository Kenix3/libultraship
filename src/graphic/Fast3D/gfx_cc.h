#ifndef GFX_CC_H
#define GFX_CC_H

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
    SHADER_NOISE
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
    MAX
};

#define SHADER_OPT(opt) ((uint64_t)(1 << static_cast<int>(ShaderOpts::opt)))
#endif

struct ColorCombinerKey {
    uint64_t combine_mode;
    uint64_t options;

#ifdef __cplusplus
    auto operator<=>(const ColorCombinerKey&) const = default;
#endif
};

#define SHADER_MAX_TEXTURES 6
#define SHADER_FIRST_TEXTURE 0
#define SHADER_FIRST_MASK_TEXTURE 2
#define SHADER_FIRST_REPLACEMENT_TEXTURE 4

struct CCFeatures {
    uint8_t c[2][2][4];
    bool opt_alpha;
    bool opt_fog;
    bool opt_texture_edge;
    bool opt_noise;
    bool opt_2cyc;
    bool opt_alpha_threshold;
    bool opt_invisible;
    bool opt_grayscale;
    bool used_textures[2];
    bool used_masks[2];
    bool used_blend[2];
    bool clamp[2][2];
    int num_inputs;
    bool do_single[2][2];
    bool do_multiply[2][2];
    bool do_mix[2][2];
    bool color_alpha_same[2];
};

void gfx_cc_get_features(uint64_t shader_id0, uint32_t shader_id1, struct CCFeatures* cc_features);

#endif
