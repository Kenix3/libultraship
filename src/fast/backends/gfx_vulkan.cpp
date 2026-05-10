#include "fast/backends/gfx_vulkan.h"

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "fast/backends/gfx_sdl.h"
#include "fast/interpreter.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/resource/ResourceManager.h"
#include "ship/resource/factory/ShaderFactory.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <prism/processor.h>
#include <shaderc/shaderc.hpp>
#include <spdlog/spdlog.h>
#ifdef LUS_ENABLE_TRACY
#include <tracy/Tracy.hpp>
#endif

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef LUS_ENABLE_TRACY
#define LUS_TRACY_ZONE(name) ZoneScopedN(name)
#define LUS_TRACY_PLOT(name, value) TracyPlot(name, value)
#define LUS_TRACY_ZONE_TEXT(text) ZoneText((text).data(), (text).size())
#define LUS_TRACY_FRAME_MARK FrameMark
#else
#define LUS_TRACY_ZONE(name) ((void)0)
#define LUS_TRACY_PLOT(name, value) ((void)0)
#define LUS_TRACY_ZONE_TEXT(text) ((void)0)
#define LUS_TRACY_FRAME_MARK ((void)0)
#endif

namespace Fast {
namespace {

constexpr uint32_t ImGuiDescriptorPoolSize = 1024;
constexpr uint32_t PreferredBindlessTextureCount = 4096;
constexpr VkDeviceSize InitialVertexRingBufferSize = 1024 * 1024;

float GetZmodeDecalSlopeScaledDepthBias(uint32_t renderTargetHeight) {
    constexpr int N64ModeFactor = 120;
    constexpr int NoVanishFactor = 100;

    switch (Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_Z_FIGHTING_MODE, 0)) {
        case 1:
            return -1.0f * static_cast<float>(renderTargetHeight) / N64ModeFactor;
        case 2:
            return -1.0f * static_cast<float>(renderTargetHeight) / NoVanishFactor;
        case 0:
        default:
            return -2.0f;
    }
}

struct VulkanPushConstants {
    VkDeviceAddress vertexAddress = 0;
    VkDeviceAddress configAddress = 0;
    uint32_t frameCount = 0;
    float noiseScale = 1.0f;
};

enum VulkanUberShaderFlags : uint32_t {
    UBER_FLAG_ALPHA = 1u << 0,
    UBER_FLAG_FOG = 1u << 1,
    UBER_FLAG_TEXTURE_EDGE = 1u << 2,
    UBER_FLAG_NOISE = 1u << 3,
    UBER_FLAG_2CYC = 1u << 4,
    UBER_FLAG_ALPHA_THRESHOLD = 1u << 5,
    UBER_FLAG_INVISIBLE = 1u << 6,
    UBER_FLAG_GRAYSCALE = 1u << 7,
    UBER_FLAG_USED_TEX0 = 1u << 8,
    UBER_FLAG_USED_TEX1 = 1u << 9,
    UBER_FLAG_MASK_TEX0 = 1u << 10,
    UBER_FLAG_MASK_TEX1 = 1u << 11,
    UBER_FLAG_BLEND_TEX0 = 1u << 12,
    UBER_FLAG_BLEND_TEX1 = 1u << 13,
    UBER_FLAG_CLAMP_TEX0_S = 1u << 14,
    UBER_FLAG_CLAMP_TEX0_T = 1u << 15,
    UBER_FLAG_CLAMP_TEX1_S = 1u << 16,
    UBER_FLAG_CLAMP_TEX1_T = 1u << 17,
    UBER_FLAG_COLOR_ALPHA_SAME_C0 = 1u << 18,
    UBER_FLAG_COLOR_ALPHA_SAME_C1 = 1u << 19,
};

struct VulkanUberDrawConfig {
    uint32_t textureIds[SHADER_MAX_TEXTURES] = {};
    uint32_t textureSize[2][2] = {};
    uint32_t textureFiltering[2] = {};
    uint32_t flags = 0;
    uint32_t vertexStrideFloats = 0;
    uint32_t numInputs = 0;
    uint32_t texCoordOffset[2] = {};
    uint32_t texClampSOffset[2] = {};
    uint32_t texClampTOffset[2] = {};
    uint32_t fogOffset = 0;
    uint32_t grayscaleOffset = 0;
    uint32_t inputOffset[7] = {};
    int32_t combiner[2][2][4] = {};
};

constexpr const char* VulkanUberVertexShaderSource = R"(
#version 450
#extension GL_EXT_buffer_reference : require

layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer VertexData {
    float data[];
};

layout(buffer_reference, std430, buffer_reference_align = 8) readonly buffer DrawConfig {
    uint textureIds[6];
    uvec2 textureSize[2];
    uint textureFiltering[2];
    uint flags;
    uint vertexStrideFloats;
    uint numInputs;
    uint texCoordOffset[2];
    uint texClampSOffset[2];
    uint texClampTOffset[2];
    uint fogOffset;
    uint grayscaleOffset;
    uint inputOffset[7];
    int combiner[16];
};

layout(push_constant) uniform PushConstants {
    VertexData vertices;
    DrawConfig cfg;
    uint frameCount;
    float noiseScale;
} pc;

layout(location = 0) out vec2 vTexCoord0;
layout(location = 1) out float vTexClampS0;
layout(location = 2) out float vTexClampT0;
layout(location = 3) out vec2 vTexCoord1;
layout(location = 4) out float vTexClampS1;
layout(location = 5) out float vTexClampT1;
layout(location = 6) out vec4 vFog;
layout(location = 7) out vec4 vGrayscaleColor;
layout(location = 8) out vec4 vInput1;
layout(location = 9) out vec4 vInput2;
layout(location = 10) out vec4 vInput3;
layout(location = 11) out vec4 vInput4;
layout(location = 12) out vec4 vInput5;
layout(location = 13) out vec4 vInput6;
layout(location = 14) out vec4 vInput7;

const uint FLAG_ALPHA = 1u << 0;
const uint FLAG_FOG = 1u << 1;
const uint FLAG_GRAYSCALE = 1u << 7;
const uint FLAG_USED_TEX0 = 1u << 8;
const uint FLAG_USED_TEX1 = 1u << 9;
const uint FLAG_CLAMP_TEX0_S = 1u << 14;
const uint FLAG_CLAMP_TEX0_T = 1u << 15;
const uint FLAG_CLAMP_TEX1_S = 1u << 16;
const uint FLAG_CLAMP_TEX1_T = 1u << 17;
const uint FLAG_COLOR_ALPHA_SAME_C0 = 1u << 18;
const uint FLAG_COLOR_ALPHA_SAME_C1 = 1u << 19;

bool hasFlag(uint flag) {
    return (pc.cfg.flags & flag) != 0u;
}

float load1(uint base, uint offset) {
    return pc.vertices.data[base + offset];
}

vec2 load2(uint base, uint offset) {
    return vec2(pc.vertices.data[base + offset], pc.vertices.data[base + offset + 1u]);
}

vec4 load4(uint base, uint offset) {
    return vec4(pc.vertices.data[base + offset], pc.vertices.data[base + offset + 1u],
                pc.vertices.data[base + offset + 2u], pc.vertices.data[base + offset + 3u]);
}

vec4 loadInput(uint base, uint inputIndex) {
    if (inputIndex >= pc.cfg.numInputs) {
        return vec4(0.0);
    }

    uint offset = pc.cfg.inputOffset[inputIndex];
    vec3 rgb = vec3(pc.vertices.data[base + offset], pc.vertices.data[base + offset + 1u],
                    pc.vertices.data[base + offset + 2u]);
    float alpha = hasFlag(FLAG_ALPHA) ? pc.vertices.data[base + offset + 3u] : 1.0;
    return vec4(rgb, alpha);
}

void main() {
    uint base = uint(gl_VertexIndex) * pc.cfg.vertexStrideFloats;
    gl_Position = load4(base, 0u);

    vTexCoord0 = hasFlag(FLAG_USED_TEX0) ? load2(base, pc.cfg.texCoordOffset[0]) : vec2(0.0);
    vTexClampS0 = hasFlag(FLAG_CLAMP_TEX0_S) ? load1(base, pc.cfg.texClampSOffset[0]) : 0.0;
    vTexClampT0 = hasFlag(FLAG_CLAMP_TEX0_T) ? load1(base, pc.cfg.texClampTOffset[0]) : 0.0;

    vTexCoord1 = hasFlag(FLAG_USED_TEX1) ? load2(base, pc.cfg.texCoordOffset[1]) : vec2(0.0);
    vTexClampS1 = hasFlag(FLAG_CLAMP_TEX1_S) ? load1(base, pc.cfg.texClampSOffset[1]) : 0.0;
    vTexClampT1 = hasFlag(FLAG_CLAMP_TEX1_T) ? load1(base, pc.cfg.texClampTOffset[1]) : 0.0;

    vFog = hasFlag(FLAG_FOG) ? load4(base, pc.cfg.fogOffset) : vec4(0.0);
    vGrayscaleColor = hasFlag(FLAG_GRAYSCALE) ? load4(base, pc.cfg.grayscaleOffset) : vec4(0.0);

    vInput1 = loadInput(base, 0u);
    vInput2 = loadInput(base, 1u);
    vInput3 = loadInput(base, 2u);
    vInput4 = loadInput(base, 3u);
    vInput5 = loadInput(base, 4u);
    vInput6 = loadInput(base, 5u);
    vInput7 = loadInput(base, 6u);
}
)";

constexpr const char* VulkanUberFragmentShaderSource = R"(
#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : require

layout(location = 0) in vec2 vTexCoord0;
layout(location = 1) in float vTexClampS0;
layout(location = 2) in float vTexClampT0;
layout(location = 3) in vec2 vTexCoord1;
layout(location = 4) in float vTexClampS1;
layout(location = 5) in float vTexClampT1;
layout(location = 6) in vec4 vFog;
layout(location = 7) in vec4 vGrayscaleColor;
layout(location = 8) in vec4 vInput1;
layout(location = 9) in vec4 vInput2;
layout(location = 10) in vec4 vInput3;
layout(location = 11) in vec4 vInput4;
layout(location = 12) in vec4 vInput5;
layout(location = 13) in vec4 vInput6;
layout(location = 14) in vec4 vInput7;

layout(location = 0) out vec4 vOutColor;

layout(set = 0, binding = 0) uniform sampler2D uTextures[];

layout(buffer_reference, std430, buffer_reference_align = 8) readonly buffer DrawConfig {
    uint textureIds[6];
    uvec2 textureSize[2];
    uint textureFiltering[2];
    uint flags;
    uint vertexStrideFloats;
    uint numInputs;
    uint texCoordOffset[2];
    uint texClampSOffset[2];
    uint texClampTOffset[2];
    uint fogOffset;
    uint grayscaleOffset;
    uint inputOffset[7];
    int combiner[16];
};

layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer VertexData {
    float data[];
};

layout(push_constant) uniform PushConstants {
    VertexData vertices;
    DrawConfig cfg;
    uint frameCount;
    float noiseScale;
} pc;

const uint FILTER_THREE_POINT = 0u;

const uint FLAG_ALPHA = 1u << 0;
const uint FLAG_FOG = 1u << 1;
const uint FLAG_TEXTURE_EDGE = 1u << 2;
const uint FLAG_NOISE = 1u << 3;
const uint FLAG_2CYC = 1u << 4;
const uint FLAG_ALPHA_THRESHOLD = 1u << 5;
const uint FLAG_INVISIBLE = 1u << 6;
const uint FLAG_GRAYSCALE = 1u << 7;
const uint FLAG_USED_TEX0 = 1u << 8;
const uint FLAG_USED_TEX1 = 1u << 9;
const uint FLAG_MASK_TEX0 = 1u << 10;
const uint FLAG_MASK_TEX1 = 1u << 11;
const uint FLAG_BLEND_TEX0 = 1u << 12;
const uint FLAG_BLEND_TEX1 = 1u << 13;
const uint FLAG_CLAMP_TEX0_S = 1u << 14;
const uint FLAG_CLAMP_TEX0_T = 1u << 15;
const uint FLAG_CLAMP_TEX1_S = 1u << 16;
const uint FLAG_CLAMP_TEX1_T = 1u << 17;
const uint FLAG_COLOR_ALPHA_SAME_C0 = 1u << 18;
const uint FLAG_COLOR_ALPHA_SAME_C1 = 1u << 19;

const int SHADER_0 = 0;
const int SHADER_INPUT_1 = 1;
const int SHADER_INPUT_7 = 7;
const int SHADER_TEXEL0 = 8;
const int SHADER_TEXEL0A = 9;
const int SHADER_TEXEL1 = 10;
const int SHADER_TEXEL1A = 11;
const int SHADER_1 = 12;
const int SHADER_COMBINED = 13;
const int SHADER_NOISE = 14;

vec4 texVal0 = vec4(0.0);
vec4 texVal1 = vec4(0.0);
vec4 texel = vec4(0.0);
float randomValue = 0.0;

bool hasFlag(uint flag) {
    return (pc.cfg.flags & flag) != 0u;
}

int combinerAt(int cycle, int alpha, int index) {
    return pc.cfg.combiner[cycle * 8 + alpha * 4 + index];
}

float random(in vec3 value) {
    float v = dot(sin(value), vec3(12.9898, 78.233, 37.719));
    return fract(sin(v) * 143758.5453);
}

vec4 sampleTexture(uint textureSlot, vec2 uv) {
    return texture(uTextures[nonuniformEXT(pc.cfg.textureIds[textureSlot])], uv);
}

vec4 texOffset(uint textureSlot, vec2 texCoord, vec2 off, vec2 texSize) {
    return sampleTexture(textureSlot, texCoord - off / texSize);
}

vec4 filter3point(uint textureSlot, vec2 texCoord, vec2 texSize) {
    vec2 offset = fract(texCoord * texSize - vec2(0.5));
    offset -= step(1.0, offset.x + offset.y);
    vec4 c0 = texOffset(textureSlot, texCoord, offset, texSize);
    vec4 c1 = texOffset(textureSlot, texCoord, vec2(offset.x - sign(offset.x), offset.y), texSize);
    vec4 c2 = texOffset(textureSlot, texCoord, vec2(offset.x, offset.y - sign(offset.y)), texSize);
    return c0 + abs(offset.x) * (c1 - c0) + abs(offset.y) * (c2 - c0);
}

vec4 hookTexture2D(uint id, uint textureSlot, vec2 uv, vec2 texSize) {
    if (pc.cfg.textureFiltering[id] == FILTER_THREE_POINT) {
        return filter3point(textureSlot, uv, texSize);
    }
    return sampleTexture(textureSlot, uv);
}

vec4 inputValue(int inputId) {
    switch (inputId) {
        case SHADER_INPUT_1: return vInput1;
        case 2: return vInput2;
        case 3: return vInput3;
        case 4: return vInput4;
        case 5: return vInput5;
        case 6: return vInput6;
        case SHADER_INPUT_7: return vInput7;
        default: return vec4(0.0);
    }
}

vec4 sourceValue(int item, bool firstCycle) {
    if (item >= SHADER_INPUT_1 && item <= SHADER_INPUT_7) {
        return inputValue(item);
    }

    switch (item) {
        case SHADER_1:
            return vec4(1.0);
        case SHADER_TEXEL0:
            return firstCycle ? texVal0 : texVal1;
        case SHADER_TEXEL0A:
            return vec4(firstCycle ? texVal0.a : texVal1.a);
        case SHADER_TEXEL1:
            return firstCycle ? texVal1 : texVal0;
        case SHADER_TEXEL1A:
            return vec4(firstCycle ? texVal1.a : texVal0.a);
        case SHADER_COMBINED:
            return texel;
        case SHADER_NOISE:
            return vec4(randomValue);
        case SHADER_0:
        default:
            return vec4(0.0);
    }
}

float sourceAlpha(int item, bool firstCycle) {
    if (item >= SHADER_INPUT_1 && item <= SHADER_INPUT_7) {
        return inputValue(item).a;
    }

    switch (item) {
        case SHADER_1:
            return 1.0;
        case SHADER_TEXEL0:
        case SHADER_TEXEL0A:
            return firstCycle ? texVal0.a : texVal1.a;
        case SHADER_TEXEL1:
        case SHADER_TEXEL1A:
            return firstCycle ? texVal1.a : texVal0.a;
        case SHADER_COMBINED:
            return texel.a;
        case SHADER_NOISE:
            return randomValue;
        case SHADER_0:
        default:
            return 0.0;
    }
}

vec3 evalColorCycle(int cycle) {
    bool firstCycle = cycle == 0;
    int aItem = combinerAt(cycle, 0, 0);
    int bItem = combinerAt(cycle, 0, 1);
    int cItem = combinerAt(cycle, 0, 2);
    int dItem = combinerAt(cycle, 0, 3);

    if (cItem == SHADER_0) {
        return sourceValue(dItem, firstCycle).rgb;
    }
    if (bItem == SHADER_0 && dItem == SHADER_0) {
        return sourceValue(aItem, firstCycle).rgb * sourceValue(cItem, firstCycle).rgb;
    }
    if (bItem == dItem) {
        return mix(sourceValue(bItem, firstCycle).rgb, sourceValue(aItem, firstCycle).rgb,
                   sourceValue(cItem, firstCycle).rgb);
    }
    return (sourceValue(aItem, firstCycle).rgb - sourceValue(bItem, firstCycle).rgb) *
               sourceValue(cItem, firstCycle).rgb +
           sourceValue(dItem, firstCycle).rgb;
}

float evalAlphaCycle(int cycle) {
    bool firstCycle = cycle == 0;
    int aItem = combinerAt(cycle, 1, 0);
    int bItem = combinerAt(cycle, 1, 1);
    int cItem = combinerAt(cycle, 1, 2);
    int dItem = combinerAt(cycle, 1, 3);

    if (cItem == SHADER_0) {
        return sourceAlpha(dItem, firstCycle);
    }
    if (bItem == SHADER_0 && dItem == SHADER_0) {
        return sourceAlpha(aItem, firstCycle) * sourceAlpha(cItem, firstCycle);
    }
    if (bItem == dItem) {
        return mix(sourceAlpha(bItem, firstCycle), sourceAlpha(aItem, firstCycle), sourceAlpha(cItem, firstCycle));
    }
    return (sourceAlpha(aItem, firstCycle) - sourceAlpha(bItem, firstCycle)) * sourceAlpha(cItem, firstCycle) +
           sourceAlpha(dItem, firstCycle);
}

vec4 evalVectorCycle(int cycle) {
    bool firstCycle = cycle == 0;
    int aItem = combinerAt(cycle, 0, 0);
    int bItem = combinerAt(cycle, 0, 1);
    int cItem = combinerAt(cycle, 0, 2);
    int dItem = combinerAt(cycle, 0, 3);

    if (cItem == SHADER_0) {
        return sourceValue(dItem, firstCycle);
    }
    if (bItem == SHADER_0 && dItem == SHADER_0) {
        return sourceValue(aItem, firstCycle) * sourceValue(cItem, firstCycle);
    }
    if (bItem == dItem) {
        return mix(sourceValue(bItem, firstCycle), sourceValue(aItem, firstCycle), sourceValue(cItem, firstCycle));
    }
    return (sourceValue(aItem, firstCycle) - sourceValue(bItem, firstCycle)) * sourceValue(cItem, firstCycle) +
           sourceValue(dItem, firstCycle);
}

void main() {
    randomValue = (random(vec3(floor(gl_FragCoord.xy * pc.noiseScale), float(pc.frameCount))) + 1.0) / 2.0;

    if (hasFlag(FLAG_USED_TEX0)) {
        vec2 texSize = vec2(pc.cfg.textureSize[0]);
        vec2 uv = vTexCoord0;
        if (hasFlag(FLAG_CLAMP_TEX0_S)) {
            uv.s = clamp(uv.s, 0.5 / texSize.s, vTexClampS0);
        }
        if (hasFlag(FLAG_CLAMP_TEX0_T)) {
            uv.t = clamp(uv.t, 0.5 / texSize.t, vTexClampT0);
        }

        texVal0 = hookTexture2D(0u, 0u, uv, texSize);
        if (hasFlag(FLAG_MASK_TEX0)) {
            vec2 maskSize = vec2(textureSize(uTextures[nonuniformEXT(pc.cfg.textureIds[2])], 0));
            vec4 maskVal = hookTexture2D(0u, 2u, uv, maskSize);
            vec4 blendVal = hasFlag(FLAG_BLEND_TEX0) ? hookTexture2D(0u, 4u, uv, texSize) : vec4(0.0);
            texVal0 = mix(texVal0, blendVal, maskVal.a);
        }
    }

    if (hasFlag(FLAG_USED_TEX1)) {
        vec2 texSize = vec2(pc.cfg.textureSize[1]);
        vec2 uv = vTexCoord1;
        if (hasFlag(FLAG_CLAMP_TEX1_S)) {
            uv.s = clamp(uv.s, 0.5 / texSize.s, vTexClampS1);
        }
        if (hasFlag(FLAG_CLAMP_TEX1_T)) {
            uv.t = clamp(uv.t, 0.5 / texSize.t, vTexClampT1);
        }

        texVal1 = hookTexture2D(1u, 1u, uv, texSize);
        if (hasFlag(FLAG_MASK_TEX1)) {
            vec2 maskSize = vec2(textureSize(uTextures[nonuniformEXT(pc.cfg.textureIds[3])], 0));
            vec4 maskVal = hookTexture2D(1u, 3u, uv, maskSize);
            vec4 blendVal = hasFlag(FLAG_BLEND_TEX1) ? hookTexture2D(1u, 5u, uv, texSize) : vec4(0.0);
            texVal1 = mix(texVal1, blendVal, maskVal.a);
        }
    }

    int cycles = hasFlag(FLAG_2CYC) ? 2 : 1;
    for (int cycle = 0; cycle < cycles; cycle++) {
        if (cycle == 1) {
            if (combinerAt(cycle, 1, 2) == SHADER_COMBINED) {
                texel.a = mod(texel.a + 1.01, 2.02) - 1.01;
            } else {
                texel.a = mod(texel.a + 0.51, 2.02) - 0.51;
            }

            if (combinerAt(cycle, 0, 2) == SHADER_COMBINED) {
                texel.rgb = mod(texel.rgb + vec3(1.01), vec3(2.02)) - vec3(1.01);
            } else {
                texel.rgb = mod(texel.rgb + vec3(0.51), vec3(2.02)) - vec3(0.51);
            }
        }

        bool colorAlphaSame = cycle == 0 ? hasFlag(FLAG_COLOR_ALPHA_SAME_C0) : hasFlag(FLAG_COLOR_ALPHA_SAME_C1);
        if (hasFlag(FLAG_ALPHA) && colorAlphaSame) {
            texel = evalVectorCycle(cycle);
        } else {
            texel.rgb = evalColorCycle(cycle);
            texel.a = hasFlag(FLAG_ALPHA) ? evalAlphaCycle(cycle) : 1.0;
        }
    }

    texel = mod(texel + vec4(0.51), vec4(2.02)) - vec4(0.51);
    texel = clamp(texel, 0.0, 1.0);

    if (hasFlag(FLAG_FOG)) {
        texel.rgb = mix(texel.rgb, vFog.rgb, vFog.a);
    }

    if (hasFlag(FLAG_TEXTURE_EDGE) && hasFlag(FLAG_ALPHA)) {
        if (texel.a > 0.19) {
            texel.a = 1.0;
        } else {
            discard;
        }
    }

    if (hasFlag(FLAG_ALPHA) && hasFlag(FLAG_NOISE)) {
        texel.a *= floor(clamp(randomValue + texel.a, 0.0, 1.0));
    }

    if (hasFlag(FLAG_GRAYSCALE)) {
        float intensity = (texel.r + texel.g + texel.b) / 3.0;
        vec3 newTexel = vGrayscaleColor.rgb * intensity;
        texel.rgb = mix(texel.rgb, newTexel, vGrayscaleColor.a);
    }

    if (hasFlag(FLAG_ALPHA)) {
        if (hasFlag(FLAG_ALPHA_THRESHOLD) && texel.a < 8.0 / 256.0) {
            discard;
        }
        if (hasFlag(FLAG_INVISIBLE)) {
            texel.a = 0.0;
        }
        vOutColor = texel;
    } else {
        vOutColor = vec4(texel.rgb, 1.0);
    }
}
)";

void CheckImGuiVkResult(VkResult result) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error("ImGui Vulkan backend call failed with VkResult " + std::to_string(result));
    }
}

void CheckVk(VkResult result, const char* message) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(message) + " (VkResult " + std::to_string(result) + ")");
    }
}

struct VulkanImageUsageState {
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags2 stageMask = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 accessMask = VK_ACCESS_2_NONE;
};

VulkanImageUsageState GetImageUsageState(VulkanImageUsage usage) {
    switch (usage) {
        case VulkanImageUsage::ColorAttachment:
            return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT };
        case VulkanImageUsage::DepthAttachment:
            return { VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                     VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                         VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT };
        case VulkanImageUsage::ShaderRead:
            return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                     VK_ACCESS_2_SHADER_SAMPLED_READ_BIT };
        case VulkanImageUsage::TransferSrc:
            return { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT,
                     VK_ACCESS_2_TRANSFER_READ_BIT };
        case VulkanImageUsage::TransferDst:
            return { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT,
                     VK_ACCESS_2_TRANSFER_WRITE_BIT };
        case VulkanImageUsage::Present:
            return { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE };
    }

    throw std::runtime_error("Unsupported Vulkan image usage");
}

VkSamplerAddressMode GfxCmToVulkan(uint32_t value) {
    switch (value) {
        case G_TX_NOMIRROR | G_TX_CLAMP:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case G_TX_MIRROR | G_TX_WRAP:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case G_TX_MIRROR | G_TX_CLAMP:
            return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        case G_TX_NOMIRROR | G_TX_WRAP:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        default:
            throw std::runtime_error("Unsupported Vulkan texture address mode");
    }
}

static size_t rawNumFloats = 0;
static uint32_t vertexAttributeLocation = 0;
static uint32_t varyingLocation = 0;

bool GetBool(prism::ContextTypes* value) {
    if (std::holds_alternative<int>(*value)) {
        return std::get<int>(*value) == 1;
    }
    return false;
}

#define RAND_NOISE "((random(vec3(floor(gl_FragCoord.xy * noiseScale), float(frameCount))) + 1.0) / 2.0)"

const char* ShaderItemToStr(uint32_t item, bool withAlpha, bool onlyAlpha, bool inputsHaveAlpha, bool firstCycle,
                            bool hintSingleElement) {
    if (!onlyAlpha) {
        switch (item) {
            default:
            case SHADER_0:
                return withAlpha ? "vec4(0.0, 0.0, 0.0, 0.0)" : "vec3(0.0, 0.0, 0.0)";
            case SHADER_1:
                return withAlpha ? "vec4(1.0, 1.0, 1.0, 1.0)" : "vec3(1.0, 1.0, 1.0)";
            case SHADER_INPUT_1:
                return withAlpha || !inputsHaveAlpha ? "vInput1" : "vInput1.rgb";
            case SHADER_INPUT_2:
                return withAlpha || !inputsHaveAlpha ? "vInput2" : "vInput2.rgb";
            case SHADER_INPUT_3:
                return withAlpha || !inputsHaveAlpha ? "vInput3" : "vInput3.rgb";
            case SHADER_INPUT_4:
                return withAlpha || !inputsHaveAlpha ? "vInput4" : "vInput4.rgb";
            case SHADER_TEXEL0:
                return firstCycle ? (withAlpha ? "texVal0" : "texVal0.rgb") : (withAlpha ? "texVal1" : "texVal1.rgb");
            case SHADER_TEXEL0A:
                return firstCycle ? (hintSingleElement ? "texVal0.a"
                                                       : (withAlpha ? "vec4(texVal0.a)" : "vec3(texVal0.a)"))
                                  : (hintSingleElement ? "texVal1.a"
                                                       : (withAlpha ? "vec4(texVal1.a)" : "vec3(texVal1.a)"));
            case SHADER_TEXEL1A:
                return firstCycle ? (hintSingleElement ? "texVal1.a"
                                                       : (withAlpha ? "vec4(texVal1.a)" : "vec3(texVal1.a)"))
                                  : (hintSingleElement ? "texVal0.a"
                                                       : (withAlpha ? "vec4(texVal0.a)" : "vec3(texVal0.a)"));
            case SHADER_TEXEL1:
                return firstCycle ? (withAlpha ? "texVal1" : "texVal1.rgb") : (withAlpha ? "texVal0" : "texVal0.rgb");
            case SHADER_COMBINED:
                return withAlpha ? "texel" : "texel.rgb";
            case SHADER_NOISE:
                return withAlpha ? "vec4(" RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ")"
                                 : "vec3(" RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ")";
        }
    }

    switch (item) {
        default:
        case SHADER_0:
            return "0.0";
        case SHADER_1:
            return "1.0";
        case SHADER_INPUT_1:
            return "vInput1.a";
        case SHADER_INPUT_2:
            return "vInput2.a";
        case SHADER_INPUT_3:
            return "vInput3.a";
        case SHADER_INPUT_4:
            return "vInput4.a";
        case SHADER_TEXEL0:
        case SHADER_TEXEL0A:
            return firstCycle ? "texVal0.a" : "texVal1.a";
        case SHADER_TEXEL1:
        case SHADER_TEXEL1A:
            return firstCycle ? "texVal1.a" : "texVal0.a";
        case SHADER_COMBINED:
            return "texel.a";
        case SHADER_NOISE:
            return RAND_NOISE;
    }
}

#undef RAND_NOISE

prism::ContextTypes* AppendFormula(prism::ContextTypes*, prism::ContextTypes* arg, prism::ContextTypes* single,
                                   prism::ContextTypes* mult, prism::ContextTypes* mix,
                                   prism::ContextTypes* withAlphaArg, prism::ContextTypes* onlyAlphaArg,
                                   prism::ContextTypes* alphaArg, prism::ContextTypes* firstCycleArg) {
    auto c = std::get<prism::MTDArray<int>>(*arg);
    bool doSingle = GetBool(single);
    bool doMultiply = GetBool(mult);
    bool doMix = GetBool(mix);
    bool withAlpha = GetBool(withAlphaArg);
    bool onlyAlpha = GetBool(onlyAlphaArg);
    bool optAlpha = GetBool(alphaArg);
    bool firstCycle = GetBool(firstCycleArg);

    std::string out;
    if (doSingle) {
        out += ShaderItemToStr(c.at(onlyAlpha, 3), withAlpha, onlyAlpha, optAlpha, firstCycle, false);
    } else if (doMultiply) {
        out += ShaderItemToStr(c.at(onlyAlpha, 0), withAlpha, onlyAlpha, optAlpha, firstCycle, false);
        out += " * ";
        out += ShaderItemToStr(c.at(onlyAlpha, 2), withAlpha, onlyAlpha, optAlpha, firstCycle, true);
    } else if (doMix) {
        out += "mix(";
        out += ShaderItemToStr(c.at(onlyAlpha, 1), withAlpha, onlyAlpha, optAlpha, firstCycle, false);
        out += ", ";
        out += ShaderItemToStr(c.at(onlyAlpha, 0), withAlpha, onlyAlpha, optAlpha, firstCycle, false);
        out += ", ";
        out += ShaderItemToStr(c.at(onlyAlpha, 2), withAlpha, onlyAlpha, optAlpha, firstCycle, true);
        out += ")";
    } else {
        out += "(";
        out += ShaderItemToStr(c.at(onlyAlpha, 0), withAlpha, onlyAlpha, optAlpha, firstCycle, false);
        out += " - ";
        out += ShaderItemToStr(c.at(onlyAlpha, 1), withAlpha, onlyAlpha, optAlpha, firstCycle, false);
        out += ") * ";
        out += ShaderItemToStr(c.at(onlyAlpha, 2), withAlpha, onlyAlpha, optAlpha, firstCycle, true);
        out += " + ";
        out += ShaderItemToStr(c.at(onlyAlpha, 3), withAlpha, onlyAlpha, optAlpha, firstCycle, false);
    }
    return new prism::ContextTypes{ out };
}

prism::ContextTypes* NextVertexAttributeLocation(prism::ContextTypes*, prism::ContextTypes* numFloats) {
    uint32_t location = vertexAttributeLocation++;
    rawNumFloats += std::get<int>(*numFloats);
    return new prism::ContextTypes{ static_cast<int>(location) };
}

prism::ContextTypes* NextVaryingLocation() {
    return new prism::ContextTypes{ static_cast<int>(varyingLocation++) };
}

std::optional<std::string> VulkanIncludeFs(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = static_cast<uint32_t>(Ship::ResourceType::Shader);
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    auto res = std::static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));
    if (res == nullptr) {
        return std::nullopt;
    }

    auto inc = static_cast<std::string*>(res->GetRawPointer());
    return *inc;
}

[[maybe_unused]] std::string BuildVulkanShaderSource(const char* resourcePath, const CCFeatures& ccFeatures,
                                                     bool vertexShader) {
    if (vertexShader) {
        rawNumFloats = 4;
        vertexAttributeLocation = 1;
    }
    varyingLocation = 0;

    prism::Processor processor;
    prism::ContextItems context = {
        { "SHADER_0", SHADER_0 },
        { "SHADER_INPUT_1", SHADER_INPUT_1 },
        { "SHADER_INPUT_2", SHADER_INPUT_2 },
        { "SHADER_INPUT_3", SHADER_INPUT_3 },
        { "SHADER_INPUT_4", SHADER_INPUT_4 },
        { "SHADER_INPUT_5", SHADER_INPUT_5 },
        { "SHADER_INPUT_6", SHADER_INPUT_6 },
        { "SHADER_INPUT_7", SHADER_INPUT_7 },
        { "SHADER_TEXEL0", SHADER_TEXEL0 },
        { "SHADER_TEXEL0A", SHADER_TEXEL0A },
        { "SHADER_TEXEL1", SHADER_TEXEL1 },
        { "SHADER_TEXEL1A", SHADER_TEXEL1A },
        { "SHADER_1", SHADER_1 },
        { "SHADER_COMBINED", SHADER_COMBINED },
        { "SHADER_NOISE", SHADER_NOISE },
        { "FILTER_THREE_POINT", Fast::FILTER_THREE_POINT },
        { "o_c", M_ARRAY(ccFeatures.c, int, 2, 2, 4) },
        { "o_alpha", ccFeatures.opt_alpha },
        { "o_fog", ccFeatures.opt_fog },
        { "o_texture_edge", ccFeatures.opt_texture_edge },
        { "o_noise", ccFeatures.opt_noise },
        { "o_2cyc", ccFeatures.opt_2cyc },
        { "o_alpha_threshold", ccFeatures.opt_alpha_threshold },
        { "o_invisible", ccFeatures.opt_invisible },
        { "o_grayscale", ccFeatures.opt_grayscale },
        { "o_textures", M_ARRAY(ccFeatures.usedTextures, bool, 2) },
        { "o_masks", M_ARRAY(ccFeatures.used_masks, bool, 2) },
        { "o_blend", M_ARRAY(ccFeatures.used_blend, bool, 2) },
        { "o_clamp", M_ARRAY(ccFeatures.clamp, bool, 2, 2) },
        { "o_inputs", ccFeatures.numInputs },
        { "o_do_mix", M_ARRAY(ccFeatures.do_mix, bool, 2, 2) },
        { "o_do_single", M_ARRAY(ccFeatures.do_single, bool, 2, 2) },
        { "o_do_multiply", M_ARRAY(ccFeatures.do_multiply, bool, 2, 2) },
        { "o_color_alpha_same", M_ARRAY(ccFeatures.color_alpha_same, bool, 2) },
        { "o_three_point_filtering", true },
        { "append_formula", reinterpret_cast<InvokeFunc>(AppendFormula) },
        { "next_attrib_location", reinterpret_cast<InvokeFunc>(NextVertexAttributeLocation) },
        { "next_varying_location", reinterpret_cast<InvokeFunc>(NextVaryingLocation) },
    };
    processor.populate(context);

    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = static_cast<uint32_t>(Ship::ResourceType::Shader);
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    auto res = std::static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(resourcePath, true, init));
    if (res == nullptr) {
        SPDLOG_ERROR("Failed to load Vulkan shader template {}, missing f3d.o2r?", resourcePath);
        abort();
    }

    auto shader = static_cast<std::string*>(res->GetRawPointer());
    processor.load(*shader);
    processor.bind_include_loader(VulkanIncludeFs);
    return processor.process();
}

std::vector<uint32_t> CompileVulkanGlslToSpirv(const std::string& source, shaderc_shader_kind shaderKind,
                                               const char* sourceName) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetTargetSpirv(shaderc_spirv_version_1_6);
#ifdef _DEBUG
    options.SetGenerateDebugInfo();
    options.SetOptimizationLevel(shaderc_optimization_level_zero);
#else
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
#endif

    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, shaderKind, sourceName, options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error(std::string("Failed to compile Vulkan shader ") + sourceName + ": " +
                                 result.GetErrorMessage());
    }

    return { result.cbegin(), result.cend() };
}

VkShaderModule CreateShaderModule(VkDevice device, const std::vector<uint32_t>& spirv, const char* shaderName) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    CheckVk(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), shaderName);
    return shaderModule;
}

VkFormat VertexFormatForFloatCount(uint32_t floatCount) {
    switch (floatCount) {
        case 1:
            return VK_FORMAT_R32_SFLOAT;
        case 2:
            return VK_FORMAT_R32G32_SFLOAT;
        case 3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case 4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:
            throw std::runtime_error("Unsupported Vulkan vertex attribute size");
    }
}

void AddVertexAttribute(std::vector<VkVertexInputAttributeDescription>& attributes, uint32_t location,
                        uint32_t floatCount, uint32_t& offset) {
    VkVertexInputAttributeDescription attribute = {};
    attribute.location = location;
    attribute.binding = 0;
    attribute.format = VertexFormatForFloatCount(floatCount);
    attribute.offset = offset;
    attributes.push_back(attribute);
    offset += floatCount * sizeof(float);
}

[[maybe_unused]] std::vector<VkVertexInputAttributeDescription> BuildVertexAttributes(const CCFeatures& ccFeatures) {
    std::vector<VkVertexInputAttributeDescription> attributes;
    attributes.reserve(16);

    uint32_t location = 0;
    uint32_t offset = 0;
    AddVertexAttribute(attributes, location++, 4, offset);

    for (uint32_t texture = 0; texture < 2; texture++) {
        if (!ccFeatures.usedTextures[texture]) {
            continue;
        }

        AddVertexAttribute(attributes, location++, 2, offset);
        if (ccFeatures.clamp[texture][0]) {
            AddVertexAttribute(attributes, location++, 1, offset);
        }
        if (ccFeatures.clamp[texture][1]) {
            AddVertexAttribute(attributes, location++, 1, offset);
        }
    }

    if (ccFeatures.opt_fog) {
        AddVertexAttribute(attributes, location++, 4, offset);
    }
    if (ccFeatures.opt_grayscale) {
        AddVertexAttribute(attributes, location++, 4, offset);
    }

    for (int input = 0; input < ccFeatures.numInputs; input++) {
        AddVertexAttribute(attributes, location++, ccFeatures.opt_alpha ? 4 : 3, offset);
    }

    return attributes;
}

} // namespace

VkDeviceSize VulkanVertexRingBuffer::AlignUp(VkDeviceSize value, VkDeviceSize alignment) {
    if (alignment <= 1) {
        return value;
    }
    return (value + alignment - 1) & ~(alignment - 1);
}

void VulkanVertexRingBuffer::Init(VkDevice device, VmaAllocator_T* allocator, VkDeviceSize initialSize) {
    mDevice = device;
    mAllocator = allocator;
    mCurrent = CreateBuffer(initialSize);
}

void VulkanVertexRingBuffer::Destroy() {
    DestroyBuffer(mCurrent);
    for (auto& buffer : mRetiredBuffers) {
        DestroyBuffer(buffer);
    }
    mRetiredBuffers.clear();
    mCurrentRanges.clear();
    mDevice = VK_NULL_HANDLE;
    mAllocator = nullptr;
}

void VulkanVertexRingBuffer::BeginFrame() {
    CollectCompletedRanges();
    mFrameAllocated = false;
    if (!CurrentBufferIsInUse()) {
        mCurrent.head = 0;
    }
}

void VulkanVertexRingBuffer::EndFrame(VkSemaphore timelineSemaphore, uint64_t timelineValue) {
    if (!mFrameAllocated || timelineSemaphore == VK_NULL_HANDLE || timelineValue == 0) {
        return;
    }
    mCurrentRanges.push_back({ timelineSemaphore, timelineValue });
}

VulkanVertexRingBuffer::Allocation VulkanVertexRingBuffer::Allocate(VkDeviceSize size, VkDeviceSize alignment) {
    if (mAllocator == nullptr || mCurrent.buffer == VK_NULL_HANDLE) {
        throw std::runtime_error("Vulkan vertex ring buffer is not initialized");
    }

    VkDeviceSize alignedHead = AlignUp(mCurrent.head, alignment);
    if (size > mCurrent.size || alignedHead + size > mCurrent.size) {
        if (!CurrentBufferIsInUse()) {
            mCurrent.head = 0;
            alignedHead = 0;
        } else {
            Grow(size);
            alignedHead = 0;
        }
    }

    if (size > mCurrent.size || alignedHead + size > mCurrent.size) {
        throw std::runtime_error("Vulkan vertex ring buffer allocation is larger than the backing buffer");
    }

    mFrameAllocated = true;
    mCurrent.head = alignedHead + size;
    return { mCurrent.buffer, alignedHead, mCurrent.deviceAddress + alignedHead,
             static_cast<uint8_t*>(mCurrent.mapped) + alignedHead };
}

VulkanVertexRingBuffer::Buffer VulkanVertexRingBuffer::CreateBuffer(VkDeviceSize size) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    Buffer buffer;
    buffer.size = size;
    VmaAllocationInfo allocationInfo = {};
    CheckVk(vmaCreateBuffer(mAllocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &allocationInfo),
            "Failed to create Vulkan vertex ring buffer");
    buffer.mapped = allocationInfo.pMappedData;
    VkBufferDeviceAddressInfo addressInfo = {};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = buffer.buffer;
    buffer.deviceAddress = vkGetBufferDeviceAddress(mDevice, &addressInfo);
    if (buffer.deviceAddress == 0) {
        throw std::runtime_error("Failed to get Vulkan vertex ring buffer device address");
    }
    return buffer;
}

void VulkanVertexRingBuffer::DestroyBuffer(Buffer& buffer) {
    if (buffer.buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(mAllocator, buffer.buffer, buffer.allocation);
    }
    buffer = {};
}

void VulkanVertexRingBuffer::Grow(VkDeviceSize requiredSize) {
    VkDeviceSize newSize = std::max(requiredSize, std::max(mCurrent.size * 2, InitialVertexRingBufferSize));
    if (mCurrent.buffer != VK_NULL_HANDLE) {
        mRetiredBuffers.push_back(mCurrent);
    }
    mCurrent = CreateBuffer(newSize);
    mCurrentRanges.clear();
}

bool VulkanVertexRingBuffer::CurrentBufferIsInUse() const {
    return !mCurrentRanges.empty();
}

void VulkanVertexRingBuffer::CollectCompletedRanges() {
    if (mDevice == VK_NULL_HANDLE) {
        return;
    }

    auto it = mCurrentRanges.begin();
    while (it != mCurrentRanges.end()) {
        uint64_t completedValue = 0;
        VkResult result = vkGetSemaphoreCounterValue(mDevice, it->timelineSemaphore, &completedValue);
        if (result != VK_SUCCESS || completedValue < it->timelineValue) {
            ++it;
            continue;
        }
        it = mCurrentRanges.erase(it);
    }
}

GfxRenderingAPIVulkan::GfxRenderingAPIVulkan(GfxWindowBackendSDL2* windowBackend) : mWindowBackend(windowBackend) {
}

GfxRenderingAPIVulkan::~GfxRenderingAPIVulkan() {
    DestroyVulkanObjects();
}

const char* GfxRenderingAPIVulkan::GetName() {
    return "Vulkan";
}

int GfxRenderingAPIVulkan::GetMaxTextureSize() {
    return 8192;
}

GfxClipParameters GfxRenderingAPIVulkan::GetClipParameters() {
    return { true, false };
}

void GfxRenderingAPIVulkan::UnloadShader(ShaderProgram* oldPrg) {
}

void GfxRenderingAPIVulkan::LoadShader(ShaderProgram* newPrg) {
    mShaderProgram = reinterpret_cast<VulkanShaderProgram*>(newPrg);
}

ShaderProgram* GfxRenderingAPIVulkan::CreateAndLoadNewShader(uint64_t shaderId0, uint32_t shaderId1) {
    LUS_TRACY_ZONE("Vulkan CreateAndLoadNewShader");
    mTracyShaderCreatesThisFrame++;

#ifdef LUS_ENABLE_TRACY
    const std::string tracyShaderKey =
        "shaderId0=" + std::to_string(shaderId0) + " shaderId1=" + std::to_string(shaderId1);
    LUS_TRACY_ZONE_TEXT(tracyShaderKey);
#endif

    CCFeatures ccFeatures;
    {
        LUS_TRACY_ZONE("Vulkan Shader GetFeatures");
        gfx_cc_get_features(shaderId0, shaderId1, &ccFeatures);
    }

    auto key = std::make_pair(shaderId0, shaderId1);
    decltype(mShaderProgramPool)::iterator it;
    {
        LUS_TRACY_ZONE("Vulkan Shader Pool Insert");
        auto [insertedIt, _] = mShaderProgramPool.emplace(key, std::make_unique<VulkanShaderProgram>());
        it = insertedIt;
    }
    VulkanShaderProgram* prg = it->second.get();
    prg->numInputs = ccFeatures.numInputs;
    prg->usedTextures[0] = ccFeatures.usedTextures[0];
    prg->usedTextures[1] = ccFeatures.usedTextures[1];
    prg->usedTextures[2] = ccFeatures.used_masks[0];
    prg->usedTextures[3] = ccFeatures.used_masks[1];
    prg->usedTextures[4] = ccFeatures.used_blend[0];
    prg->usedTextures[5] = ccFeatures.used_blend[1];

    prg->flags = 0;
    prg->flags |= ccFeatures.opt_alpha ? UBER_FLAG_ALPHA : 0;
    prg->flags |= ccFeatures.opt_fog ? UBER_FLAG_FOG : 0;
    prg->flags |= ccFeatures.opt_texture_edge ? UBER_FLAG_TEXTURE_EDGE : 0;
    prg->flags |= ccFeatures.opt_noise ? UBER_FLAG_NOISE : 0;
    prg->flags |= ccFeatures.opt_2cyc ? UBER_FLAG_2CYC : 0;
    prg->flags |= ccFeatures.opt_alpha_threshold ? UBER_FLAG_ALPHA_THRESHOLD : 0;
    prg->flags |= ccFeatures.opt_invisible ? UBER_FLAG_INVISIBLE : 0;
    prg->flags |= ccFeatures.opt_grayscale ? UBER_FLAG_GRAYSCALE : 0;
    prg->flags |= ccFeatures.usedTextures[0] ? UBER_FLAG_USED_TEX0 : 0;
    prg->flags |= ccFeatures.usedTextures[1] ? UBER_FLAG_USED_TEX1 : 0;
    prg->flags |= ccFeatures.used_masks[0] ? UBER_FLAG_MASK_TEX0 : 0;
    prg->flags |= ccFeatures.used_masks[1] ? UBER_FLAG_MASK_TEX1 : 0;
    prg->flags |= ccFeatures.used_blend[0] ? UBER_FLAG_BLEND_TEX0 : 0;
    prg->flags |= ccFeatures.used_blend[1] ? UBER_FLAG_BLEND_TEX1 : 0;
    prg->flags |= ccFeatures.clamp[0][0] ? UBER_FLAG_CLAMP_TEX0_S : 0;
    prg->flags |= ccFeatures.clamp[0][1] ? UBER_FLAG_CLAMP_TEX0_T : 0;
    prg->flags |= ccFeatures.clamp[1][0] ? UBER_FLAG_CLAMP_TEX1_S : 0;
    prg->flags |= ccFeatures.clamp[1][1] ? UBER_FLAG_CLAMP_TEX1_T : 0;
    prg->flags |= ccFeatures.color_alpha_same[0] ? UBER_FLAG_COLOR_ALPHA_SAME_C0 : 0;
    prg->flags |= ccFeatures.color_alpha_same[1] ? UBER_FLAG_COLOR_ALPHA_SAME_C1 : 0;

    uint32_t offset = 4;
    for (uint32_t texture = 0; texture < 2; texture++) {
        if (!ccFeatures.usedTextures[texture]) {
            continue;
        }
        prg->texCoordOffset[texture] = offset;
        offset += 2;
        if (ccFeatures.clamp[texture][0]) {
            prg->texClampSOffset[texture] = offset++;
        }
        if (ccFeatures.clamp[texture][1]) {
            prg->texClampTOffset[texture] = offset++;
        }
    }
    if (ccFeatures.opt_fog) {
        prg->fogOffset = offset;
        offset += 4;
    }
    if (ccFeatures.opt_grayscale) {
        prg->grayscaleOffset = offset;
        offset += 4;
    }
    for (int input = 0; input < ccFeatures.numInputs && input < 7; input++) {
        prg->inputOffset[input] = offset;
        offset += ccFeatures.opt_alpha ? 4 : 3;
    }
    prg->numFloats = offset;

    std::memcpy(prg->combiner, ccFeatures.c, sizeof(prg->combiner));
    prg->pipelineLayout = mUberPipelineLayout;
    prg->pipeline = ccFeatures.opt_alpha ? mUberAlphaPipeline : mUberOpaquePipeline;

    LoadShader(reinterpret_cast<ShaderProgram*>(prg));
    return reinterpret_cast<ShaderProgram*>(it->second.get());
}

ShaderProgram* GfxRenderingAPIVulkan::LookupShader(uint64_t shaderId0, uint32_t shaderId1) {
    auto it = mShaderProgramPool.find(std::make_pair(shaderId0, shaderId1));
    if (it == mShaderProgramPool.end()) {
        return nullptr;
    }
    return reinterpret_cast<ShaderProgram*>(it->second.get());
}

void GfxRenderingAPIVulkan::ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) {
    auto* vulkanPrg = reinterpret_cast<VulkanShaderProgram*>(prg);
    *numInputs = vulkanPrg != nullptr ? vulkanPrg->numInputs : 0;
    usedTextures[0] = vulkanPrg != nullptr ? vulkanPrg->usedTextures[0] : false;
    usedTextures[1] = vulkanPrg != nullptr ? vulkanPrg->usedTextures[1] : false;
}

uint32_t GfxRenderingAPIVulkan::NewTexture() {
    if (mTextures.size() >= mMaxBindlessTextures) {
        throw std::runtime_error("Vulkan bindless texture descriptor array is full");
    }
    mTextures.emplace_back();
    return static_cast<uint32_t>(mTextures.size() - 1);
}

void GfxRenderingAPIVulkan::SelectTexture(int tile, uint32_t textureId) {
    if (tile < 0 || tile >= SHADER_MAX_TEXTURES) {
        throw std::runtime_error("Invalid Vulkan texture tile index");
    }
    GetTexture(textureId);
    mCurrentTile = tile;
    mCurrentTextureIds[tile] = textureId;
}

void GfxRenderingAPIVulkan::UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) {
    LUS_TRACY_ZONE("Vulkan UploadTexture");
    if (rgba32Buf == nullptr || width == 0 || height == 0) {
        throw std::runtime_error("Cannot upload empty Vulkan texture");
    }

    mTracyTextureUploadsThisFrame++;
    uint32_t textureId = mCurrentTextureIds[mCurrentTile];
    VulkanTexture& texture = GetTexture(textureId);
    UploadTextureToGpu(texture, rgba32Buf, width, height);
    WriteBindlessTextureDescriptor(textureId);
}

void GfxRenderingAPIVulkan::SetSamplerParameters(int sampler, bool linearFilter, uint32_t cms, uint32_t cmt) {
    LUS_TRACY_ZONE("Vulkan SetSamplerParameters");
    if (sampler < 0 || sampler >= SHADER_MAX_TEXTURES) {
        throw std::runtime_error("Invalid Vulkan sampler tile index");
    }

    mTracySamplerRecreatesThisFrame++;
    VulkanTexture& texture = GetTexture(mCurrentTextureIds[sampler]);
    texture.linearFiltering = linearFilter;
    texture.filtering = !linearFilter ? FILTER_LINEAR : FILTER_THREE_POINT;
    texture.cms = cms;
    texture.cmt = cmt;

    if (texture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(mDevice, texture.sampler, nullptr);
        texture.sampler = VK_NULL_HANDLE;
    }

    VkFilter filter = linearFilter && mCurrentFilterMode == FILTER_LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.mipmapMode =
        filter == VK_FILTER_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = GfxCmToVulkan(cms);
    samplerInfo.addressModeV = GfxCmToVulkan(cmt);
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    CheckVk(vkCreateSampler(mDevice, &samplerInfo, nullptr, &texture.sampler),
            "Failed to create Vulkan texture sampler");

    if (texture.uploaded) {
        WriteBindlessTextureDescriptor(mCurrentTextureIds[sampler]);
    }
}

void GfxRenderingAPIVulkan::SetDepthTestAndMask(bool depth_test, bool z_upd) {
    mCurrentDepthTest = depth_test;
    mCurrentDepthMask = z_upd;
}

void GfxRenderingAPIVulkan::SetZmodeDecal(bool decal) {
    mCurrentZmodeDecal = decal;
}

void GfxRenderingAPIVulkan::SetViewport(int x, int y, int width, int height) {
    if (!mFrameActive || !mRenderingActive || mCurrentCommandBuffer == VK_NULL_HANDLE) {
        return;
    }

    uint32_t renderTargetHeight = mCurrentRenderTargetHeight != 0 ? mCurrentRenderTargetHeight : mSwapchainExtent.height;
    VkViewport viewport = {};
    viewport.x = static_cast<float>(x);
    viewport.y = static_cast<float>(static_cast<int>(renderTargetHeight) - y);
    viewport.width = static_cast<float>(width);
    viewport.height = -static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(mCurrentCommandBuffer, 0, 1, &viewport);
}

void GfxRenderingAPIVulkan::SetScissor(int x, int y, int width, int height) {
    if (!mFrameActive || !mRenderingActive || mCurrentCommandBuffer == VK_NULL_HANDLE) {
        return;
    }

    uint32_t renderTargetWidth = mCurrentRenderTargetWidth != 0 ? mCurrentRenderTargetWidth : mSwapchainExtent.width;
    uint32_t renderTargetHeight = mCurrentRenderTargetHeight != 0 ? mCurrentRenderTargetHeight : mSwapchainExtent.height;
    int scissorX = std::max(0, std::min(x, static_cast<int>(renderTargetWidth)));
    int scissorY = std::max(0, std::min(static_cast<int>(renderTargetHeight) - y - height,
                                        static_cast<int>(renderTargetHeight)));
    VkRect2D scissor = {};
    scissor.offset.x = scissorX;
    scissor.offset.y = scissorY;
    scissor.extent.width = static_cast<uint32_t>(std::max(0, std::min<int>(width, renderTargetWidth - scissor.offset.x)));
    scissor.extent.height =
        static_cast<uint32_t>(std::max(0, std::min<int>(height, renderTargetHeight - scissor.offset.y)));
    vkCmdSetScissor(mCurrentCommandBuffer, 0, 1, &scissor);
}

void GfxRenderingAPIVulkan::SetUseAlpha(bool useAlpha) {
}

void GfxRenderingAPIVulkan::DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    LUS_TRACY_ZONE("Vulkan DrawTriangles");
    if (!mFrameActive || !mRenderingActive || mCurrentCommandBuffer == VK_NULL_HANDLE || mShaderProgram == nullptr ||
        buf_vbo_len == 0 || buf_vbo_num_tris == 0) {
        return;
    }

    mTracyDrawCallsThisFrame++;
    VkDeviceSize vertexBufferSize = static_cast<VkDeviceSize>(buf_vbo_len * sizeof(float));
    auto allocation = mVertexRingBuffer.Allocate(vertexBufferSize, alignof(float));
    std::memcpy(allocation.mapped, buf_vbo, static_cast<size_t>(vertexBufferSize));

    VulkanUberDrawConfig drawConfig = {};
    for (uint32_t i = 0; i < SHADER_MAX_TEXTURES; i++) {
        drawConfig.textureIds[i] = mCurrentTextureIds[i];
    }
    for (uint32_t i = 0; i < 2; i++) {
        if (mCurrentTextureIds[i] < mTextures.size()) {
            VulkanTexture& texture = GetTexture(mCurrentTextureIds[i]);
            drawConfig.textureSize[i][0] = std::max(texture.width, 1u);
            drawConfig.textureSize[i][1] = std::max(texture.height, 1u);
            drawConfig.textureFiltering[i] = texture.filtering;
        } else {
            drawConfig.textureSize[i][0] = 1;
            drawConfig.textureSize[i][1] = 1;
            drawConfig.textureFiltering[i] = FILTER_LINEAR;
        }
    }
    drawConfig.flags = mShaderProgram->flags;
    drawConfig.vertexStrideFloats = static_cast<uint32_t>(mShaderProgram->numFloats);
    drawConfig.numInputs = mShaderProgram->numInputs;
    std::memcpy(drawConfig.texCoordOffset, mShaderProgram->texCoordOffset, sizeof(drawConfig.texCoordOffset));
    std::memcpy(drawConfig.texClampSOffset, mShaderProgram->texClampSOffset, sizeof(drawConfig.texClampSOffset));
    std::memcpy(drawConfig.texClampTOffset, mShaderProgram->texClampTOffset, sizeof(drawConfig.texClampTOffset));
    drawConfig.fogOffset = mShaderProgram->fogOffset;
    drawConfig.grayscaleOffset = mShaderProgram->grayscaleOffset;
    std::memcpy(drawConfig.inputOffset, mShaderProgram->inputOffset, sizeof(drawConfig.inputOffset));
    std::memcpy(drawConfig.combiner, mShaderProgram->combiner, sizeof(drawConfig.combiner));

    auto configAllocation = mVertexRingBuffer.Allocate(sizeof(drawConfig), 16);
    std::memcpy(configAllocation.mapped, &drawConfig, sizeof(drawConfig));

    VulkanPushConstants pushConstants = {};
    pushConstants.vertexAddress = allocation.deviceAddress;
    pushConstants.configAddress = configAllocation.deviceAddress;
    pushConstants.frameCount = mFrameCount;
    pushConstants.noiseScale = mCurrentNoiseScale;

    vkCmdBindPipeline(mCurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mShaderProgram->pipeline);
    vkCmdSetDepthTestEnable(mCurrentCommandBuffer,
                            (mCurrentDepthTest || mCurrentDepthMask) ? VK_TRUE : VK_FALSE);
    vkCmdSetDepthWriteEnable(mCurrentCommandBuffer, mCurrentDepthMask ? VK_TRUE : VK_FALSE);
    vkCmdSetDepthCompareOp(mCurrentCommandBuffer,
                           mCurrentDepthTest
                               ? (mCurrentZmodeDecal ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_LESS)
                               : VK_COMPARE_OP_ALWAYS);
    uint32_t renderTargetHeight = mCurrentRenderTargetHeight != 0 ? mCurrentRenderTargetHeight : mSwapchainExtent.height;
    vkCmdSetDepthBias(mCurrentCommandBuffer, 0.0f, 0.0f,
                      mCurrentZmodeDecal ? GetZmodeDecalSlopeScaledDepthBias(renderTargetHeight) : 0.0f);
    vkCmdBindDescriptorSets(mCurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mShaderProgram->pipelineLayout, 0, 1,
                            &mTextureDescriptorSet, 0, nullptr);
    vkCmdPushConstants(mCurrentCommandBuffer, mShaderProgram->pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(pushConstants), &pushConstants);
    vkCmdDraw(mCurrentCommandBuffer, static_cast<uint32_t>(buf_vbo_num_tris * 3), 1, 0, 0);
}

void GfxRenderingAPIVulkan::Init() {
    if (mWindowBackend == nullptr || mWindowBackend->GetWindow() == nullptr) {
        throw std::runtime_error("Vulkan backend requires an initialized SDL window");
    }

    mInstance = Vulkan::CreateInstance(mWindowBackend->GetWindow());
    mDebugMessenger = Vulkan::CreateDebugMessenger(mInstance);
    mSurface = Vulkan::CreateSurface(mInstance, mWindowBackend->GetWindow());

    auto deviceSelection = Vulkan::PickPhysicalDevice(mInstance, mSurface);
    mPhysicalDevice = deviceSelection.physicalDevice;
    mQueueFamilies = deviceSelection.queueFamilies;
    mDevice = Vulkan::CreateLogicalDevice(mPhysicalDevice, mQueueFamilies, &mGraphicsQueue, &mPresentQueue);
    CreateAllocator();
    mVertexRingBuffer.Init(mDevice, mAllocator, InitialVertexRingBufferSize);
    CreateTextureDescriptorResources();
    CreateSwapchain();
    CreateImageViews();
    CreateDepthResources();
    CreateUberShaderPipeline();
    CreateCommandPool();
    CreateUploadCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();

    CreateFramebuffer();
}

void GfxRenderingAPIVulkan::OnResize() {
    mFramebufferResized = true;
}

void GfxRenderingAPIVulkan::StartFrame() {
    LUS_TRACY_ZONE("Vulkan StartFrame");
    if (mSwapchain == VK_NULL_HANDLE || mFrameActive) {
        return;
    }

    mTracyDrawCallsThisFrame = 0;
    mTracyTextureUploadsThisFrame = 0;
    mTracySamplerRecreatesThisFrame = 0;
    mTracyShaderCreatesThisFrame = 0;
    mTracyImmediateSubmitsThisFrame = 0;

    mCurrentFrame = &mFrames[mCurrentFrameIndex];
    if (mCurrentFrame->renderFinishedTimelineValue > 0) {
        LUS_TRACY_ZONE("Vulkan Wait Frame Timeline");
        VkSemaphoreWaitInfo waitInfo = {};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &mCurrentFrame->renderFinishedTimelineSemaphore;
        waitInfo.pValues = &mCurrentFrame->renderFinishedTimelineValue;
        CheckVk(vkWaitSemaphores(mDevice, &waitInfo, UINT64_MAX), "Failed to wait for Vulkan frame timeline");
    }

    mFrameCount++;
    {
        LUS_TRACY_ZONE("Vulkan VertexRing BeginFrame");
        mVertexRingBuffer.BeginFrame();
    }

    VkResult acquireResult = VK_SUCCESS;
    {
        LUS_TRACY_ZONE("Vulkan AcquireNextImage");
        acquireResult = vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX,
                                              mCurrentFrame->imageAvailableSemaphore, VK_NULL_HANDLE,
                                              &mCurrentImageIndex);
    }
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        mCurrentFrame = nullptr;
        RecreateSwapchain();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire Vulkan swapchain image");
    }
    mSwapchainImageStageMasks[mCurrentImageIndex] = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    mSwapchainImageAccessMasks[mCurrentImageIndex] = VK_ACCESS_2_NONE;

    mCurrentCommandBuffer = mCurrentFrame->commandBuffer;
    {
        LUS_TRACY_ZONE("Vulkan ResetCommandBuffer");
        vkResetCommandBuffer(mCurrentCommandBuffer, 0);
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    {
        LUS_TRACY_ZONE("Vulkan BeginCommandBuffer");
        if (vkBeginCommandBuffer(mCurrentCommandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin Vulkan command buffer");
        }
    }

    mFrameActive = true;
    mRenderingActive = false;
    mCurrentFramebuffer = 0;
    mCurrentRenderTargetWidth = mSwapchainExtent.width;
    mCurrentRenderTargetHeight = mSwapchainExtent.height;
}

void GfxRenderingAPIVulkan::EndFrame() {
}

void GfxRenderingAPIVulkan::FinishRender() {
    LUS_TRACY_ZONE("Vulkan FinishRender");
    if (mSwapchain == VK_NULL_HANDLE || !mFrameActive) {
        return;
    }

    EndCurrentRendering();

    TransitionImageUsage(mSwapchainImages[mCurrentImageIndex], VK_IMAGE_ASPECT_COLOR_BIT,
                         mSwapchainImageLayouts[mCurrentImageIndex], mSwapchainImageStageMasks[mCurrentImageIndex],
                         mSwapchainImageAccessMasks[mCurrentImageIndex], VulkanImageUsage::Present);

    if (vkEndCommandBuffer(mCurrentCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record Vulkan command buffer");
    }
    mFrameActive = false;

    VkCommandBufferSubmitInfo commandBufferInfo = {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferInfo.commandBuffer = mCurrentCommandBuffer;

    VkSemaphoreSubmitInfo waitSemaphoreInfo = {};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.semaphore = mCurrentFrame->imageAvailableSemaphore;
    waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    mCurrentFrame->renderFinishedTimelineValue++;
    mVertexRingBuffer.EndFrame(mCurrentFrame->renderFinishedTimelineSemaphore, mCurrentFrame->renderFinishedTimelineValue);

    VkSemaphore renderFinishedSemaphore = mSwapchainRenderFinishedSemaphores[mCurrentImageIndex];
    std::array<VkSemaphoreSubmitInfo, 2> signalSemaphoreInfos = {};
    signalSemaphoreInfos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfos[0].semaphore = renderFinishedSemaphore;
    signalSemaphoreInfos[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    signalSemaphoreInfos[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfos[1].semaphore = mCurrentFrame->renderFinishedTimelineSemaphore;
    signalSemaphoreInfos[1].value = mCurrentFrame->renderFinishedTimelineValue;
    signalSemaphoreInfos[1].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkSubmitInfo2 submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandBufferInfo;
    submitInfo.signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreInfos.size());
    submitInfo.pSignalSemaphoreInfos = signalSemaphoreInfos.data();

    {
        LUS_TRACY_ZONE("Vulkan QueueSubmit Frame");
        if (vkQueueSubmit2(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit Vulkan command buffer");
        }
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &mSwapchain;
    presentInfo.pImageIndices = &mCurrentImageIndex;

    VkResult presentResult = VK_SUCCESS;
    {
        LUS_TRACY_ZONE("Vulkan QueuePresent");
        presentResult = vkQueuePresentKHR(mPresentQueue, &presentInfo);
    }
    mCurrentCommandBuffer = VK_NULL_HANDLE;
    mCurrentFrame = nullptr;
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % FRAMES_IN_FLIGHT;
    LUS_TRACY_PLOT("Vulkan draw calls/frame", static_cast<int64_t>(mTracyDrawCallsThisFrame));
    LUS_TRACY_PLOT("Vulkan texture uploads/frame", static_cast<int64_t>(mTracyTextureUploadsThisFrame));
    LUS_TRACY_PLOT("Vulkan sampler recreates/frame", static_cast<int64_t>(mTracySamplerRecreatesThisFrame));
    LUS_TRACY_PLOT("Vulkan shader creates/frame", static_cast<int64_t>(mTracyShaderCreatesThisFrame));
    LUS_TRACY_PLOT("Vulkan immediate submits/frame", static_cast<int64_t>(mTracyImmediateSubmitsThisFrame));
    LUS_TRACY_FRAME_MARK;
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || mFramebufferResized) {
        mFramebufferResized = false;
        RecreateSwapchain();
        return;
    }
    if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present Vulkan swapchain image");
    }
}

int GfxRenderingAPIVulkan::CreateFramebuffer() {
    VulkanFramebuffer framebuffer;
    framebuffer.colorTextureId = NewTexture();
    mFramebuffers.push_back(framebuffer);
    return static_cast<int>(mFramebuffers.size() - 1);
}

void GfxRenderingAPIVulkan::UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                                        bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                                        bool can_extract_depth) {
    if (fb_id < 0) {
        return;
    }
    if (static_cast<size_t>(fb_id) >= mFramebuffers.size()) {
        mFramebuffers.resize(static_cast<size_t>(fb_id) + 1);
    }

    VulkanFramebuffer& framebuffer = mFramebuffers[fb_id];
    if (framebuffer.colorTextureId == UINT32_MAX || framebuffer.colorTextureId >= mTextures.size()) {
        framebuffer.colorTextureId = NewTexture();
    }

    width = std::max(width, 1U);
    height = std::max(height, 1U);
    // The current Vulkan shader pipeline is single-sample. Keep the API functional and resolve-compatible
    // until pipelines are keyed by sample count.
    msaa_level = 1;

    bool colorChanged = framebuffer.width != width || framebuffer.height != height ||
                        framebuffer.msaaLevel != msaa_level || framebuffer.renderTarget != render_target ||
                        framebuffer.colorImage == VK_NULL_HANDLE;
    bool depthChanged = framebuffer.width != width || framebuffer.height != height ||
                        framebuffer.msaaLevel != msaa_level || framebuffer.hasDepthBuffer != has_depth_buffer ||
                        framebuffer.canExtractDepth != can_extract_depth;

    framebuffer.width = width;
    framebuffer.height = height;
    framebuffer.msaaLevel = msaa_level;
    framebuffer.openglInvertY = opengl_invertY;
    framebuffer.renderTarget = render_target;
    framebuffer.hasDepthBuffer = has_depth_buffer;
    framebuffer.canExtractDepth = can_extract_depth;

    if (fb_id == 0) {
        return;
    }

    if (colorChanged) {
        DestroyFramebufferResources(framebuffer);
        CreateFramebufferColorResources(framebuffer);
    }

    if (!has_depth_buffer) {
        if (framebuffer.depthImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(mDevice, framebuffer.depthImageView, nullptr);
            framebuffer.depthImageView = VK_NULL_HANDLE;
        }
        if (framebuffer.depthImage != VK_NULL_HANDLE) {
            vmaDestroyImage(mAllocator, framebuffer.depthImage, framebuffer.depthAllocation);
            framebuffer.depthImage = VK_NULL_HANDLE;
            framebuffer.depthAllocation = nullptr;
        }
        framebuffer.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        return;
    }

    if (depthChanged || framebuffer.depthImage == VK_NULL_HANDLE) {
        if (framebuffer.depthImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(mDevice, framebuffer.depthImageView, nullptr);
        }
        if (framebuffer.depthImage != VK_NULL_HANDLE) {
            vmaDestroyImage(mAllocator, framebuffer.depthImage, framebuffer.depthAllocation);
        }
        framebuffer.depthImage = VK_NULL_HANDLE;
        framebuffer.depthAllocation = nullptr;
        framebuffer.depthImageView = VK_NULL_HANDLE;
        framebuffer.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        CreateFramebufferDepthResources(framebuffer);
    }
}

void GfxRenderingAPIVulkan::StartDrawToFramebuffer(int fbId, float noiseScale) {
    if (!mFrameActive || mCurrentCommandBuffer == VK_NULL_HANDLE) {
        return;
    }

    EndCurrentRendering();
    mCurrentFramebuffer = std::max(0, fbId);
    VkExtent2D extent = GetFramebufferExtent(mCurrentFramebuffer);
    mCurrentRenderTargetWidth = extent.width;
    mCurrentRenderTargetHeight = extent.height;
    mCurrentNoiseScale = noiseScale != 0.0f ? 1.0f / noiseScale : 1.0f;
    BeginRenderingToCurrentFramebuffer();
}

void GfxRenderingAPIVulkan::CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1,
                                            int dstX0, int dstY0, int dstX1, int dstY1) {
    if (!mFrameActive || mCurrentCommandBuffer == VK_NULL_HANDLE || fbSrcId < 0 || fbDstId < 0 ||
        static_cast<size_t>(fbSrcId) >= mFramebuffers.size() || static_cast<size_t>(fbDstId) >= mFramebuffers.size()) {
        return;
    }

    VkImage srcImage = GetFramebufferColorImage(fbSrcId);
    VkImage dstImage = GetFramebufferColorImage(fbDstId);
    if (srcImage == VK_NULL_HANDLE || dstImage == VK_NULL_HANDLE) {
        return;
    }

    EndCurrentRendering();

    VulkanFramebuffer* srcFb = GetFramebuffer(fbSrcId);
    VulkanFramebuffer* dstFb = GetFramebuffer(fbDstId);
    if (srcFb != nullptr && !srcFb->openglInvertY) {
        int height = srcY1 - srcY0;
        srcY1 = static_cast<int>(srcFb->height) - srcY0;
        srcY0 = srcY1 - height;
    }
    if (srcFb != nullptr && dstFb != nullptr && srcFb->openglInvertY != dstFb->openglInvertY) {
        std::swap(srcY0, srcY1);
    }

    VkExtent2D srcExtent = GetFramebufferExtent(fbSrcId);
    VkExtent2D dstExtent = GetFramebufferExtent(fbDstId);
    int srcMinX = std::min(srcX0, srcX1);
    int srcMaxX = std::max(srcX0, srcX1);
    int srcMinY = std::min(srcY0, srcY1);
    int srcMaxY = std::max(srcY0, srcY1);
    int dstMinX = std::min(dstX0, dstX1);
    int dstMaxX = std::max(dstX0, dstX1);
    int dstMinY = std::min(dstY0, dstY1);
    int dstMaxY = std::max(dstY0, dstY1);
    if (srcMinX < 0 || srcMinY < 0 || dstMinX < 0 || dstMinY < 0 ||
        srcMaxX > static_cast<int>(srcExtent.width) || srcMaxY > static_cast<int>(srcExtent.height) ||
        dstMaxX > static_cast<int>(dstExtent.width) || dstMaxY > static_cast<int>(dstExtent.height)) {
        BeginRenderingToCurrentFramebuffer();
        return;
    }

    VkImageLayout& srcLayout = GetFramebufferColorLayout(fbSrcId);
    VkImageLayout& dstLayout = GetFramebufferColorLayout(fbDstId);
    VkPipelineStageFlags2& srcStageMask = GetFramebufferColorStageMask(fbSrcId);
    VkPipelineStageFlags2& dstStageMask = GetFramebufferColorStageMask(fbDstId);
    VkAccessFlags2& srcAccessMask = GetFramebufferColorAccessMask(fbSrcId);
    VkAccessFlags2& dstAccessMask = GetFramebufferColorAccessMask(fbDstId);
    TransitionImageUsage(srcImage, VK_IMAGE_ASPECT_COLOR_BIT, srcLayout, srcStageMask, srcAccessMask,
                         VulkanImageUsage::TransferSrc);
    TransitionImageUsage(dstImage, VK_IMAGE_ASPECT_COLOR_BIT, dstLayout, dstStageMask, dstAccessMask,
                         VulkanImageUsage::TransferDst);

    if ((srcX1 - srcX0) == (dstX1 - dstX0) && (srcY1 - srcY0) == (dstY1 - dstY0)) {
        VkImageCopy2 copyRegion = {};
        copyRegion.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2;
        copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.srcOffset = { srcX0, srcY0, 0 };
        copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.dstOffset = { dstX0, dstY0, 0 };
        copyRegion.extent = { static_cast<uint32_t>(std::abs(srcX1 - srcX0)),
                              static_cast<uint32_t>(std::abs(srcY1 - srcY0)), 1 };

        VkCopyImageInfo2 copyInfo = {};
        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2;
        copyInfo.srcImage = srcImage;
        copyInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        copyInfo.dstImage = dstImage;
        copyInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copyInfo.regionCount = 1;
        copyInfo.pRegions = &copyRegion;
        vkCmdCopyImage2(mCurrentCommandBuffer, &copyInfo);
    } else {
        VkImageBlit2 blitRegion = {};
        blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
        blitRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        blitRegion.srcOffsets[0] = { srcX0, srcY0, 0 };
        blitRegion.srcOffsets[1] = { srcX1, srcY1, 1 };
        blitRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        blitRegion.dstOffsets[0] = { dstX0, dstY0, 0 };
        blitRegion.dstOffsets[1] = { dstX1, dstY1, 1 };

        VkBlitImageInfo2 blitInfo = {};
        blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
        blitInfo.srcImage = srcImage;
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitInfo.dstImage = dstImage;
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitInfo.regionCount = 1;
        blitInfo.pRegions = &blitRegion;
        blitInfo.filter = VK_FILTER_NEAREST;
        vkCmdBlitImage2(mCurrentCommandBuffer, &blitInfo);
    }

    if (fbDstId == static_cast<int>(mCurrentFramebuffer)) {
        TransitionImageUsage(dstImage, VK_IMAGE_ASPECT_COLOR_BIT, dstLayout, dstStageMask, dstAccessMask,
                             VulkanImageUsage::ColorAttachment);
    } else if (dstFb != nullptr && fbDstId != 0 && dstFb->colorTextureId < mTextures.size()) {
        VulkanTexture& texture = mTextures[dstFb->colorTextureId];
        TransitionImageUsage(dstImage, VK_IMAGE_ASPECT_COLOR_BIT, dstLayout, dstStageMask, dstAccessMask,
                             VulkanImageUsage::ShaderRead);
        texture.layout = dstLayout;
        WriteBindlessTextureDescriptor(dstFb->colorTextureId);
    }

    if (srcFb != nullptr && srcFb->colorTextureId < mTextures.size() && fbSrcId != static_cast<int>(mCurrentFramebuffer)) {
        VulkanTexture& texture = mTextures[srcFb->colorTextureId];
        TransitionImageUsage(srcImage, VK_IMAGE_ASPECT_COLOR_BIT, srcLayout, srcStageMask, srcAccessMask,
                             VulkanImageUsage::ShaderRead);
        texture.layout = srcLayout;
        WriteBindlessTextureDescriptor(srcFb->colorTextureId);
    }

    BeginRenderingToCurrentFramebuffer();
}

void GfxRenderingAPIVulkan::ClearFramebuffer(bool color, bool depth) {
    if (!mFrameActive || !mRenderingActive || mCurrentCommandBuffer == VK_NULL_HANDLE || (!color && !depth)) {
        return;
    }
    std::array<VkClearAttachment, 2> clearAttachments = {};
    uint32_t clearAttachmentCount = 0;
    if (color) {
        clearAttachments[clearAttachmentCount].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clearAttachments[clearAttachmentCount].colorAttachment = 0;
        clearAttachments[clearAttachmentCount].clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        clearAttachmentCount++;
    }
    if (depth) {
        bool hasDepth = mCurrentFramebuffer == 0 ? mDepthImageView != VK_NULL_HANDLE
                                                 : (GetFramebuffer(mCurrentFramebuffer) != nullptr &&
                                                    GetFramebuffer(mCurrentFramebuffer)->depthImageView != VK_NULL_HANDLE);
        if (hasDepth) {
            clearAttachments[clearAttachmentCount].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            clearAttachments[clearAttachmentCount].clearValue.depthStencil = { 1.0f, 0 };
            clearAttachmentCount++;
        }
    }
    if (clearAttachmentCount == 0) {
        return;
    }

    VkClearRect clearRect = {};
    clearRect.rect.offset = { 0, 0 };
    clearRect.rect.extent = { mCurrentRenderTargetWidth, mCurrentRenderTargetHeight };
    clearRect.baseArrayLayer = 0;
    clearRect.layerCount = 1;
    vkCmdClearAttachments(mCurrentCommandBuffer, clearAttachmentCount, clearAttachments.data(), 1, &clearRect);
}

void GfxRenderingAPIVulkan::ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) {
    if (rgba16Buf != nullptr) {
        std::memset(rgba16Buf, 0, width * height * sizeof(uint16_t));
    }
}

void GfxRenderingAPIVulkan::ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) {
    CopyFramebuffer(fbIdTarger, fbIdSrc, 0, 0, static_cast<int>(GetFramebufferExtent(fbIdSrc).width),
                    static_cast<int>(GetFramebufferExtent(fbIdSrc).height), 0, 0,
                    static_cast<int>(GetFramebufferExtent(fbIdTarger).width),
                    static_cast<int>(GetFramebufferExtent(fbIdTarger).height));
}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
GfxRenderingAPIVulkan::GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) {
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> depths;
    for (const auto& coordinate : coordinates) {
        depths[coordinate] = 0;
    }
    return depths;
}

void* GfxRenderingAPIVulkan::GetFramebufferTextureId(int fbId) {
    VulkanFramebuffer* framebuffer = GetFramebuffer(fbId);
    if (framebuffer == nullptr || framebuffer->colorTextureId >= mTextures.size()) {
        return nullptr;
    }
    EnsureImGuiTextureDescriptor(framebuffer->colorTextureId);
    return reinterpret_cast<void*>(GetTexture(framebuffer->colorTextureId).imguiDescriptorSet);
}

void GfxRenderingAPIVulkan::SelectTextureFb(int fbId) {
    VulkanFramebuffer* framebuffer = GetFramebuffer(fbId);
    if (framebuffer == nullptr || framebuffer->colorTextureId == UINT32_MAX ||
        framebuffer->colorTextureId >= mTextures.size()) {
        return;
    }
    SelectTexture(0, framebuffer->colorTextureId);
}

void GfxRenderingAPIVulkan::DeleteTexture(uint32_t texId) {
    (void)texId;
}

void GfxRenderingAPIVulkan::SetTextureFilter(FilteringMode mode) {
    mCurrentFilterMode = mode;
}

FilteringMode GfxRenderingAPIVulkan::GetTextureFilter() {
    return mCurrentFilterMode;
}

void GfxRenderingAPIVulkan::SetSrgbMode() {
    mSrgbMode = true;
}

ImTextureID GfxRenderingAPIVulkan::GetTextureById(int id) {
    if (id < 0) {
        throw std::runtime_error("Invalid negative Vulkan texture id");
    }

    EnsureImGuiTextureDescriptor(static_cast<uint32_t>(id));
    return reinterpret_cast<ImTextureID>(GetTexture(static_cast<uint32_t>(id)).imguiDescriptorSet);
}

bool GfxRenderingAPIVulkan::InitImGui() {
    if (mImGuiInitialized) {
        return true;
    }
    if (mInstance == VK_NULL_HANDLE || mPhysicalDevice == VK_NULL_HANDLE || mDevice == VK_NULL_HANDLE ||
        mGraphicsQueue == VK_NULL_HANDLE || mSwapchainImages.empty()) {
        return false;
    }

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.ApiVersion = Vulkan::MinimumApiVersion;
    initInfo.Instance = mInstance;
    initInfo.PhysicalDevice = mPhysicalDevice;
    initInfo.Device = mDevice;
    initInfo.QueueFamily = *mQueueFamilies.graphicsFamily;
    initInfo.Queue = mGraphicsQueue;
    initInfo.DescriptorPoolSize = ImGuiDescriptorPoolSize;
    initInfo.MinImageCount = GetMinImageCount();
    initInfo.ImageCount = static_cast<uint32_t>(mSwapchainImages.size());
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.UseDynamicRendering = true;
    initInfo.CheckVkResultFn = CheckImGuiVkResult;
#ifdef IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
    initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &mSwapchainImageFormat;
    initInfo.PipelineRenderingCreateInfo.depthAttachmentFormat = mDepthFormat;
#endif

    mImGuiInitialized = ImGui_ImplVulkan_Init(&initInfo);
    if (mImGuiInitialized) {
        ImGui_ImplVulkan_CreateFontsTexture();
    }
    return mImGuiInitialized;
}

void GfxRenderingAPIVulkan::ShutdownImGui() {
    if (!mImGuiInitialized) {
        return;
    }

    if (mDevice != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(mDevice);
    }
    ImGui_ImplVulkan_Shutdown();
    mImGuiInitialized = false;
    for (auto& texture : mTextures) {
        texture.imguiDescriptorSet = VK_NULL_HANDLE;
    }
}

void GfxRenderingAPIVulkan::NewFrame() {
    if (!mImGuiInitialized) {
        InitImGui();
    }
    if (mImGuiInitialized) {
        ImGui_ImplVulkan_NewFrame();
    }
}

void GfxRenderingAPIVulkan::RenderDrawData(ImDrawData* drawData) {
    if (!mImGuiInitialized || !mFrameActive || mCurrentCommandBuffer == VK_NULL_HANDLE) {
        return;
    }

    ImGui_ImplVulkan_RenderDrawData(drawData, mCurrentCommandBuffer);
}

uint32_t GfxRenderingAPIVulkan::GetMinImageCount() const {
    return std::max(2u, static_cast<uint32_t>(mSwapchainImages.size()));
}

void GfxRenderingAPIVulkan::CreateSwapchain() {
    auto swapchainSupport = Vulkan::QuerySwapchainSupport(mPhysicalDevice, mSurface);
    auto surfaceFormat = Vulkan::ChooseSwapSurfaceFormat(swapchainSupport.formats);
    auto presentMode = Vulkan::ChooseSwapPresentMode(swapchainSupport.presentModes);
    auto extent = Vulkan::ChooseSwapExtent(swapchainSupport.capabilities, mWindowBackend->GetWindow());

    constexpr VkImageUsageFlags requiredImageUsage =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ((swapchainSupport.capabilities.supportedUsageFlags & requiredImageUsage) != requiredImageUsage) {
        // TODO: Add alternate paths for platforms whose swapchain images cannot be transfer targets or attachments.
        throw std::runtime_error("Vulkan swapchain images do not support required usage flags");
    }

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = mSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = requiredImageUsage;

    std::array<uint32_t, 2> queueFamilyIndices = { *mQueueFamilies.graphicsFamily, *mQueueFamilies.presentFamily };
    if (mQueueFamilies.graphicsFamily != mQueueFamilies.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan swapchain");
    }

    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, nullptr);
    mSwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, mSwapchainImages.data());

    mSwapchainImageFormat = surfaceFormat.format;
    mSwapchainExtent = extent;
    mSwapchainImageLayouts.assign(mSwapchainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);
    mSwapchainImageStageMasks.assign(mSwapchainImages.size(), VK_PIPELINE_STAGE_2_NONE);
    mSwapchainImageAccessMasks.assign(mSwapchainImages.size(), VK_ACCESS_2_NONE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    mSwapchainRenderFinishedSemaphores.resize(mSwapchainImages.size(), VK_NULL_HANDLE);
    for (VkSemaphore& semaphore : mSwapchainRenderFinishedSemaphores) {
        CheckVk(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &semaphore),
                "Failed to create Vulkan swapchain render-finished semaphore");
    }
}

void GfxRenderingAPIVulkan::CreateImageViews() {
    mSwapchainImageViews.resize(mSwapchainImages.size());

    for (size_t i = 0; i < mSwapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = mSwapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = mSwapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(mDevice, &createInfo, nullptr, &mSwapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan swapchain image view");
        }
    }
}

void GfxRenderingAPIVulkan::CreateDepthResources() {
    if (mAllocator == nullptr || mSwapchainExtent.width == 0 || mSwapchainExtent.height == 0) {
        return;
    }

    DestroyDepthResources();

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { mSwapchainExtent.width, mSwapchainExtent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = mDepthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    CheckVk(vmaCreateImage(mAllocator, &imageInfo, &allocInfo, &mDepthImage, &mDepthAllocation, nullptr),
            "Failed to create Vulkan depth image");

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = mDepthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = mDepthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    CheckVk(vkCreateImageView(mDevice, &viewInfo, nullptr, &mDepthImageView),
            "Failed to create Vulkan depth image view");

    mDepthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    mDepthImageStageMask = VK_PIPELINE_STAGE_2_NONE;
    mDepthImageAccessMask = VK_ACCESS_2_NONE;
    mDepthExtent = mSwapchainExtent;
}

void GfxRenderingAPIVulkan::DestroyDepthResources() {
    if (mDevice != VK_NULL_HANDLE && mDepthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(mDevice, mDepthImageView, nullptr);
        mDepthImageView = VK_NULL_HANDLE;
    }
    if (mAllocator != nullptr && mDepthImage != VK_NULL_HANDLE) {
        vmaDestroyImage(mAllocator, mDepthImage, mDepthAllocation);
        mDepthImage = VK_NULL_HANDLE;
        mDepthAllocation = nullptr;
    }
    mDepthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    mDepthImageStageMask = VK_PIPELINE_STAGE_2_NONE;
    mDepthImageAccessMask = VK_ACCESS_2_NONE;
    mDepthExtent = {};
}

void GfxRenderingAPIVulkan::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = *mQueueFamilies.graphicsFamily;

    if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan command pool");
    }
}

void GfxRenderingAPIVulkan::CreateUploadCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = *mQueueFamilies.graphicsFamily;

    CheckVk(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mUploadCommandPool),
            "Failed to create Vulkan upload command pool");
}

VkPipelineLayout GfxRenderingAPIVulkan::CreatePipelineLayout() {
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(VulkanPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &mTextureDescriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    CheckVk(vkCreatePipelineLayout(mDevice, &layoutInfo, nullptr, &pipelineLayout),
            "Failed to create Vulkan graphics pipeline layout");
    return pipelineLayout;
}

VkPipeline GfxRenderingAPIVulkan::CreateGraphicsPipeline(VulkanShaderProgram& program, const CCFeatures& ccFeatures,
                                                         bool useAlpha) {
    (void)ccFeatures;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = program.vertexShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = program.fragmentShaderModule;
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization = {};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.depthBiasEnable = VK_TRUE;
    rasterization.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = useAlpha ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(std::size(dynamicStates));
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineRenderingCreateInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &mSwapchainImageFormat;
    renderingInfo.depthAttachmentFormat = mDepthFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = static_cast<uint32_t>(std::size(shaderStages));
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterization;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = program.pipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    CheckVk(vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
            "Failed to create Vulkan graphics pipeline");
    return pipeline;
}

void GfxRenderingAPIVulkan::CreateUberShaderPipeline() {
    if (mUberOpaquePipeline != VK_NULL_HANDLE && mUberAlphaPipeline != VK_NULL_HANDLE) {
        return;
    }

    LUS_TRACY_ZONE("Vulkan Create Uber Shader Pipeline");

    auto vertexSpirv =
        CompileVulkanGlslToSpirv(VulkanUberVertexShaderSource, shaderc_vertex_shader, "vulkan_uber.vert");
    auto fragmentSpirv =
        CompileVulkanGlslToSpirv(VulkanUberFragmentShaderSource, shaderc_fragment_shader, "vulkan_uber.frag");

    mUberVertexShaderModule =
        CreateShaderModule(mDevice, vertexSpirv, "Failed to create Vulkan uber vertex shader module");
    mUberFragmentShaderModule =
        CreateShaderModule(mDevice, fragmentSpirv, "Failed to create Vulkan uber fragment shader module");
    mUberPipelineLayout = CreatePipelineLayout();

    VulkanShaderProgram program = {};
    program.vertexShaderModule = mUberVertexShaderModule;
    program.fragmentShaderModule = mUberFragmentShaderModule;
    program.pipelineLayout = mUberPipelineLayout;

    CCFeatures dummyFeatures = {};
    mUberOpaquePipeline = CreateGraphicsPipeline(program, dummyFeatures, false);
    mUberAlphaPipeline = CreateGraphicsPipeline(program, dummyFeatures, true);
}

void GfxRenderingAPIVulkan::DestroyUberShaderPipeline() {
    if (mUberOpaquePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(mDevice, mUberOpaquePipeline, nullptr);
        mUberOpaquePipeline = VK_NULL_HANDLE;
    }
    if (mUberAlphaPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(mDevice, mUberAlphaPipeline, nullptr);
        mUberAlphaPipeline = VK_NULL_HANDLE;
    }
    if (mUberPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(mDevice, mUberPipelineLayout, nullptr);
        mUberPipelineLayout = VK_NULL_HANDLE;
    }
    if (mUberVertexShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(mDevice, mUberVertexShaderModule, nullptr);
        mUberVertexShaderModule = VK_NULL_HANDLE;
    }
    if (mUberFragmentShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(mDevice, mUberFragmentShaderModule, nullptr);
        mUberFragmentShaderModule = VK_NULL_HANDLE;
    }
}

void GfxRenderingAPIVulkan::CreateCommandBuffers() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(mFrames.size());

    std::array<VkCommandBuffer, FRAMES_IN_FLIGHT> commandBuffers = {};
    if (vkAllocateCommandBuffers(mDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate Vulkan command buffers");
    }

    for (size_t i = 0; i < mFrames.size(); i++) {
        mFrames[i].commandBuffer = commandBuffers[i];
    }
}

void GfxRenderingAPIVulkan::CreateSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphoreTypeCreateInfo timelineSemaphoreTypeInfo = {};
    timelineSemaphoreTypeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timelineSemaphoreTypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineSemaphoreTypeInfo.initialValue = 0;

    VkSemaphoreCreateInfo timelineSemaphoreInfo = {};
    timelineSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    timelineSemaphoreInfo.pNext = &timelineSemaphoreTypeInfo;

    for (auto& frame : mFrames) {
        if (frame.imageAvailableSemaphore == VK_NULL_HANDLE &&
            vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan image-available semaphore");
        }
        if (frame.renderFinishedTimelineSemaphore == VK_NULL_HANDLE &&
            vkCreateSemaphore(mDevice, &timelineSemaphoreInfo, nullptr, &frame.renderFinishedTimelineSemaphore) !=
                VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan render-finished timeline semaphore");
        }
    }
}

void GfxRenderingAPIVulkan::DestroyFrameResources() {
    if (mDevice == VK_NULL_HANDLE) {
        return;
    }

    for (auto& frame : mFrames) {
        if (frame.commandBuffer != VK_NULL_HANDLE && mCommandPool != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(mDevice, mCommandPool, 1, &frame.commandBuffer);
            frame.commandBuffer = VK_NULL_HANDLE;
        }
        if (frame.imageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(mDevice, frame.imageAvailableSemaphore, nullptr);
            frame.imageAvailableSemaphore = VK_NULL_HANDLE;
        }
        if (frame.renderFinishedTimelineSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(mDevice, frame.renderFinishedTimelineSemaphore, nullptr);
            frame.renderFinishedTimelineSemaphore = VK_NULL_HANDLE;
        }
        frame.renderFinishedTimelineValue = 0;
    }

    mCurrentFrame = nullptr;
    mCurrentCommandBuffer = VK_NULL_HANDLE;
    mCurrentFrameIndex = 0;
}

void GfxRenderingAPIVulkan::CreateAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.physicalDevice = mPhysicalDevice;
    allocatorInfo.device = mDevice;
    allocatorInfo.instance = mInstance;
    allocatorInfo.vulkanApiVersion = Vulkan::MinimumApiVersion;

    if (vmaCreateAllocator(&allocatorInfo, &mAllocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan memory allocator");
    }
}

void GfxRenderingAPIVulkan::CreateTextureDescriptorResources() {
    VkPhysicalDeviceDescriptorIndexingProperties indexingProperties = {};
    indexingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;

    VkPhysicalDeviceProperties2 properties = {};
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext = &indexingProperties;
    vkGetPhysicalDeviceProperties2(mPhysicalDevice, &properties);

    uint32_t deviceLimit =
        std::min({ indexingProperties.maxDescriptorSetUpdateAfterBindSampledImages,
                   indexingProperties.maxDescriptorSetUpdateAfterBindSamplers,
                   indexingProperties.maxPerStageDescriptorUpdateAfterBindSampledImages,
                   indexingProperties.maxPerStageDescriptorUpdateAfterBindSamplers });
    if (deviceLimit == 0) {
        throw std::runtime_error("Vulkan device does not expose update-after-bind sampled image descriptors");
    }
    mMaxBindlessTextures = std::min(PreferredBindlessTextureCount, deviceLimit);

    VkDescriptorSetLayoutBinding textureBinding = {};
    textureBinding.binding = 0;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = mMaxBindlessTextures;
    textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorBindingFlags bindingFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
                                            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                                            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {};
    bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlagsInfo.bindingCount = 1;
    bindingFlagsInfo.pBindingFlags = &bindingFlags;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = &bindingFlagsInfo;
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &textureBinding;
    CheckVk(vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mTextureDescriptorSetLayout),
            "Failed to create Vulkan texture descriptor set layout");

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = mMaxBindlessTextures;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    CheckVk(vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mTextureDescriptorPool),
            "Failed to create Vulkan texture descriptor pool");

    VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo = {};
    variableCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    variableCountInfo.descriptorSetCount = 1;
    variableCountInfo.pDescriptorCounts = &mMaxBindlessTextures;

    VkDescriptorSetAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.pNext = &variableCountInfo;
    allocateInfo.descriptorPool = mTextureDescriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &mTextureDescriptorSetLayout;
    CheckVk(vkAllocateDescriptorSets(mDevice, &allocateInfo, &mTextureDescriptorSet),
            "Failed to allocate Vulkan texture descriptor set");
}

void GfxRenderingAPIVulkan::DestroyTextureDescriptorResources() {
    if (mTextureDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(mDevice, mTextureDescriptorPool, nullptr);
        mTextureDescriptorPool = VK_NULL_HANDLE;
    }
    if (mTextureDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(mDevice, mTextureDescriptorSetLayout, nullptr);
        mTextureDescriptorSetLayout = VK_NULL_HANDLE;
    }
    mTextureDescriptorSet = VK_NULL_HANDLE;
    mMaxBindlessTextures = 0;
}

void GfxRenderingAPIVulkan::DestroyShaderProgram(VulkanShaderProgram& program) {
    if (program.pipeline != VK_NULL_HANDLE && program.pipeline != mUberOpaquePipeline &&
        program.pipeline != mUberAlphaPipeline) {
        vkDestroyPipeline(mDevice, program.pipeline, nullptr);
    }
    program.pipeline = VK_NULL_HANDLE;
    if (program.pipelineLayout != VK_NULL_HANDLE && program.pipelineLayout != mUberPipelineLayout) {
        vkDestroyPipelineLayout(mDevice, program.pipelineLayout, nullptr);
    }
    program.pipelineLayout = VK_NULL_HANDLE;
    if (program.vertexShaderModule != VK_NULL_HANDLE && program.vertexShaderModule != mUberVertexShaderModule) {
        vkDestroyShaderModule(mDevice, program.vertexShaderModule, nullptr);
    }
    program.vertexShaderModule = VK_NULL_HANDLE;
    if (program.fragmentShaderModule != VK_NULL_HANDLE && program.fragmentShaderModule != mUberFragmentShaderModule) {
        vkDestroyShaderModule(mDevice, program.fragmentShaderModule, nullptr);
    }
    program.fragmentShaderModule = VK_NULL_HANDLE;
}

void GfxRenderingAPIVulkan::DestroyShaderPrograms() {
    for (auto& [_, program] : mShaderProgramPool) {
        DestroyShaderProgram(*program);
    }
    mShaderProgramPool.clear();
    mShaderProgram = nullptr;
}

VkCommandBuffer GfxRenderingAPIVulkan::BeginImmediateCommands() {
    LUS_TRACY_ZONE("Vulkan BeginImmediateCommands");
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = mUploadCommandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    CheckVk(vkAllocateCommandBuffers(mDevice, &allocateInfo, &commandBuffer),
            "Failed to allocate Vulkan immediate command buffer");

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CheckVk(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin Vulkan immediate command buffer");
    return commandBuffer;
}

void GfxRenderingAPIVulkan::EndImmediateCommands(VkCommandBuffer commandBuffer) {
    LUS_TRACY_ZONE("Vulkan EndImmediateCommands");
    mTracyImmediateSubmitsThisFrame++;

    CheckVk(vkEndCommandBuffer(commandBuffer), "Failed to end Vulkan immediate command buffer");

    VkCommandBufferSubmitInfo commandBufferInfo = {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferInfo.commandBuffer = commandBuffer;

    VkSubmitInfo2 submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandBufferInfo;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = VK_NULL_HANDLE;
    CheckVk(vkCreateFence(mDevice, &fenceInfo, nullptr, &fence), "Failed to create Vulkan immediate fence");

    {
        LUS_TRACY_ZONE("Vulkan QueueSubmit Immediate");
        CheckVk(vkQueueSubmit2(mGraphicsQueue, 1, &submitInfo, fence),
                "Failed to submit Vulkan immediate command buffer");
    }
    {
        LUS_TRACY_ZONE("Vulkan Wait Immediate Fence");
        CheckVk(vkWaitForFences(mDevice, 1, &fence, VK_TRUE, UINT64_MAX), "Failed to wait for Vulkan immediate fence");
    }
    vkDestroyFence(mDevice, fence, nullptr);
    vkFreeCommandBuffers(mDevice, mUploadCommandPool, 1, &commandBuffer);
}

VulkanTexture& GfxRenderingAPIVulkan::GetTexture(uint32_t textureId) {
    if (textureId >= mTextures.size()) {
        throw std::runtime_error("Vulkan texture id does not exist");
    }
    return mTextures[textureId];
}

void GfxRenderingAPIVulkan::TransitionImageUsage(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout& layout,
                                                 VkPipelineStageFlags2& stageMask, VkAccessFlags2& accessMask,
                                                 VulkanImageUsage newUsage) {
    if (image == VK_NULL_HANDLE) {
        return;
    }

    VulkanImageUsageState newState = GetImageUsageState(newUsage);
    VkPipelineStageFlags2 oldStageMask = layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_PIPELINE_STAGE_2_NONE : stageMask;
    VkAccessFlags2 oldAccessMask = layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_ACCESS_2_NONE : accessMask;
    if (layout == newState.layout && oldStageMask == newState.stageMask && oldAccessMask == newState.accessMask) {
        return;
    }

    VkImageMemoryBarrier2 barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.srcStageMask = oldStageMask;
    barrier.srcAccessMask = oldAccessMask;
    barrier.dstStageMask = newState.stageMask;
    barrier.dstAccessMask = newState.accessMask;
    barrier.oldLayout = layout;
    barrier.newLayout = newState.layout;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dependency = {};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(mCurrentCommandBuffer, &dependency);

    layout = newState.layout;
    stageMask = newState.stageMask;
    accessMask = newState.accessMask;
}

VulkanFramebuffer* GfxRenderingAPIVulkan::GetFramebuffer(int fbId) {
    if (fbId < 0 || static_cast<size_t>(fbId) >= mFramebuffers.size()) {
        return nullptr;
    }
    return &mFramebuffers[fbId];
}

VkImage GfxRenderingAPIVulkan::GetFramebufferColorImage(int fbId) {
    if (fbId == 0) {
        return mCurrentImageIndex < mSwapchainImages.size() ? mSwapchainImages[mCurrentImageIndex] : VK_NULL_HANDLE;
    }
    VulkanFramebuffer* framebuffer = GetFramebuffer(fbId);
    return framebuffer != nullptr ? framebuffer->colorImage : VK_NULL_HANDLE;
}

VkImageView GfxRenderingAPIVulkan::GetFramebufferColorImageView(int fbId) {
    if (fbId == 0) {
        return mCurrentImageIndex < mSwapchainImageViews.size() ? mSwapchainImageViews[mCurrentImageIndex]
                                                               : VK_NULL_HANDLE;
    }
    VulkanFramebuffer* framebuffer = GetFramebuffer(fbId);
    return framebuffer != nullptr ? framebuffer->colorImageView : VK_NULL_HANDLE;
}

VkImageLayout& GfxRenderingAPIVulkan::GetFramebufferColorLayout(int fbId) {
    if (fbId == 0) {
        return mSwapchainImageLayouts[mCurrentImageIndex];
    }
    VulkanFramebuffer* framebuffer = GetFramebuffer(fbId);
    if (framebuffer == nullptr) {
        throw std::runtime_error("Invalid Vulkan framebuffer id");
    }
    return framebuffer->colorLayout;
}

VkPipelineStageFlags2& GfxRenderingAPIVulkan::GetFramebufferColorStageMask(int fbId) {
    if (fbId == 0) {
        return mSwapchainImageStageMasks[mCurrentImageIndex];
    }
    VulkanFramebuffer* framebuffer = GetFramebuffer(fbId);
    if (framebuffer == nullptr) {
        throw std::runtime_error("Invalid Vulkan framebuffer id");
    }
    return framebuffer->colorStageMask;
}

VkAccessFlags2& GfxRenderingAPIVulkan::GetFramebufferColorAccessMask(int fbId) {
    if (fbId == 0) {
        return mSwapchainImageAccessMasks[mCurrentImageIndex];
    }
    VulkanFramebuffer* framebuffer = GetFramebuffer(fbId);
    if (framebuffer == nullptr) {
        throw std::runtime_error("Invalid Vulkan framebuffer id");
    }
    return framebuffer->colorAccessMask;
}

VkExtent2D GfxRenderingAPIVulkan::GetFramebufferExtent(int fbId) const {
    if (fbId <= 0 || static_cast<size_t>(fbId) >= mFramebuffers.size() || mFramebuffers[fbId].width == 0 ||
        mFramebuffers[fbId].height == 0) {
        return mSwapchainExtent;
    }
    return { mFramebuffers[fbId].width, mFramebuffers[fbId].height };
}

void GfxRenderingAPIVulkan::BeginRenderingToCurrentFramebuffer() {
    if (!mFrameActive || mRenderingActive || mCurrentCommandBuffer == VK_NULL_HANDLE) {
        return;
    }

    VkImage colorImage = GetFramebufferColorImage(mCurrentFramebuffer);
    VkImageView colorImageView = GetFramebufferColorImageView(mCurrentFramebuffer);
    if (colorImage == VK_NULL_HANDLE || colorImageView == VK_NULL_HANDLE) {
        return;
    }

    VkImageLayout& colorLayout = GetFramebufferColorLayout(mCurrentFramebuffer);
    VkPipelineStageFlags2& colorStageMask = GetFramebufferColorStageMask(mCurrentFramebuffer);
    VkAccessFlags2& colorAccessMask = GetFramebufferColorAccessMask(mCurrentFramebuffer);
    TransitionImageUsage(colorImage, VK_IMAGE_ASPECT_COLOR_BIT, colorLayout, colorStageMask, colorAccessMask,
                         VulkanImageUsage::ColorAttachment);

    VulkanFramebuffer* framebuffer = GetFramebuffer(mCurrentFramebuffer);
    VkImageView depthImageView = VK_NULL_HANDLE;
    VkImageLayout* depthLayout = nullptr;
    VkPipelineStageFlags2* depthStageMask = nullptr;
    VkAccessFlags2* depthAccessMask = nullptr;
    VkImage depthImage = VK_NULL_HANDLE;
    if (mCurrentFramebuffer == 0) {
        depthImage = mDepthImage;
        depthImageView = mDepthImageView;
        depthLayout = &mDepthImageLayout;
        depthStageMask = &mDepthImageStageMask;
        depthAccessMask = &mDepthImageAccessMask;
    } else if (framebuffer != nullptr && framebuffer->hasDepthBuffer) {
        depthImage = framebuffer->depthImage;
        depthImageView = framebuffer->depthImageView;
        depthLayout = &framebuffer->depthLayout;
        depthStageMask = &framebuffer->depthStageMask;
        depthAccessMask = &framebuffer->depthAccessMask;
    }
    if (depthImage != VK_NULL_HANDLE && depthLayout != nullptr && depthStageMask != nullptr && depthAccessMask != nullptr) {
        TransitionImageUsage(depthImage, VK_IMAGE_ASPECT_DEPTH_BIT, *depthLayout, *depthStageMask, *depthAccessMask,
                             VulkanImageUsage::DepthAttachment);
    }

    VkRenderingAttachmentInfo colorAttachment = {};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = colorImageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depthAttachment = {};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthImageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = { 0, 0 };
    renderingInfo.renderArea.extent = { mCurrentRenderTargetWidth, mCurrentRenderTargetHeight };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = depthImageView != VK_NULL_HANDLE ? &depthAttachment : nullptr;
    vkCmdBeginRendering(mCurrentCommandBuffer, &renderingInfo);
    mRenderingActive = true;

    SetViewport(0, 0, static_cast<int>(mCurrentRenderTargetWidth), static_cast<int>(mCurrentRenderTargetHeight));
    SetScissor(0, 0, static_cast<int>(mCurrentRenderTargetWidth), static_cast<int>(mCurrentRenderTargetHeight));
}

void GfxRenderingAPIVulkan::EndCurrentRendering() {
    if (!mRenderingActive || mCurrentCommandBuffer == VK_NULL_HANDLE) {
        return;
    }
    vkCmdEndRendering(mCurrentCommandBuffer);
    mRenderingActive = false;

    VulkanFramebuffer* framebuffer = GetFramebuffer(mCurrentFramebuffer);
    if (framebuffer != nullptr && mCurrentFramebuffer != 0 && framebuffer->colorTextureId < mTextures.size()) {
        TransitionImageUsage(framebuffer->colorImage, VK_IMAGE_ASPECT_COLOR_BIT, framebuffer->colorLayout,
                             framebuffer->colorStageMask, framebuffer->colorAccessMask, VulkanImageUsage::ShaderRead);
        VulkanTexture& texture = mTextures[framebuffer->colorTextureId];
        texture.layout = framebuffer->colorLayout;
        if (texture.uploaded) {
            WriteBindlessTextureDescriptor(framebuffer->colorTextureId);
        }
    }
}

void GfxRenderingAPIVulkan::CreateFramebufferColorResources(VulkanFramebuffer& framebuffer) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { framebuffer.width, framebuffer.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = mSwapchainImageFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    CheckVk(vmaCreateImage(mAllocator, &imageInfo, &allocInfo, &framebuffer.colorImage,
                           &framebuffer.colorAllocation, nullptr),
            "Failed to create Vulkan framebuffer color image");

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = framebuffer.colorImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = mSwapchainImageFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    CheckVk(vkCreateImageView(mDevice, &viewInfo, nullptr, &framebuffer.colorImageView),
            "Failed to create Vulkan framebuffer color image view");
    framebuffer.colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    framebuffer.colorStageMask = VK_PIPELINE_STAGE_2_NONE;
    framebuffer.colorAccessMask = VK_ACCESS_2_NONE;

    VulkanTexture& texture = GetTexture(framebuffer.colorTextureId);
    if (texture.imguiDescriptorSet != VK_NULL_HANDLE && mImGuiInitialized) {
        ImGui_ImplVulkan_RemoveTexture(texture.imguiDescriptorSet);
    }
    if (texture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(mDevice, texture.sampler, nullptr);
    }
    texture.image = framebuffer.colorImage;
    texture.allocation = framebuffer.colorAllocation;
    texture.imageView = framebuffer.colorImageView;
    texture.imguiDescriptorSet = VK_NULL_HANDLE;
    texture.width = framebuffer.width;
    texture.height = framebuffer.height;
    texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texture.uploaded = true;
    texture.linearFiltering = true;
    texture.filtering = FILTER_LINEAR;
    texture.cms = G_TX_NOMIRROR | G_TX_CLAMP;
    texture.cmt = G_TX_NOMIRROR | G_TX_CLAMP;

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxAnisotropy = 1.0f;
    CheckVk(vkCreateSampler(mDevice, &samplerInfo, nullptr, &texture.sampler),
            "Failed to create Vulkan framebuffer sampler");
    WriteBindlessTextureDescriptor(framebuffer.colorTextureId);
}

void GfxRenderingAPIVulkan::CreateFramebufferDepthResources(VulkanFramebuffer& framebuffer) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { framebuffer.width, framebuffer.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = mDepthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                      (framebuffer.canExtractDepth ? VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0);
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    CheckVk(vmaCreateImage(mAllocator, &imageInfo, &allocInfo, &framebuffer.depthImage,
                           &framebuffer.depthAllocation, nullptr),
            "Failed to create Vulkan framebuffer depth image");

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = framebuffer.depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = mDepthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    CheckVk(vkCreateImageView(mDevice, &viewInfo, nullptr, &framebuffer.depthImageView),
            "Failed to create Vulkan framebuffer depth image view");
    framebuffer.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    framebuffer.depthStageMask = VK_PIPELINE_STAGE_2_NONE;
    framebuffer.depthAccessMask = VK_ACCESS_2_NONE;
}

void GfxRenderingAPIVulkan::UploadTextureToGpu(VulkanTexture& texture, const uint8_t* rgba32Buf, uint32_t width,
                                               uint32_t height) {
    LUS_TRACY_ZONE("Vulkan UploadTextureToGpu");
    if (mAllocator == nullptr) {
        throw std::runtime_error("Cannot upload Vulkan texture before VMA allocator creation");
    }

    VkDeviceSize uploadSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = uploadSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                             VMA_ALLOCATION_CREATE_MAPPED_BIT;

    // TODO: Use a persistent staging/ring buffer instead of allocating a small upload buffer per texture.
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAllocation = nullptr;
    VmaAllocationInfo stagingInfo = {};
    {
        LUS_TRACY_ZONE("Vulkan Create Texture Staging Buffer");
        CheckVk(vmaCreateBuffer(mAllocator, &bufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation,
                                &stagingInfo),
                "Failed to create Vulkan texture staging buffer");
    }
    {
        LUS_TRACY_ZONE("Vulkan Copy Texture To Staging");
        std::memcpy(stagingInfo.pMappedData, rgba32Buf, static_cast<size_t>(uploadSize));
    }

    VkImage oldImage = texture.image;
    VmaAllocation oldAllocation = texture.allocation;
    VkImageView oldImageView = texture.imageView;
    VkSampler oldSampler = texture.sampler;
    VkDescriptorSet oldImGuiDescriptorSet = texture.imguiDescriptorSet;

    texture.image = VK_NULL_HANDLE;
    texture.allocation = nullptr;
    texture.imageView = VK_NULL_HANDLE;
    texture.sampler = VK_NULL_HANDLE;
    texture.imguiDescriptorSet = VK_NULL_HANDLE;
    texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    texture.uploaded = false;

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { width, height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo imageAllocInfo = {};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    {
        LUS_TRACY_ZONE("Vulkan Create Texture Image");
        CheckVk(vmaCreateImage(mAllocator, &imageInfo, &imageAllocInfo, &texture.image, &texture.allocation, nullptr),
                "Failed to create Vulkan texture image");
    }

    VkCommandBuffer commandBuffer = BeginImmediateCommands();

    VkImageMemoryBarrier2 transferBarrier = {};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
    transferBarrier.srcAccessMask = VK_ACCESS_2_NONE;
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    transferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarrier.image = texture.image;
    transferBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    transferBarrier.subresourceRange.baseMipLevel = 0;
    transferBarrier.subresourceRange.levelCount = 1;
    transferBarrier.subresourceRange.baseArrayLayer = 0;
    transferBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo transferDependency = {};
    transferDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    transferDependency.imageMemoryBarrierCount = 1;
    transferDependency.pImageMemoryBarriers = &transferBarrier;
    vkCmdPipelineBarrier2(commandBuffer, &transferDependency);

    VkBufferImageCopy copyRegion = {};
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = { width, height, 1 };
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &copyRegion);

    VkImageMemoryBarrier2 shaderReadBarrier = {};
    shaderReadBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    shaderReadBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    shaderReadBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    shaderReadBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    shaderReadBarrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    shaderReadBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    shaderReadBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    shaderReadBarrier.image = texture.image;
    shaderReadBarrier.subresourceRange = transferBarrier.subresourceRange;

    VkDependencyInfo shaderReadDependency = {};
    shaderReadDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    shaderReadDependency.imageMemoryBarrierCount = 1;
    shaderReadDependency.pImageMemoryBarriers = &shaderReadBarrier;
    vkCmdPipelineBarrier2(commandBuffer, &shaderReadDependency);

    EndImmediateCommands(commandBuffer);
    {
        LUS_TRACY_ZONE("Vulkan Destroy Texture Staging Buffer");
        vmaDestroyBuffer(mAllocator, stagingBuffer, stagingAllocation);
    }

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    CheckVk(vkCreateImageView(mDevice, &viewInfo, nullptr, &texture.imageView),
            "Failed to create Vulkan texture image view");

    VkFilter filter =
        texture.linearFiltering && mCurrentFilterMode == FILTER_LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.mipmapMode =
        filter == VK_FILTER_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = GfxCmToVulkan(texture.cms);
    samplerInfo.addressModeV = GfxCmToVulkan(texture.cmt);
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    CheckVk(vkCreateSampler(mDevice, &samplerInfo, nullptr, &texture.sampler),
            "Failed to create Vulkan texture sampler");

    texture.width = width;
    texture.height = height;
    texture.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texture.uploaded = true;

    if (oldImGuiDescriptorSet != VK_NULL_HANDLE && mImGuiInitialized) {
        ImGui_ImplVulkan_RemoveTexture(oldImGuiDescriptorSet);
    }
    if (oldSampler != VK_NULL_HANDLE) {
        vkDestroySampler(mDevice, oldSampler, nullptr);
    }
    if (oldImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(mDevice, oldImageView, nullptr);
    }
    if (oldImage != VK_NULL_HANDLE) {
        vmaDestroyImage(mAllocator, oldImage, oldAllocation);
    }
}

void GfxRenderingAPIVulkan::WriteBindlessTextureDescriptor(uint32_t textureId) {
    VulkanTexture& texture = GetTexture(textureId);
    if (!texture.uploaded || texture.imageView == VK_NULL_HANDLE || texture.sampler == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot write descriptor for incomplete Vulkan texture");
    }

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.sampler = texture.sampler;
    imageInfo.imageView = texture.imageView;
    imageInfo.imageLayout = texture.layout;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = mTextureDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = textureId;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(mDevice, 1, &descriptorWrite, 0, nullptr);
}

void GfxRenderingAPIVulkan::EnsureImGuiTextureDescriptor(uint32_t textureId) {
    VulkanTexture& texture = GetTexture(textureId);
    if (!texture.uploaded || texture.imageView == VK_NULL_HANDLE || texture.sampler == VK_NULL_HANDLE) {
        throw std::runtime_error("Vulkan texture has not been uploaded");
    }
    if (texture.imguiDescriptorSet != VK_NULL_HANDLE) {
        return;
    }
    if (!mImGuiInitialized && !InitImGui()) {
        throw std::runtime_error("Cannot create ImGui descriptor for Vulkan texture before ImGui initialization");
    }
    texture.imguiDescriptorSet =
        ImGui_ImplVulkan_AddTexture(texture.sampler, texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GfxRenderingAPIVulkan::DestroyTexture(VulkanTexture& texture) {
    if (texture.imguiDescriptorSet != VK_NULL_HANDLE && mImGuiInitialized) {
        ImGui_ImplVulkan_RemoveTexture(texture.imguiDescriptorSet);
    }
    texture.imguiDescriptorSet = VK_NULL_HANDLE;
    if (texture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(mDevice, texture.sampler, nullptr);
        texture.sampler = VK_NULL_HANDLE;
    }
    if (texture.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(mDevice, texture.imageView, nullptr);
        texture.imageView = VK_NULL_HANDLE;
    }
    if (texture.image != VK_NULL_HANDLE) {
        vmaDestroyImage(mAllocator, texture.image, texture.allocation);
        texture.image = VK_NULL_HANDLE;
        texture.allocation = nullptr;
    }
    texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    texture.uploaded = false;
}

void GfxRenderingAPIVulkan::DestroyTextures() {
    for (auto& texture : mTextures) {
        DestroyTexture(texture);
    }
    mTextures.clear();
}

void GfxRenderingAPIVulkan::DestroyFramebufferResources(VulkanFramebuffer& framebuffer) {
    if (framebuffer.colorTextureId < mTextures.size()) {
        VulkanTexture& texture = mTextures[framebuffer.colorTextureId];
        if (texture.imguiDescriptorSet != VK_NULL_HANDLE && mImGuiInitialized) {
            ImGui_ImplVulkan_RemoveTexture(texture.imguiDescriptorSet);
        }
        texture.imguiDescriptorSet = VK_NULL_HANDLE;
        if (texture.sampler != VK_NULL_HANDLE) {
            vkDestroySampler(mDevice, texture.sampler, nullptr);
        }
        texture.sampler = VK_NULL_HANDLE;
        texture.image = VK_NULL_HANDLE;
        texture.allocation = nullptr;
        texture.imageView = VK_NULL_HANDLE;
        texture.layout = VK_IMAGE_LAYOUT_UNDEFINED;
        texture.uploaded = false;
    }

    if (framebuffer.colorImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(mDevice, framebuffer.colorImageView, nullptr);
        framebuffer.colorImageView = VK_NULL_HANDLE;
    }
    if (framebuffer.colorImage != VK_NULL_HANDLE) {
        vmaDestroyImage(mAllocator, framebuffer.colorImage, framebuffer.colorAllocation);
        framebuffer.colorImage = VK_NULL_HANDLE;
        framebuffer.colorAllocation = nullptr;
    }
    framebuffer.colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    framebuffer.colorStageMask = VK_PIPELINE_STAGE_2_NONE;
    framebuffer.colorAccessMask = VK_ACCESS_2_NONE;

    if (framebuffer.depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(mDevice, framebuffer.depthImageView, nullptr);
        framebuffer.depthImageView = VK_NULL_HANDLE;
    }
    if (framebuffer.depthImage != VK_NULL_HANDLE) {
        vmaDestroyImage(mAllocator, framebuffer.depthImage, framebuffer.depthAllocation);
        framebuffer.depthImage = VK_NULL_HANDLE;
        framebuffer.depthAllocation = nullptr;
    }
    framebuffer.depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    framebuffer.depthStageMask = VK_PIPELINE_STAGE_2_NONE;
    framebuffer.depthAccessMask = VK_ACCESS_2_NONE;
}

void GfxRenderingAPIVulkan::DestroyFramebuffers() {
    for (auto& framebuffer : mFramebuffers) {
        DestroyFramebufferResources(framebuffer);
    }
    mFramebuffers.clear();
}

void GfxRenderingAPIVulkan::CleanupSwapchain() {
    if (mDevice == VK_NULL_HANDLE) {
        return;
    }

    DestroyDepthResources();

    for (auto imageView : mSwapchainImageViews) {
        vkDestroyImageView(mDevice, imageView, nullptr);
    }
    mSwapchainImageViews.clear();

    for (VkSemaphore semaphore : mSwapchainRenderFinishedSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(mDevice, semaphore, nullptr);
        }
    }
    mSwapchainRenderFinishedSemaphores.clear();

    if (mSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
        mSwapchain = VK_NULL_HANDLE;
    }

    mSwapchainImages.clear();
    mSwapchainImageLayouts.clear();
    mSwapchainImageStageMasks.clear();
    mSwapchainImageAccessMasks.clear();
    mSwapchainImageFormat = VK_FORMAT_UNDEFINED;
    mSwapchainExtent = {};
    mCurrentCommandBuffer = VK_NULL_HANDLE;
    mCurrentFrame = nullptr;
    mFrameActive = false;
    mRenderingActive = false;
    mCurrentFramebuffer = 0;
    mCurrentRenderTargetWidth = 0;
    mCurrentRenderTargetHeight = 0;
}

void GfxRenderingAPIVulkan::RecreateSwapchain() {
    vkDeviceWaitIdle(mDevice);

    bool restoreImGui = mImGuiInitialized;
    ShutdownImGui();
    CleanupSwapchain();
    CreateSwapchain();
    CreateImageViews();
    CreateDepthResources();
    if (restoreImGui) {
        InitImGui();
    }
}

void GfxRenderingAPIVulkan::DestroyVulkanObjects() {
    // TODO: use destructor queue

    if (mDevice != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(mDevice);
        ShutdownImGui();
        CleanupSwapchain();
        DestroyShaderPrograms();
        DestroyUberShaderPipeline();
        DestroyFramebuffers();
        DestroyTextures();
        DestroyTextureDescriptorResources();
        DestroyFrameResources();
        mVertexRingBuffer.Destroy();
        if (mUploadCommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(mDevice, mUploadCommandPool, nullptr);
            mUploadCommandPool = VK_NULL_HANDLE;
        }
        if (mCommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
            mCommandPool = VK_NULL_HANDLE;
        }
        if (mAllocator != nullptr) {
            vmaDestroyAllocator(mAllocator);
            mAllocator = nullptr;
        }
        vkDestroyDevice(mDevice, nullptr);
        mDevice = VK_NULL_HANDLE;
    }

    if (mSurface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
        mSurface = VK_NULL_HANDLE;
    }

    if (mDebugMessenger != VK_NULL_HANDLE) {
        Vulkan::DestroyDebugMessenger(mInstance, mDebugMessenger);
        mDebugMessenger = VK_NULL_HANDLE;
    }

    if (mInstance != VK_NULL_HANDLE) {
        vkDestroyInstance(mInstance, nullptr);
        mInstance = VK_NULL_HANDLE;
    }

    mPhysicalDevice = VK_NULL_HANDLE;
    mGraphicsQueue = VK_NULL_HANDLE;
    mPresentQueue = VK_NULL_HANDLE;
}

} // namespace Fast
