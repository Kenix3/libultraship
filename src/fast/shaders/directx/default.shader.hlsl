@prism(type='hlsl', name='Fast3D HLSL Shader', version='1.0.0', description='Ported shader to prism', author='Emill & Prism Team')

@if(o_root_signature)
    @if(o_textures[0])
        #define RS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_VERTEX_SHADER_ROOT_ACCESS), CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), DescriptorTable(Sampler(s0), visibility = SHADER_VISIBILITY_PIXEL)"
    @end
    @if(o_textures[1])
        #define RS "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_VERTEX_SHADER_ROOT_ACCESS), CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), DescriptorTable(SRV(t1), visibility = SHADER_VISIBILITY_PIXEL), DescriptorTable(Sampler(s1), visibility = SHADER_VISIBILITY_PIXEL)"
    @end
@end

struct PSInput {
    float4 position : SV_POSITION;
@for(i in 0..2)
    @if(o_textures[i])
        float2 uv@{i} : TEXCOORD@{i};
        @{update_floats(2)}
        @for(j in 0..2)
            @if(o_clamp[i][j])
                @if(j == 0)
                    float texClampS@{i} : TEXCLAMPS@{i};
                @else
                    float texClampT@{i} : TEXCLAMPT@{i};
                @end
                @{update_floats(1)}
            @end
        @end
    @end
@end

@if(o_fog)
float fogFactor : FOG;
@{update_floats(1)}
@end

@if(o_shade)
    @if(o_alpha)
        float4 shade : SHADE;
        @{update_floats(4)}
    @else
        float3 shade : SHADE;
        @{update_floats(3)}
    @end
@end
};

@if(o_lighting || o_texgen)
// Mirrors struct LightingUniforms in gfx_rendering_api.h
cbuffer LightCB : register(b3) {
    float4 lights[96]; // 3 float4 per light: color/isPoint, dir/kc, pos/kq
    float4 ambient;
    float4 lookat_x;
    float4 lookat_y;
    float4 texgen_uv[2];
    float4 mv_rows[3];
    int num_lights;
}
@end

@if(o_textures[0]) 
    Texture2D g_texture0 : register(t0);
    SamplerState g_sampler0 : register(s0);
@end
@if(o_textures[1]) 
    Texture2D g_texture1 : register(t1);
    SamplerState g_sampler1 : register(s1);
@end

@if(o_masks[0]) Texture2D g_textureMask0 : register(t2);
@if(o_masks[1]) Texture2D g_textureMask1 : register(t3);

@if(o_blend[0]) Texture2D g_textureBlend0 : register(t4);
@if(o_blend[1]) Texture2D g_textureBlend1 : register(t5);

cbuffer PerFrameCB : register(b0) {
    uint noise_frame;
    float noise_scale;
}

float random(in float3 value) {
    float random = dot(value, float3(12.9898, 78.233, 37.719));
    return frac(sin(random) * 143758.5453);
}

// Per-draw constants: texture metadata for 3-point filtering plus the combiner
// constant operands (prim, env, keys, K4/K5, fog/grayscale colors).
// Layout must mirror struct PerDrawCB in gfx_direct3d_common.h.
cbuffer PerDrawCB : register(b1) {
    struct {
        uint width;
        uint height;
        bool linear_filtering;
    } textures[6];
    float4 combiner_inputs[6];
    float4 fog_color;
    float4 grayscale_color;
}

// 3 point texture filtering
// Original author: ArthurCarvalho
// Based on GLSL implementation by twinaphex, mupen64plus-libretro project.

@if(o_three_point_filtering && (o_textures[0] || o_textures[1]))
#define TEX_OFFSET(tex, tSampler, texCoord, off, texSize) tex.Sample(tSampler, texCoord - off / texSize)

float4 tex2D3PointFilter(in Texture2D tex, in SamplerState tSampler, in float2 texCoord, in float2 texSize) {
    float2 offset = frac(texCoord * texSize - float2(0.5, 0.5));
    offset -= step(1.0, offset.x + offset.y);
    float4 c0 = TEX_OFFSET(tex, tSampler, texCoord, offset, texSize);
    float4 c1 = TEX_OFFSET(tex, tSampler, texCoord, float2(offset.x - sign(offset.x), offset.y), texSize);
    float4 c2 = TEX_OFFSET(tex, tSampler, texCoord, float2(offset.x, offset.y - sign(offset.y)), texSize);
    return c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0);
}
@end

@if(o_prim_depth || o_uses_lod)
cbuffer PerPrimDepthCB : register(b2) {
    float prim_depth;
    float lod_max;
    float2 _pad; // pad to 16-byte cbuffer alignment
}
@end

PSInput VSMain(
    float4 position : POSITION
@for(i in 0..2)
    @if(o_textures[i])
        , float2 uv@{i} : TEXCOORD@{i}
    @end
    @for(j in 0..2)
        @if(o_clamp[i][j])
            @if(j == 0)
                , float texClampS@{i} : TEXCLAMPS@{i}
            @else
                , float texClampT@{i} : TEXCLAMPT@{i}
            @end
        @end
    @end
@end
@if(o_fog)
    , float fogFactor : FOG
@end
@if(o_shade || o_lighting)
    @if(o_alpha)
        , float4 shade : SHADE
    @else
        , float3 shade : SHADE
    @end
@end
@if(o_point_lighting)
    , float3 worldPos : WORLDPOS
@end
) {
    PSInput result;
    result.position = position;
    @for(i in 0..2)
        @if(o_textures[i])
            result.uv@{i} = uv@{i};
            @for(j in 0..2)
                @if(o_clamp[i][j])
                    @if(j == 0)
                        result.texClampS@{i} = texClampS@{i};
                    @else
                        result.texClampT@{i} = texClampT@{i};
                    @end
                @end
            @end
        @end
    @end

    @if(o_fog)
        result.fogFactor = fogFactor;
    @end

    @if(o_texgen)
        // N64 texgen: project the vertex normal onto the lookat vectors, then run
        // the result through the tile/scale UV pipeline (folded into texgen_uv).
        float texgenDotX = clamp(dot(shade.xyz, lookat_x.xyz) / 127.0, -1.0, 1.0);
        float texgenDotY = clamp(dot(shade.xyz, lookat_y.xyz) / 127.0, -1.0, 1.0);
        @if(o_texgen_linear)
            texgenDotX = acos(-texgenDotX) * 0.159155;
            texgenDotY = acos(-texgenDotY) * 0.159155;
        @else
            texgenDotX = (texgenDotX + 1.0) / 4.0;
            texgenDotY = (texgenDotY + 1.0) / 4.0;
        @end
        @if(o_textures[0])
            result.uv0 = float2(texgenDotX * texgen_uv[0].x + texgen_uv[0].y,
                                texgenDotY * texgen_uv[0].z + texgen_uv[0].w);
        @end
        @if(o_textures[1])
            result.uv1 = float2(texgenDotX * texgen_uv[1].x + texgen_uv[1].y,
                                texgenDotY * texgen_uv[1].z + texgen_uv[1].w);
        @end
    @end

    @if(o_shade)
        @if(o_lighting)
            // N64 RSP lighting: the shade input carries the raw signed vertex normal
            float3 litColor = ambient.rgb;
            for (int i = 0; i < num_lights; i++) {
                float intensity = 0.0;
                @if(o_point_lighting)
                if (lights[i * 3].w > 0.5) {
                    float3 distVec = lights[i * 3 + 2].xyz - worldPos;
                    float distSq = distVec.x * distVec.x + distVec.y * distVec.y + distVec.z * distVec.z * 2.0;
                    float dist = sqrt(distSq);
                    float3 lightModel = float3(dot(distVec, mv_rows[0].xyz), dot(distVec, mv_rows[1].xyz),
                                               dot(distVec, mv_rows[2].xyz));
                    float3 lightIntensity = clamp(4.0 * lightModel / distSq, -1.0, 1.0);
                    float totalIntensity = clamp(dot(lightIntensity, shade.xyz), -1.0, 1.0);
                    float distF = floor(dist);
                    float attenuation = (distF * lights[i * 3 + 1].w * 2.0 +
                                         distF * distF * lights[i * 3 + 2].w / 8.0) / 65535.0 + 1.0;
                    intensity = totalIntensity / attenuation;
                } else {
                    intensity = dot(shade.xyz, lights[i * 3 + 1].xyz) / 127.0;
                }
                @else
                    intensity = dot(shade.xyz, lights[i * 3 + 1].xyz) / 127.0;
                @end
                if (intensity > 0.0) {
                    litColor += intensity * lights[i * 3].rgb;
                }
            }
            litColor = min(litColor, float3(1.0, 1.0, 1.0));
            @if(o_alpha)
                result.shade = float4(litColor, shade.a);
            @else
                result.shade = litColor;
            @end
        @else
            result.shade = shade;
        @end
    @end

    return result;
}

@if(o_root_signature)
    [RootSignature(RS)]
@end

@if(srgb_mode)
    float4 fromLinear(float4 linearRGB){
        bool3 cutoff = linearRGB.rgb < float3(0.0031308, 0.0031308, 0.0031308);
        float3 higher = 1.055 * pow(linearRGB.rgb, float3(1.0 / 2.4, 1.0 / 2.4, 1.0 / 2.4)) - float3(0.055, 0.055, 0.055);
        float3 lower = linearRGB.rgb * float3(12.92, 12.92, 12.92);
        return float4(lerp(higher, lower, cutoff), linearRGB.a);
    }
@end

#define MOD(x, y) ((x) - (y) * floor((x)/(y)))
#define WRAP(x, low, high) MOD((x)-(low), (high)-(low)) + (low)

struct PSOutput {
    float4 color : SV_TARGET;
    @if(o_prim_depth)
    float depth : SV_Depth;
    @end
};

PSOutput PSMain(PSInput input, float4 screenSpace : SV_Position) {
    @if(o_uses_lod)
        float lodFrac = 0.0;
    @end
    @for(i in 0..2)
        @if(o_textures[i])
            float2 tc@{i} = input.uv@{i};
            @{s = o_clamp[i][0]}
            @{t = o_clamp[i][1]}
            @if(s || t)
                int2 texSize@{i};
                g_texture@{i}.GetDimensions(texSize@{i}.x, texSize@{i}.y);
                @if(s && t)
                    tc@{i} = clamp(tc@{i}, 0.5 / texSize@{i}, float2(input.texClampS@{i}, input.texClampT@{i}));
                @elseif(s)
                    tc@{i} = float2(clamp(tc@{i}.x, 0.5 / texSize@{i}.x, input.texClampS@{i}), tc@{i}.y);
                @else
                    tc@{i} = float2(tc@{i}.x, clamp(tc@{i}.y, 0.5 / texSize@{i}.y, input.texClampT@{i}));
                @end
            @end

            @if(i == 0)
                @if(o_uses_lod)
                    @if(!s && !t)
                        int2 texSize0;
                        g_texture0.GetDimensions(texSize0.x, texSize0.y);
                    @end
                    // N64 texture LOD: per-pixel level from the screen-space UV footprint
                    float2 lodScaled = tc0 * float2(texSize0);
                    float2 lodDx = ddx(lodScaled);
                    float2 lodDy = ddy(lodScaled);
                    float lodVal = max(0.5 * log2(max(max(dot(lodDx, lodDx), dot(lodDy, lodDy)), 0.000001)), 0.0);
                    float lodTile = min(floor(lodVal), lod_max);
                    lodFrac = clamp(lodVal - lodTile, 0.0, 1.0);
                @end
            @end

            @if(i == 0 && o_mip_lod)
                float4 texVal0 = g_texture0.SampleLevel(g_sampler0, tc0, lodTile);
                @if(o_masks[0])
                    @if(o_blend[0])
                        float4 blendVal0 = g_textureBlend0.Sample(g_sampler0, tc0);
                    @else
                        float4 blendVal0 = float4(0, 0, 0, 0);
                    @end
                    texVal0 = lerp(texVal0, blendVal0, g_textureMask0.Sample(g_sampler0, tc0).a);
                @end
            @elseif(o_three_point_filtering)
                float4 texVal@{i};
                if (textures[@{i}].linear_filtering) {
                    @if(o_masks[i])
                        texVal@{i} = tex2D3PointFilter(g_texture@{i}, g_sampler@{i}, tc@{i}, float2(textures[@{i}].width, textures[@{i}].height));
                        float2 maskSize@{i};
                        g_textureMask@{i}.GetDimensions(maskSize@{i}.x, maskSize@{i}.y);
                        float4 maskVal@{i} = tex2D3PointFilter(g_textureMask@{i}, g_sampler@{i}, tc@{i}, maskSize@{i});
                        @if(o_blend[i])
                            float4 blendVal@{i} = tex2D3PointFilter(g_textureBlend@{i}, g_sampler@{i}, tc@{i}, float2(textures[@{i}].width, textures[@{i}].height));
                        @else
                            float4 blendVal@{i} = float4(0, 0, 0, 0);
                        @end

                        texVal@{i} = lerp(texVal@{i}, blendVal@{i}, maskVal@{i}.a);
                    @else
                        texVal@{i} = tex2D3PointFilter(g_texture@{i}, g_sampler@{i}, tc@{i}, float2(textures[@{i}].width, textures[@{i}].height));
                    @end
                } else {
                    texVal@{i} = g_texture@{i}.Sample(g_sampler@{i}, tc@{i});
                    @if(o_masks[i])
                        @if(o_blend[i])
                            float4 blendVal@{i} = g_textureBlend@{i}.Sample(g_sampler@{i}, tc@{i});
                        @else
                            float4 blendVal@{i} = float4(0, 0, 0, 0);
                        @end
                        texVal@{i} = lerp(texVal@{i}, blendVal@{i}, g_textureMask@{i}.Sample(g_sampler@{i}, tc@{i}).a);
                    @end
                }
            @else
                float4 texVal@{i} = g_texture@{i}.Sample(g_sampler@{i}, tc@{i});
                @if(o_masks[i])
                    @if(o_blend[i])
                        float4 blendVal@{i} = g_textureBlend@{i}.Sample(g_sampler@{i}, tc@{i});
                    @else
                        float4 blendVal@{i} = float4(0, 0, 0, 0);
                    @end
                    texVal@{i} = lerp(texVal@{i}, blendVal@{i}, g_textureMask@{i}.Sample(g_sampler@{i}, tc@{i}).a);
                @end
            @end
        @end
    @end

    @if(o_mip_lod)
        // TEXEL1 reads the next mip level of texture 0
        float4 texVal1 = g_texture0.SampleLevel(g_sampler0, tc0, min(lodTile + 1.0, lod_max));
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
        if (texel.a > 0.19) texel.a = 1.0; else discard;
    @end

    texel = WRAP(texel, -0.51, 1.51);
    texel = clamp(texel, 0.0, 1.0);
    // TODO discard if alpha is 0?
    @if(o_fog)
        @if(o_alpha)
            texel = float4(lerp(texel.rgb, fog_color.rgb, input.fogFactor), texel.a);
        @else
            texel = lerp(texel, fog_color.rgb, input.fogFactor);
        @end
    @end

    @if(o_grayscale)
        float intensity = (texel.r + texel.g + texel.b) / 3.0;
        float3 new_texel = grayscale_color.rgb * intensity;
        texel.rgb = lerp(texel.rgb, new_texel, grayscale_color.a);
    @end

    @if(o_alpha && o_noise)
        float2 coords = screenSpace.xy * noise_scale;
        texel.a *= round(saturate(random(float3(floor(coords), noise_frame)) + texel.a - 0.5));
    @end

    @if(o_alpha)
        @if(o_alpha_threshold)
            if (texel.a < 8.0 / 256.0) discard;
        @end
        @if(o_invisible)
            texel.a = 0.0;
        @end
    @end

    PSOutput output;
    @if(o_alpha)
        @if(srgb_mode)
            output.color = fromLinear(texel);
        @else
            output.color = texel;
        @end
    @else
        @if(srgb_mode)
            output.color = fromLinear(float4(texel, 1.0));
        @else
            output.color = float4(texel, 1.0);
        @end
    @end
    @if(o_prim_depth)
        output.depth = prim_depth;
    @end
    return output;
}
