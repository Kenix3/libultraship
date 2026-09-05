@prism(type='fragment', name='Fast3D Fragment Shader', version='1.0.0', description='Ported shader to prism', author='Emill & Prism Team')

@{GLSL_VERSION}

@if(VERTEX_SHADER)
    @{attr} vec4 aVtxPos;
    @{attr} float aMtxSlot;
    @{update_floats(1)}

    // Matrix palette: 4 vec4 rows per slot; clip = obj.x*r0 + obj.y*r1 + obj.z*r2 + obj.w*r3
    uniform vec4 uMtxPalette[32];
    uniform vec4 uYScale;

    @for(i in 0..2)
        @if(o_textures[i])
            @{attr} vec2 aTexCoord@{i};
            @{out} vec2 vTexCoord@{i};
            @{update_floats(2)}
        @end
    @end

    @if(o_textures[0] || o_textures[1])
        uniform vec4 uUvTransform[2];
    @end

    @if(o_fog)
        @{out} float vFogFactor;
        uniform vec4 uFogParams;
    @end

    @if(o_shade || o_lighting)
        @if(o_alpha)
            @{attr} vec4 aShade;
            @{update_floats(4)}
        @else
            @{attr} vec3 aShade;
            @{update_floats(3)}
        @end
    @end
    @if(o_shade)
        @if(o_alpha)
            @{out} vec4 vShade;
        @else
            @{out} vec3 vShade;
        @end
    @end

    @if(o_lighting)
        uniform vec4 uAmbient;
        uniform int uNumLights;
        uniform vec4 uLights[96];
        @if(o_point_lighting)
            uniform vec4 uMvRows[3];
            uniform vec4 uMvCols[3];
        @end
    @end
    @if(o_texgen)
        uniform vec4 uLookatX;
        uniform vec4 uLookatY;
        @if(o_textures[0]) uniform vec4 uTexgen0;
        @if(o_textures[1]) uniform vec4 uTexgen1;
    @end

    void main() {
        // RSP position transform: matrix palette entry selected per vertex
        int mtxBase = int(aMtxSlot + 0.5) * 4;
        vec4 clipPos = aVtxPos.x * uMtxPalette[mtxBase] + aVtxPos.y * uMtxPalette[mtxBase + 1] +
                       aVtxPos.z * uMtxPalette[mtxBase + 2] + aVtxPos.w * uMtxPalette[mtxBase + 3];

        @for(i in 0..2)
            @if(o_textures[i])
                // Tile shift/origin/bilerp/size pipeline folded into one transform
                vTexCoord@{i} = vec2(aTexCoord@{i}.x * uUvTransform[@{i}].x + uUvTransform[@{i}].y,
                                     aTexCoord@{i}.y * uUvTransform[@{i}].z + uUvTransform[@{i}].w);
            @end
        @end
        @if(o_fog)
            // N64 RSP fog from clip-space z/w; uFogParams.w selects the source
            // (0: computed, 1: constant register, 2: legacy vertex-alpha fallback)
            float fogW = abs(clipPos.w) < 0.001 ? 0.001 : clipPos.w;
            float fogWinv = 1.0 / fogW;
            if (fogWinv < 0.0) {
                fogWinv = 32767.0;
            }
            float fogCalc = clamp(clipPos.z * fogWinv * uFogParams.x + uFogParams.y, 0.0, 255.0) / 255.0;
            @if(o_shade && o_alpha)
                vFogFactor = uFogParams.w > 1.5 ? aShade.a : (uFogParams.w > 0.5 ? uFogParams.z : fogCalc);
            @else
                vFogFactor = uFogParams.w > 0.5 ? uFogParams.z : fogCalc;
            @end
        @end
        @if(o_texgen)
            // N64 texgen: project the vertex normal onto the lookat vectors, then
            // run the result through the standard tile/scale UV pipeline (folded
            // into the per-draw uTexgen linear transforms).
            float texgenDotX = clamp(dot(aShade.xyz, uLookatX.xyz) / 127.0, -1.0, 1.0);
            float texgenDotY = clamp(dot(aShade.xyz, uLookatY.xyz) / 127.0, -1.0, 1.0);
            @if(o_texgen_linear)
                texgenDotX = acos(-texgenDotX) * 0.159155;
                texgenDotY = acos(-texgenDotY) * 0.159155;
            @else
                texgenDotX = (texgenDotX + 1.0) / 4.0;
                texgenDotY = (texgenDotY + 1.0) / 4.0;
            @end
            @if(o_textures[0])
                vTexCoord0 = vec2(texgenDotX * uTexgen0.x + uTexgen0.y, texgenDotY * uTexgen0.z + uTexgen0.w);
            @end
            @if(o_textures[1])
                vTexCoord1 = vec2(texgenDotX * uTexgen1.x + uTexgen1.y, texgenDotY * uTexgen1.z + uTexgen1.w);
            @end
        @end
        @if(o_shade && o_alpha)
            // Standard fog forces shade alpha to 1.0 (the factor rides vFogFactor);
            // blend-color mode (fog mode 1) preserves the vertex alpha.
            float shadeAlpha = aShade.a;
            @if(o_fog)
                if (uFogParams.w < 0.5 || uFogParams.w > 1.5) {
                    shadeAlpha = 1.0;
                }
            @end
        @end
        @if(o_point_lighting)
            // World-space position derived from the object-space vertex position
            vec3 worldPos = vec3(dot(aVtxPos, uMvCols[0]), dot(aVtxPos, uMvCols[1]), dot(aVtxPos, uMvCols[2]));
        @end
        @if(o_shade)
            @if(o_lighting)
                // N64 RSP lighting: aShade carries the raw signed vertex normal
                vec3 litColor = uAmbient.rgb;
                for (int i = 0; i < uNumLights; i++) {
                    float intensity = 0.0;
                    @if(o_point_lighting)
                    if (uLights[i * 3].w > 0.5) {
                        vec3 distVec = uLights[i * 3 + 2].xyz - worldPos;
                        float distSq = distVec.x * distVec.x + distVec.y * distVec.y + distVec.z * distVec.z * 2.0;
                        float dist = sqrt(distSq);
                        vec3 lightModel = vec3(dot(distVec, uMvRows[0].xyz), dot(distVec, uMvRows[1].xyz), dot(distVec, uMvRows[2].xyz));
                        vec3 lightIntensity = clamp(4.0 * lightModel / distSq, -1.0, 1.0);
                        float totalIntensity = clamp(dot(lightIntensity, aShade.xyz), -1.0, 1.0);
                        float distF = floor(dist);
                        float attenuation = (distF * uLights[i * 3 + 1].w * 2.0 + distF * distF * uLights[i * 3 + 2].w / 8.0) / 65535.0 + 1.0;
                        intensity = totalIntensity / attenuation;
                    } else {
                        intensity = dot(aShade.xyz, uLights[i * 3 + 1].xyz) / 127.0;
                    }
                    @else
                        intensity = dot(aShade.xyz, uLights[i * 3 + 1].xyz) / 127.0;
                    @end
                    if (intensity > 0.0) {
                        litColor += intensity * uLights[i * 3].rgb;
                    }
                }
                litColor = min(litColor, vec3(1.0));
                @if(o_alpha)
                    vShade = vec4(litColor, shadeAlpha);
                @else
                    vShade = litColor;
                @end
            @else
                @if(o_alpha)
                    vShade = vec4(aShade.rgb, shadeAlpha);
                @else
                    vShade = aShade;
                @end
            @end
        @end
        gl_Position = vec4(clipPos.x, clipPos.y * uYScale.x, clipPos.z, clipPos.w);
        @if(opengles)
            gl_Position.z *= 0.3f;
        @end
    }
@else
    @if(core_opengl || opengles)
    out vec4 vOutColor;
    @end

    @for(i in 0..2)
        @if(o_textures[i])
            @{attr} vec2 vTexCoord@{i};
        @end
    @end

    @if(o_textures[0] || o_textures[1])
        uniform vec4 uTexClamp[2];
    @end

    @if(o_fog) @{attr} float vFogFactor;

    @if(o_shade)
        @if(o_alpha)
            @{attr} vec4 vShade;
        @else
            @{attr} vec3 vShade;
        @end
    @end

    @if(o_has_inputs)
        uniform vec4 uInputs[@{o_inputs}];
    @end
    @if(o_fog)
        uniform vec4 uFogColor;
    @end
    @if(o_grayscale)
        uniform vec4 uGrayscaleColor;
    @end

    @if(o_textures[0]) uniform sampler2D uTex0;
    @if(o_textures[1]) uniform sampler2D uTex1;

    @if(o_masks[0]) uniform sampler2D uTexMask0;
    @if(o_masks[1]) uniform sampler2D uTexMask1;

    @if(o_blend[0]) uniform sampler2D uTexBlend0;
    @if(o_blend[1]) uniform sampler2D uTexBlend1;

    uniform int frame_count;
    uniform float noise_scale;

    @if(o_prim_depth)
    uniform float prim_depth;
    @end

    @if(o_uses_lod)
    uniform float lod_max;
    uniform vec4 uLodParams; // x = res scale, y = prim_lod_min, z = G_TD mode
    @end

    uniform int texture_width[2];
    uniform int texture_height[2];
    uniform int texture_filtering[2];

    #define TEX_OFFSET(off) @{texture}(tex, texCoord - off / texSize)
    #define WRAP(x, low, high) clamp((x), (low), (high))

    float random(in vec3 value) {
        float random = dot(sin(value), vec3(12.9898, 78.233, 37.719));
        return fract(sin(random) * 143758.5453);
    }

    vec4 fromLinear(vec4 linearRGB){
        bvec3 cutoff = lessThan(linearRGB.rgb, vec3(0.0031308));
        vec3 higher = vec3(1.055)*pow(linearRGB.rgb, vec3(1.0/2.4)) - vec3(0.055);
        vec3 lower = linearRGB.rgb * vec3(12.92);
        return vec4(mix(higher, lower, cutoff), linearRGB.a);
    }

    vec4 filter3point(in sampler2D tex, in vec2 texCoord, in vec2 texSize) {
        vec2 offset = fract(texCoord*texSize - vec2(0.5));
        offset -= step(1.0, offset.x + offset.y);
        vec4 c0 = TEX_OFFSET(offset);
        vec4 c1 = TEX_OFFSET(vec2(offset.x - sign(offset.x), offset.y));
        vec4 c2 = TEX_OFFSET(vec2(offset.x, offset.y - sign(offset.y)));
        return c0 + abs(offset.x)*(c1-c0) + abs(offset.y)*(c2-c0);
    }

    vec4 hookTexture2D(in int id, sampler2D tex, in vec2 uv, in vec2 texSize) {
    @if(o_three_point_filtering)
        if(texture_filtering[id] == @{FILTER_THREE_POINT}) {
            return filter3point(tex, uv, texSize);
        }
    @end
        return @{texture}(tex, uv);
    }

    @if(o_palette[0] || o_palette[1])
        uniform sampler2D uTexPal;
        uniform vec4 uPaletteParams[2];

        // One CI tap: fetch the index (nearest sampler) and look it up in the
        // 256-entry palette texture. params.x is the CI4 bank entry offset.
        vec4 paletteTap(in sampler2D tex, in vec2 uv, in float bank) {
            float idx = @{texture}(tex, uv).r;
            return @{texture}(uTexPal, vec2((idx * 255.0 + bank + 0.5) / 256.0, 0.5));
        }

        // Filtering must happen after the palette lookup, like real hardware:
        // params.y selects nearest (0), bilinear (1) or N64 three-point (2).
        vec4 paletteSample(in sampler2D tex, in vec2 uv, in vec2 texSize, in vec4 params) {
            if (params.y > 1.5) {
                vec2 offset = fract(uv * texSize - vec2(0.5));
                offset -= step(1.0, offset.x + offset.y);
                vec4 c0 = paletteTap(tex, uv - offset / texSize, params.x);
                vec4 c1 = paletteTap(tex, uv - vec2(offset.x - sign(offset.x), offset.y) / texSize, params.x);
                vec4 c2 = paletteTap(tex, uv - vec2(offset.x, offset.y - sign(offset.y)) / texSize, params.x);
                return c0 + abs(offset.x) * (c1 - c0) + abs(offset.y) * (c2 - c0);
            } else if (params.y > 0.5) {
                vec2 t = uv * texSize - 0.5;
                vec2 f = fract(t);
                vec2 base = (floor(t) + 0.5) / texSize;
                vec2 px = vec2(1.0, 0.0) / texSize;
                vec2 py = vec2(0.0, 1.0) / texSize;
                vec4 c00 = paletteTap(tex, base, params.x);
                vec4 c10 = paletteTap(tex, base + px, params.x);
                vec4 c01 = paletteTap(tex, base + py, params.x);
                vec4 c11 = paletteTap(tex, base + px + py, params.x);
                return mix(mix(c00, c10, f.x), mix(c01, c11, f.x), f.y);
            } else {
                return paletteTap(tex, uv, params.x);
            }
        }
    @end

    #define TEX_SIZE(tex) vec2(texture_width[tex], texture_height[tex])

    void main() {
        @if(o_uses_lod)
            float lodFrac = 0.0;
        @end
        @for(i in 0..2)
            @if(o_textures[i])
                @{s = o_clamp[i][0]}
                @{t = o_clamp[i][1]}

                vec2 texSize@{i} = TEX_SIZE(@{i});

                @if(!s && !t)
                    vec2 vTexCoordAdj@{i} = vTexCoord@{i};
                @else
                    @if(s && t)
                        vec2 vTexCoordAdj@{i} = clamp(vTexCoord@{i}, 0.5 / texSize@{i}, uTexClamp[@{i}].xy);
                    @elseif(s)
                        vec2 vTexCoordAdj@{i} = vec2(clamp(vTexCoord@{i}.s, 0.5 / texSize@{i}.s, uTexClamp[@{i}].x), vTexCoord@{i}.t);
                    @else
                        vec2 vTexCoordAdj@{i} = vec2(vTexCoord@{i}.s, clamp(vTexCoord@{i}.t, 0.5 / texSize@{i}.t, uTexClamp[@{i}].y));
                    @end
                @end

                @if(i == 0)
                    @if(o_uses_lod)
                        // N64 texture LOD (RDP-accurate): max absolute UV derivative,
                        // linear fraction between tiles, sharpen/detail handling
                        vec2 lodScaled = vTexCoordAdj0 * texSize0;
                        vec2 lodMaxD = max(abs(dFdx(lodScaled)), abs(dFdy(lodScaled)));
                        float lodMaxDst = max(max(lodMaxD.x, lodMaxD.y) * uLodParams.x, 0.000001);
                        if (uLodParams.z > 0.5) { // sharpen or detail
                            lodMaxDst = max(lodMaxDst, uLodParams.y);
                        }
                        float lodTileBase = floor(log2(lodMaxDst));
                        lodFrac = lodMaxDst / exp2(max(lodTileBase, 0.0)) - 1.0;
                        if (uLodParams.z > 0.5 && uLodParams.z < 1.5 && lodMaxDst < 1.0) { // sharpen
                            lodFrac = lodMaxDst - 1.0;
                        }
                        if (uLodParams.z > 1.5) { // detail: tile 0 is the detail texture
                            if (lodFrac < 0.0) {
                                lodFrac = lodMaxDst;
                            }
                            lodTileBase += 1.0;
                        } else if (lodTileBase >= lod_max) {
                            lodFrac = 1.0;
                        }
                        if (uLodParams.z > 0.5) {
                            lodTileBase = max(lodTileBase, 0.0);
                        } else {
                            lodFrac = max(lodFrac, 0.0);
                        }
                        float lodTile0 = clamp(lodTileBase, 0.0, lod_max);
                        float lodTile1 = clamp(lodTileBase + 1.0, 0.0, lod_max);
                    @end
                @end

                @if(i == 0)
                    @if(o_mip_lod)
                        vec4 texVal0 = textureLod(uTex0, vTexCoordAdj0, lodTile0);
                    @elseif(o_palette[0])
                        vec4 texVal0 = paletteSample(uTex0, vTexCoordAdj0, texSize0, uPaletteParams[0]);
                    @else
                        vec4 texVal@{i} = hookTexture2D(@{i}, uTex@{i}, vTexCoordAdj@{i}, texSize@{i});
                    @end
                @else
                    @if(o_palette[1])
                        vec4 texVal1 = paletteSample(uTex1, vTexCoordAdj1, texSize1, uPaletteParams[1]);
                    @else
                        vec4 texVal@{i} = hookTexture2D(@{i}, uTex@{i}, vTexCoordAdj@{i}, texSize@{i});
                    @end
                @end

                @if(o_masks[i])
                    @if(opengles) 
                        vec2 maskSize@{i} = vec2(textureSize(uTexMask@{i}, 0));
                    @else 
                        vec2 maskSize@{i} = textureSize(uTexMask@{i}, 0);
                    @end

                    vec4 maskVal@{i} = hookTexture2D(@{i}, uTexMask@{i}, vTexCoordAdj@{i}, maskSize@{i});

                    @if(o_blend[i])
                        vec4 blendVal@{i} = hookTexture2D(@{i}, uTexBlend@{i}, vTexCoordAdj@{i}, texSize@{i});
                    @else
                        vec4 blendVal@{i} = vec4(0, 0, 0, 0);
                    @end

                    texVal@{i} = mix(texVal@{i}, blendVal@{i}, maskVal@{i}.a);
                @end
            @end
        @end

        @if(o_mip_lod)
            // TEXEL1 reads the next mip level of texture 0
            vec4 texVal1 = textureLod(uTex0, vTexCoordAdj0, lodTile1);
        @end

        @if(o_two_tile_lod)
            // N64 LOD tile selection: TEXEL0's tile is clamped to the max level,
            // but the hardware's second texel is always lod_tile + 1 — at
            // magnification TEXEL1 still reads the real tile 1 (Paper Mario's
            // sprite shading relies on this with a stale G_TL_LOD).
            texVal0 = lodTile0 < 0.5 ? texVal0 : texVal1;
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
        // TODO discard if alpha is 0?
        @if(o_fog)
            @if(o_alpha)
                texel = vec4(mix(texel.rgb, uFogColor.rgb, vFogFactor), texel.a);
            @else
                texel = mix(texel, uFogColor.rgb, vFogFactor);
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
            vec3 new_texel = uGrayscaleColor.rgb * intensity;
            texel.rgb = mix(texel.rgb, new_texel, uGrayscaleColor.a);
        @end

        @if(o_alpha)
            @if(o_alpha_threshold)
                if (texel.a < 8.0 / 256.0) discard;
            @end
            @if(o_invisible)
                texel.a = 0.0;
            @end
            @{vOutColor} = texel;
        @else
            @{vOutColor} = vec4(texel, 1.0);
        @end

        @if(srgb_mode)
            @{vOutColor} = fromLinear(@{vOutColor});
        @end

        @if(o_prim_depth)
            gl_FragDepth = prim_depth;
        @end
    }
@end