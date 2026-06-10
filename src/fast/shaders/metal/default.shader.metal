@prism(type='metal', name='Fast3D Metal Shader', version='1.0.0', description='Ported shader to prism', author='Emill & Prism Team')

#include <metal_stdlib>
using namespace metal;

// BEGIN VERTEX SHADER
struct FrameUniforms {
    int frameCount;
    float noiseScale;
};

struct DrawUniforms {
    int textureFiltering[6];
    float prim_depth;
    float lod_max;
    float4 inputs[6];
    float4 fog_color;
    float4 grayscale_color;
};

@if(o_lighting || o_texgen)
// Mirrors struct LightingUniforms in gfx_rendering_api.h
struct LightUniforms {
    float4 lights[96]; // 3 float4 per light: color/isPoint, dir/kc, pos/kq
    float4 ambient;
    float4 lookat_x;
    float4 lookat_y;
    float4 texgen_uv[2];
    float4 mv_rows[3];
    int num_lights;
};
@end

struct FragOut {
    float4 color [[color(0)]];
    @if(o_prim_depth)
    float depth [[depth(any)]];
    @end
};

struct Vertex {
    float4 position [[attribute(@{get_vertex_index()})]];
    @{update_floats(4)}
    @for(i in 0..2)
        @if(o_textures[i])
            float2 texCoord@{i} [[attribute(@{get_vertex_index()})]];
            @{update_floats(2)}
            @for(j in 0..2)
                @if(o_clamp[i][j])
                    @if(j == 0)
                        float texClampS@{i} [[attribute(@{get_vertex_index()})]];
                    @else
                        float texClampT@{i} [[attribute(@{get_vertex_index()})]];
                    @end
                    @{update_floats(1)}
                @end
            @end
        @end
    @end
    @if(o_fog)
        float fogFactor [[attribute(@{get_vertex_index()})]];
        @{update_floats(1)}
    @end
    @if(o_shade || o_lighting)
        @if(o_alpha)
            float4 shade [[attribute(@{get_vertex_index()})]];
            @{update_floats(4)}
        @else
            float3 shade [[attribute(@{get_vertex_index()})]];
            @{update_floats(3)}
        @end
    @end
    @if(o_point_lighting)
        float3 worldPos [[attribute(@{get_vertex_index()})]];
        @{update_floats(3)}
    @end
};

struct ProjectedVertex {
    @for(i in 0..2)
        @if(o_textures[i])
            float2 texCoord@{i};
            @for(j in 0..2)
                @if(o_clamp[i][j])
                    @if(j == 0)
                        float texClampS@{i};
                    @else
                        float texClampT@{i};
                    @end
                @end
            @end
        @end
    @end
    @if(o_fog)
        float fogFactor;
    @end
    @if(o_shade)
        @if(o_alpha)
            float4 shade;
        @else
            float3 shade;
        @end
    @end
    float4 position [[position]];
};

vertex ProjectedVertex vertexShader(Vertex in [[stage_in]]
@if(o_lighting || o_texgen)
    , constant LightUniforms& lightUniforms [[buffer(1)]]
@end
) {
    ProjectedVertex out;
    @for(i in 0..2)
        @if(o_textures[i])
            out.texCoord@{i} = in.texCoord@{i};
            @for(j in 0..2)
                @if(o_clamp[i][j])
                    @if(j == 0)
                        out.texClampS@{i} = in.texClampS@{i};
                    @else
                        out.texClampT@{i} = in.texClampT@{i};
                    @end
                @end
            @end
        @end
    @end
    @if(o_fog)
        out.fogFactor = in.fogFactor;
    @end
    @if(o_texgen)
        // N64 texgen: project the vertex normal onto the lookat vectors, then
        // run the result through the tile/scale UV pipeline (folded into the
        // per-draw texgen linear transforms).
        float texgenDotX = clamp(dot(in.shade.xyz, lightUniforms.lookat_x.xyz) / 127.0, -1.0, 1.0);
        float texgenDotY = clamp(dot(in.shade.xyz, lightUniforms.lookat_y.xyz) / 127.0, -1.0, 1.0);
        @if(o_texgen_linear)
            texgenDotX = acos(-texgenDotX) * 0.159155;
            texgenDotY = acos(-texgenDotY) * 0.159155;
        @else
            texgenDotX = (texgenDotX + 1.0) / 4.0;
            texgenDotY = (texgenDotY + 1.0) / 4.0;
        @end
        @if(o_textures[0])
            out.texCoord0 = float2(texgenDotX * lightUniforms.texgen_uv[0].x + lightUniforms.texgen_uv[0].y,
                                   texgenDotY * lightUniforms.texgen_uv[0].z + lightUniforms.texgen_uv[0].w);
        @end
        @if(o_textures[1])
            out.texCoord1 = float2(texgenDotX * lightUniforms.texgen_uv[1].x + lightUniforms.texgen_uv[1].y,
                                   texgenDotY * lightUniforms.texgen_uv[1].z + lightUniforms.texgen_uv[1].w);
        @end
    @end
    @if(o_shade)
        @if(o_lighting)
            // N64 RSP lighting: in.shade carries the raw signed vertex normal
            float3 litColor = lightUniforms.ambient.xyz;
            for (int i = 0; i < lightUniforms.num_lights; i++) {
                float intensity = 0.0;
                @if(o_point_lighting)
                if (lightUniforms.lights[i * 3].w > 0.5) {
                    float3 distVec = lightUniforms.lights[i * 3 + 2].xyz - in.worldPos;
                    float distSq = distVec.x * distVec.x + distVec.y * distVec.y + distVec.z * distVec.z * 2.0;
                    float dist = sqrt(distSq);
                    float3 lightModel = float3(dot(distVec, lightUniforms.mv_rows[0].xyz),
                                               dot(distVec, lightUniforms.mv_rows[1].xyz),
                                               dot(distVec, lightUniforms.mv_rows[2].xyz));
                    float3 lightIntensity = clamp(4.0 * lightModel / distSq, -1.0, 1.0);
                    float totalIntensity = clamp(dot(lightIntensity, in.shade.xyz), -1.0, 1.0);
                    float distF = floor(dist);
                    float attenuation = (distF * lightUniforms.lights[i * 3 + 1].w * 2.0 +
                                         distF * distF * lightUniforms.lights[i * 3 + 2].w / 8.0) / 65535.0 + 1.0;
                    intensity = totalIntensity / attenuation;
                } else {
                    intensity = dot(in.shade.xyz, lightUniforms.lights[i * 3 + 1].xyz) / 127.0;
                }
                @else
                    intensity = dot(in.shade.xyz, lightUniforms.lights[i * 3 + 1].xyz) / 127.0;
                @end
                if (intensity > 0.0) {
                    litColor += intensity * lightUniforms.lights[i * 3].xyz;
                }
            }
            litColor = min(litColor, float3(1.0));
            @if(o_alpha)
                out.shade = float4(litColor, in.shade.w);
            @else
                out.shade = litColor;
            @end
        @else
            out.shade = in.shade;
        @end
    @end
    out.position = in.position;
    return out;
}
// END - BEGIN FRAGMENT SHADER

float mod(float x, float y) {
    return float(x - y * floor(x / y));
}

float3 mod(float3 a, float3 b) {
    return float3(a.x - b.x * floor(a.x / b.x), a.y - b.y * floor(a.y / b.y), a.z - b.z * floor(a.z / b.z));
}

float4 mod(float4 a, float4 b) {
    return float4(a.x - b.x * floor(a.x / b.x), a.y - b.y * floor(a.y / b.y), a.z - b.z * floor(a.z / b.z), a.w - b.w * floor(a.w / b.w));
}

#define WRAP(x, low, high) mod((x)-(low), (high)-(low)) + (low)
#define TEX_OFFSET(tex, texSmplr, texCoord, off, texSize) tex.sample(texSmplr, texCoord - off / texSize)

float4 filter3point(thread const texture2d<float> tex, thread const sampler texSmplr, thread const float2& texCoord, thread const float2& texSize) {
    float2 offset = fract((texCoord * texSize) - float2(0.5));
    offset -= float2(step(1.0, offset.x + offset.y));
    float4 c0 = TEX_OFFSET(tex, texSmplr, texCoord, offset, texSize);
    float4 c1 = TEX_OFFSET(tex, texSmplr, texCoord, float2(offset.x - sign(offset.x), offset.y), texSize);
    float4 c2 = TEX_OFFSET(tex, texSmplr, texCoord, float2(offset.x, offset.y - sign(offset.y)), texSize);
    return c0 + abs(offset.x) * (c1 - c0) + abs(offset.y) * (c2 - c0);
}

float4 hookTexture2D(thread const texture2d<float> tex, thread const sampler texSmplr, thread const float2& uv, thread const float2& texSize, thread const int filtering) {
@if(o_three_point_filtering)
    if(filtering == @{FILTER_THREE_POINT}) {
        return filter3point(tex, texSmplr, uv, texSize);
    }
@end
    return tex.sample(texSmplr, uv);
}

float random(float3 value) {
    float random = dot(sin(value), float3(12.9898, 78.233, 37.719));
    return fract(sin(random) * 143758.5453);
}

fragment FragOut fragmentShader(
    ProjectedVertex in [[stage_in]],
    constant FrameUniforms &frameUniforms [[buffer(0)]],
    constant DrawUniforms &drawUniforms [[buffer(1)]]
@if(o_textures[0])
    , texture2d<float> uTex0 [[texture(0)]], sampler uTex0Smplr [[sampler(0)]]
@end
@if(o_textures[1])
    , texture2d<float> uTex1 [[texture(1)]], sampler uTex1Smplr [[sampler(1)]]
@end
@if(o_masks[0])
    , texture2d<float> uTexMask0 [[texture(2)]]
@end
@if(o_masks[1])
    , texture2d<float> uTexMask1 [[texture(3)]]
@end
@if(o_blend[0])
    , texture2d<float> uTexBlend0 [[texture(4)]]
@end
@if(o_blend[1])
    , texture2d<float> uTexBlend1 [[texture(5)]]
@end
) {
    @if(o_uses_lod)
        float lodFrac = 0.0;
    @end
    @for(i in 0..2)
        @if(o_textures[i])
            @{s = o_clamp[i][0]}
            @{t = o_clamp[i][1]}
            float2 texSize@{i} = float2(uTex@{i}.get_width(), uTex@{i}.get_height());
            @if(!s && !t)
                float2 vTexCoordAdj@{i} = in.texCoord@{i};
            @else
                @if(s && t)
                    float2 vTexCoordAdj@{i} = fast::clamp(in.texCoord@{i}, float2(0.5) / texSize@{i}, float2(in.texClampS@{i}, in.texClampT@{i}));
                @elseif(s)
                    float2 vTexCoordAdj@{i} = float2(fast::clamp(in.texCoord@{i}.x, 0.5 / texSize@{i}.x, in.texClampS@{i}), in.texCoord@{i}.y);
                @else
                    float2 vTexCoordAdj@{i} = float2(in.texCoord@{i}.x, fast::clamp(in.texCoord@{i}.y, 0.5 / texSize@{i}.y, in.texClampT@{i}));
                @end
            @end

            @if(i == 0)
                @if(o_uses_lod)
                    // N64 texture LOD: per-pixel level from the screen-space UV footprint
                    float2 lodScaled = vTexCoordAdj0 * texSize0;
                    float2 lodDx = dfdx(lodScaled);
                    float2 lodDy = dfdy(lodScaled);
                    float lodVal = max(0.5 * log2(max(max(dot(lodDx, lodDx), dot(lodDy, lodDy)), 0.000001)), 0.0);
                    float lodTile = min(floor(lodVal), drawUniforms.lod_max);
                    lodFrac = clamp(lodVal - lodTile, 0.0, 1.0);
                @end
            @end

            @if(i == 0)
                @if(o_mip_lod)
                    float4 texVal0 = uTex0.sample(uTex0Smplr, vTexCoordAdj0, level(lodTile));
                @else
                    float4 texVal@{i} = hookTexture2D(uTex@{i}, uTex@{i}Smplr, vTexCoordAdj@{i}, texSize@{i}, drawUniforms.textureFiltering[@{i}]);
                @end
            @else
                float4 texVal@{i} = hookTexture2D(uTex@{i}, uTex@{i}Smplr, vTexCoordAdj@{i}, texSize@{i}, drawUniforms.textureFiltering[@{i}]);
            @end

            @if(o_masks[i])
                float2 maskSize@{i} = float2(uTexMask@{i}.get_width(), uTexMask@{i}.get_height());
                float4 maskVal@{i} = hookTexture2D(uTexMask@{i}, uTex@{i}Smplr, vTexCoordAdj@{i}, maskSize@{i}, drawUniforms.textureFiltering[@{i}]);
                @if(o_blend[i])
                    float4 blendVal@{i} = hookTexture2D(uTexBlend@{i}, uTex@{i}Smplr, vTexCoordAdj@{i}, texSize@{i}, drawUniforms.textureFiltering[@{i}]);
                @else
                    float4 blendVal@{i} = float4(0, 0, 0, 0);
                @end

                texVal@{i} = mix(texVal@{i}, blendVal@{i}, maskVal@{i}.w);
            @end
        @end
    @end

    @if(o_mip_lod)
        // TEXEL1 reads the next mip level of texture 0
        float4 texVal1 = uTex0.sample(uTex0Smplr, vTexCoordAdj0, level(min(lodTile + 1.0, drawUniforms.lod_max)));
    @end

    @if(o_alpha)
        float4 texel;
    @else
        float3 texel;
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
                    texel.w = WRAP(texel.w, -1.01, 1.01);
                @else
                    texel.w = WRAP(texel.w, -0.51, 1.51);
                @end
            @end

            @if(o_c[c][0][2] == SHADER_COMBINED)
                texel.xyz = WRAP(texel.xyz, -1.01, 1.01);
            @else
                texel.xyz = WRAP(texel.xyz, -0.51, 1.51);
            @end
        @end

        @if(!o_color_alpha_same[c] && o_alpha)
            texel = float4(@{
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

    @if(o_texture_edge && o_alpha)
        if (texel.w > 0.19) texel.w = 1.0; else discard_fragment();
    @end

    texel = WRAP(texel, -0.51, 1.51);
    texel = clamp(texel, 0.0, 1.0);
    // TODO discard if alpha is 0?

    @if(o_fog)
        @if(o_alpha)
            texel = float4(mix(texel.xyz, drawUniforms.fog_color.xyz, in.fogFactor), texel.w);
        @else
            texel = mix(texel, drawUniforms.fog_color.xyz, in.fogFactor);
        @end
    @end

    @if(o_grayscale)
        float intensity = (texel.x + texel.y + texel.z) / 3.0;
        float3 new_texel = drawUniforms.grayscale_color.xyz * intensity;
        texel.xyz = mix(texel.xyz, new_texel, drawUniforms.grayscale_color.w);
    @end

    @if(o_alpha && o_noise)
        float2 coords = in.position.xy * frameUniforms.noiseScale;
        texel.w *= round(saturate(random(float3(floor(coords), float(frameUniforms.frameCount))) + texel.w - 0.5));
    @end

    FragOut out;
    @if(o_alpha)
        @if(o_alpha_threshold)
            if (texel.w < 8.0 / 256.0) discard_fragment();
        @end
        @if(o_invisible)
            texel.w = 0.0;
        @end
        out.color = texel;
    @else
        out.color = float4(texel, 1.0);
    @end
    @if(o_prim_depth)
        out.depth = drawUniforms.prim_depth;
    @end
    return out;
}