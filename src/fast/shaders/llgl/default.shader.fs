@prism(type='fragment', name='Fast3D LLGL Fragment Shader', version='1.0.0', description='Vulkan GLSL fragment shader for the LLGL backend', author='libultraship')

#version 450 core

// Binding callbacks (no string args – integer codes only):
//   fc_binding(0)     – frame_count UBO
//   ns_binding(0)     – noise_scale UBO
//   tex_binding(i,t)  – texture2D binding  (t: 0=regular, 1=mask, 2=blend)
//   samp_binding(i,t) – sampler binding    (t: 0=regular, 1=mask, 2=blend)
//   frag_in_loc(0)    – fragment input location (auto-increment, arg unused)

// ---- Constant buffers (UBOs) -----------------------------------------------
layout(std140, binding = @{fc_binding(0)}) uniform FrameCountBlock {
    int frame_count;
};
layout(std140, binding = @{ns_binding(0)}) uniform NoiseScaleBlock {
    float noise_scale;
};

// ---- Textures and samplers --------------------------------------------------
@for(i in 0..2)
    @if(o_textures[i])
        layout(binding = @{tex_binding(i, 0)}) uniform texture2D uTex@{i};
        layout(binding = @{samp_binding(i, 0)}) uniform sampler uTexSampl@{i};
    @end
    @if(o_masks[i])
        layout(binding = @{tex_binding(i, 1)}) uniform texture2D uTexMask@{i};
        layout(binding = @{samp_binding(i, 1)}) uniform sampler uTexMaskSampl@{i};
    @end
    @if(o_blend[i])
        layout(binding = @{tex_binding(i, 2)}) uniform texture2D uTexBlend@{i};
        layout(binding = @{samp_binding(i, 2)}) uniform sampler uTexBlendSampl@{i};
    @end
@end

// ---- Fragment inputs --------------------------------------------------------
@for(i in 0..2)
    @if(o_textures[i])
        layout(location = @{frag_in_loc(0)}) in vec2 vTexCoord@{i};
        @for(j in 0..2)
            @if(o_clamp[i][j])
                @if(j == 0)
                    layout(location = @{frag_in_loc(0)}) in float vTexClampS@{i};
                @else
                    layout(location = @{frag_in_loc(0)}) in float vTexClampT@{i};
                @end
            @end
        @end
    @end
@end

@if(o_fog) layout(location = @{frag_in_loc(0)}) in vec4 vFog;
@if(o_grayscale) layout(location = @{frag_in_loc(0)}) in vec4 vGrayscaleColor;

@for(i in 0..o_inputs)
    @if(o_alpha)
        layout(location = @{frag_in_loc(0)}) in vec4 vInput@{i + 1};
    @else
        layout(location = @{frag_in_loc(0)}) in vec3 vInput@{i + 1};
    @end
@end

// ---- Output ----------------------------------------------------------------
layout(location = 0) out vec4 vOutColor;

// ---- Helpers ---------------------------------------------------------------
float random(in vec3 value) {
    float r = dot(sin(value), vec3(12.9898, 78.233, 37.719));
    return fract(sin(r) * 143758.5453);
}

#define RAND_NOISE ((random(vec3(floor(gl_FragCoord.xy * noise_scale), float(frame_count))) + 1.0) / 2.0)

vec4 fromLinear(vec4 linearRGB) {
    bvec3 cutoff = lessThan(linearRGB.rgb, vec3(0.0031308));
    vec3 higher = vec3(1.055) * pow(linearRGB.rgb, vec3(1.0 / 2.4)) - vec3(0.055);
    vec3 lower  = linearRGB.rgb * vec3(12.92);
    return vec4(mix(higher, lower, cutoff), linearRGB.a);
}

// Three-point filter (N64-faithful).
// tex/sampl are the Vulkan-GLSL separate texture2D/sampler types.
vec4 filter3point(in texture2D tex, in sampler sampl, in vec2 texCoord, in vec2 texSize) {
    vec2 offset = fract(texCoord * texSize - vec2(0.5));
    offset -= step(1.0, offset.x + offset.y);
    vec4 c0 = texture(sampler2D(tex, sampl), texCoord - offset / texSize);
    vec4 c1 = texture(sampler2D(tex, sampl), texCoord - vec2(offset.x - sign(offset.x), offset.y) / texSize);
    vec4 c2 = texture(sampler2D(tex, sampl), texCoord - vec2(offset.x, offset.y - sign(offset.y)) / texSize);
    return c0 + abs(offset.x) * (c1 - c0) + abs(offset.y) * (c2 - c0);
}

vec4 hookTexture2D(in texture2D tex, in sampler sampl, in vec2 uv, in vec2 texSize) {
    @if(o_three_point_filtering)
        return filter3point(tex, sampl, uv, texSize);
    @else
        return texture(sampler2D(tex, sampl), uv);
    @end
}

#define WRAP(x, low, high) mod((x) - (low), (high) - (low)) + (low)

void main() {
    @for(i in 0..2)
        @if(o_textures[i])
            @{s = o_clamp[i][0]}
            @{t = o_clamp[i][1]}

            vec2 texSize@{i} = vec2(textureSize(sampler2D(uTex@{i}, uTexSampl@{i}), 0));

            @if(!s && !t)
                vec2 vTexCoordAdj@{i} = vTexCoord@{i};
            @else
                @if(s && t)
                    vec2 vTexCoordAdj@{i} = clamp(vTexCoord@{i}, 0.5 / texSize@{i}, vec2(vTexClampS@{i}, vTexClampT@{i}));
                @elseif(s)
                    vec2 vTexCoordAdj@{i} = vec2(clamp(vTexCoord@{i}.s, 0.5 / texSize@{i}.s, vTexClampS@{i}), vTexCoord@{i}.t);
                @else
                    vec2 vTexCoordAdj@{i} = vec2(vTexCoord@{i}.s, clamp(vTexCoord@{i}.t, 0.5 / texSize@{i}.t, vTexClampT@{i}));
                @end
            @end

            vec4 texVal@{i} = hookTexture2D(uTex@{i}, uTexSampl@{i}, vTexCoordAdj@{i}, texSize@{i});

            @if(o_masks[i])
                vec2 maskSize@{i} = vec2(textureSize(sampler2D(uTexMask@{i}, uTexMaskSampl@{i}), 0));
                vec4 maskVal@{i} = hookTexture2D(uTexMask@{i}, uTexMaskSampl@{i}, vTexCoordAdj@{i}, maskSize@{i});
                @if(o_blend[i])
                    vec4 blendVal@{i} = hookTexture2D(uTexBlend@{i}, uTexBlendSampl@{i}, vTexCoordAdj@{i}, texSize@{i});
                @else
                    vec4 blendVal@{i} = vec4(0.0, 0.0, 0.0, 0.0);
                @end
                texVal@{i} = mix(texVal@{i}, blendVal@{i}, maskVal@{i}.a);
            @end
        @end
    @end

    @if(o_alpha)
        vec4 texel;
    @else
        vec3 texel;
    @end

    @if(o_2cyc)
        @{f_range = 2}
    @else
        @{f_range = 1}
    @end

    @for(c in 0..f_range)
        @if(c == 1)
            @if(o_alpha)
                @if(o_c[c][1][2] == SHADER_COMBINED)
                    texel.a = WRAP(texel.a, -1.01, 1.01);
                @else
                    texel.a = WRAP(texel.a, -0.51, 1.51);
                @end
            @end
            @if(o_c[c][0][2] == SHADER_COMBINED)
                texel.rgb = WRAP(texel.rgb, -1.01, 1.01);
            @else
                texel.rgb = WRAP(texel.rgb, -0.51, 1.51);
            @end
        @end

        @if(!o_color_alpha_same[c] && o_alpha)
            texel = vec4(@{
            append_formula(o_c[c], o_do_single[c][0],
                        o_do_multiply[c][0], o_do_mix[c][0], false, false, true, c == 0)
            }, @{append_formula(o_c[c], o_do_single[c][1],
                        o_do_multiply[c][1], o_do_mix[c][1], true, true, true, c == 0)
            });
        @else
            texel = @{append_formula(o_c[c], o_do_single[c][0],
                        o_do_multiply[c][0], o_do_mix[c][0], o_alpha, false,
                        o_alpha, c == 0)};
        @end
    @end

    texel = WRAP(texel, -0.51, 1.51);
    texel = clamp(texel, 0.0, 1.0);

    @if(o_fog)
        @if(o_alpha)
            texel = vec4(mix(texel.rgb, vFog.rgb, vFog.a), texel.a);
        @else
            texel = mix(texel, vFog.rgb, vFog.a);
        @end
    @end

    @if(o_texture_edge && o_alpha)
        if (texel.a > 0.19) texel.a = 1.0; else discard;
    @end

    @if(o_alpha && o_noise)
        texel.a *= floor(clamp(random(vec3(floor(gl_FragCoord.xy * noise_scale), float(frame_count))) + texel.a, 0.0, 1.0));
    @end

    @if(o_grayscale)
        float intensity = (texel.r + texel.g + texel.b) / 3.0;
        vec3 new_texel = vGrayscaleColor.rgb * intensity;
        texel.rgb = mix(texel.rgb, new_texel, vGrayscaleColor.a);
    @end

    @if(o_alpha)
        @if(o_alpha_threshold)
            if (texel.a < 8.0 / 256.0) discard;
        @end
        @if(o_invisible)
            texel.a = 0.0;
        @end
        vOutColor = texel;
    @else
        vOutColor = vec4(texel, 1.0);
    @end

    @if(srgb_mode)
        vOutColor = fromLinear(vOutColor);
    @end
}
