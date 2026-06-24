//
//  gfx_vulkan.cpp
//  libultraship
//
//  Vulkan rendering backend for fast3d, architecturally mirroring the Metal
//  backend: per-framebuffer command buffers recorded during the frame and
//  submitted in framebuffer order at EndFrame, with the swapchain (fb 0) last.
//
//  Simplifications relative to a "maximally optimal" Vulkan renderer, chosen
//  for correctness and portability (MoltenVK included):
//   - every image lives in VK_IMAGE_LAYOUT_GENERAL (except the swapchain image,
//     which transitions to PRESENT_SRC at the end of the screen command buffer),
//   - render passes always LOAD/STORE; clears go through vkCmdClearAttachments,
//   - pipelines are created lazily per (shader, depth/cull/msaa state) combo,
//   - per-draw constants live in a per-frame host-visible ring bound once as a
//     descriptor set with dynamic offsets.
//
#ifdef ENABLE_VULKAN

#include "fast/backends/gfx_vulkan.h"

#include <vector>
#include <algorithm>
#include <cstring>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <shaderc/shaderc.hpp>
#include <spdlog/spdlog.h>
#include <prism/processor.h>

#include <imgui_impl_vulkan.h>

#include "libultraship/libultra/abi.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/resource/ResourceManager.h"
#include "ship/resource/factory/ShaderFactory.h"
#include <sstream>

#define VK_CHECK(expr)                                                          \
    do {                                                                        \
        VkResult _res = (expr);                                                 \
        if (_res != VK_SUCCESS) {                                               \
            SPDLOG_ERROR("Vulkan error {} at {}: " #expr, (int)_res, __LINE__); \
        }                                                                       \
    } while (0)

namespace Fast {

// MARK: - Shader generation (prism -> Vulkan GLSL -> SPIR-V via shaderc)

#define VK_RAND_NOISE \
    "((random(vec3(floor(gl_FragCoord.xy * frameU.noise_scale), float(frameU.frame_count))) + 1.0) / 2.0)"

static const char* vk_shader_item_to_str(uint32_t item, bool with_alpha, bool only_alpha, bool inputs_have_alpha,
                                         bool first_cycle, bool hint_single_element) {
    static thread_local char buf[64];
    if (!only_alpha) {
        switch (item) {
            case SHADER_0:
                return with_alpha ? "vec4(0.0, 0.0, 0.0, 0.0)" : "vec3(0.0, 0.0, 0.0)";
            case SHADER_1:
                return with_alpha ? "vec4(1.0, 1.0, 1.0, 1.0)" : "vec3(1.0, 1.0, 1.0)";
            case SHADER_INPUT_1:
            case SHADER_INPUT_2:
            case SHADER_INPUT_3:
            case SHADER_INPUT_4:
            case SHADER_INPUT_5:
            case SHADER_INPUT_6:
                snprintf(buf, sizeof(buf), with_alpha ? "drawU.inputs[%d]" : "drawU.inputs[%d].rgb",
                         (int)(item - SHADER_INPUT_1));
                return buf;
            case SHADER_INPUT_7:
                return with_alpha || !inputs_have_alpha ? "vShade" : "vShade.rgb";
            case SHADER_TEXEL0:
                return first_cycle ? (with_alpha ? "texVal0" : "texVal0.rgb")
                                   : (with_alpha ? "texVal1" : "texVal1.rgb");
            case SHADER_TEXEL0A:
                return first_cycle
                           ? (hint_single_element ? "texVal0.a"
                                                  : (with_alpha ? "vec4(texVal0.a, texVal0.a, texVal0.a, texVal0.a)"
                                                                : "vec3(texVal0.a, texVal0.a, texVal0.a)"))
                           : (hint_single_element ? "texVal1.a"
                                                  : (with_alpha ? "vec4(texVal1.a, texVal1.a, texVal1.a, texVal1.a)"
                                                                : "vec3(texVal1.a, texVal1.a, texVal1.a)"));
            case SHADER_TEXEL1A:
                return first_cycle
                           ? (hint_single_element ? "texVal1.a"
                                                  : (with_alpha ? "vec4(texVal1.a, texVal1.a, texVal1.a, texVal1.a)"
                                                                : "vec3(texVal1.a, texVal1.a, texVal1.a)"))
                           : (hint_single_element ? "texVal0.a"
                                                  : (with_alpha ? "vec4(texVal0.a, texVal0.a, texVal0.a, texVal0.a)"
                                                                : "vec3(texVal0.a, texVal0.a, texVal0.a)"));
            case SHADER_TEXEL1:
                return first_cycle ? (with_alpha ? "texVal1" : "texVal1.rgb")
                                   : (with_alpha ? "texVal0" : "texVal0.rgb");
            case SHADER_COMBINED:
                return with_alpha ? "texel" : "texel.rgb";
            case SHADER_NOISE:
                return with_alpha ? "vec4(" VK_RAND_NOISE ", " VK_RAND_NOISE ", " VK_RAND_NOISE ", " VK_RAND_NOISE ")"
                                  : "vec3(" VK_RAND_NOISE ", " VK_RAND_NOISE ", " VK_RAND_NOISE ")";
            case SHADER_LOD_FRAC:
                return hint_single_element ? "lodFrac"
                                           : (with_alpha ? "vec4(lodFrac, lodFrac, lodFrac, lodFrac)"
                                                         : "vec3(lodFrac, lodFrac, lodFrac)");
        }
    } else {
        switch (item) {
            case SHADER_0:
                return "0.0";
            case SHADER_1:
                return "1.0";
            case SHADER_INPUT_1:
            case SHADER_INPUT_2:
            case SHADER_INPUT_3:
            case SHADER_INPUT_4:
            case SHADER_INPUT_5:
            case SHADER_INPUT_6:
                snprintf(buf, sizeof(buf), "drawU.inputs[%d].a", (int)(item - SHADER_INPUT_1));
                return buf;
            case SHADER_INPUT_7:
                return "vShade.a";
            case SHADER_TEXEL0:
            case SHADER_TEXEL0A:
                return first_cycle ? "texVal0.a" : "texVal1.a";
            case SHADER_TEXEL1A:
            case SHADER_TEXEL1:
                return first_cycle ? "texVal1.a" : "texVal0.a";
            case SHADER_COMBINED:
                return "texel.a";
            case SHADER_NOISE:
                return VK_RAND_NOISE;
            case SHADER_LOD_FRAC:
                return "lodFrac";
        }
    }
    return "";
}

static bool vk_get_bool(prism::ContextTypes* value) {
    if (std::holds_alternative<int>(*value)) {
        return std::get<int>(*value) == 1;
    }
    return false;
}

static prism::ContextTypes* vk_append_formula(prism::ContextTypes* _, prism::ContextTypes* a_arg,
                                              prism::ContextTypes* a_single, prism::ContextTypes* a_mult,
                                              prism::ContextTypes* a_mix, prism::ContextTypes* a_with_alpha,
                                              prism::ContextTypes* a_only_alpha, prism::ContextTypes* a_alpha,
                                              prism::ContextTypes* a_first_cycle) {
    auto c = std::get<prism::MTDArray<int>>(*a_arg);
    bool do_single = vk_get_bool(a_single);
    bool do_multiply = vk_get_bool(a_mult);
    bool do_mix = vk_get_bool(a_mix);
    bool with_alpha = vk_get_bool(a_with_alpha);
    bool only_alpha = vk_get_bool(a_only_alpha);
    bool opt_alpha = vk_get_bool(a_alpha);
    bool first_cycle = vk_get_bool(a_first_cycle);
    std::string out;
    if (do_single) {
        out += vk_shader_item_to_str(c.at(only_alpha, 3), with_alpha, only_alpha, opt_alpha, first_cycle, false);
    } else if (do_multiply) {
        out += vk_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += " * ";
        out += vk_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
    } else if (do_mix) {
        out += "mix(";
        out += vk_shader_item_to_str(c.at(only_alpha, 1), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ", ";
        out += vk_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ", ";
        out += vk_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
        out += ")";
    } else {
        out += "(";
        out += vk_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += " - ";
        out += vk_shader_item_to_str(c.at(only_alpha, 1), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ") * ";
        out += vk_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
        out += " + ";
        out += vk_shader_item_to_str(c.at(only_alpha, 3), with_alpha, only_alpha, opt_alpha, first_cycle, false);
    }
    return new prism::ContextTypes{ out };
}

static std::optional<std::string> vulkan_include_fs(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    auto res = std::static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, false, init));
    if (res == nullptr) {
        return std::nullopt;
    }
    return *static_cast<std::string*>(res->GetRawPointer());
}

static std::string BuildVulkanShader(const CCFeatures& cc_features, bool vertex, bool threePoint) {
    prism::Processor processor;
    prism::ContextItems context = {
        { "BACKEND", "vulkan" },
        { "BACKEND_OPENGL", false },
        { "BACKEND_VULKAN", true },
        { "BACKEND_METAL", false },
        { "BACKEND_DIRECTX", false },
        { "VERTEX_SHADER", vertex ? 1 : 0 },
        { "o_c", M_ARRAY(cc_features.c, int, 2, 2, 4) },
        { "o_alpha", cc_features.opt_alpha },
        { "o_inputs", cc_features.numInputs },
        { "o_fog", cc_features.opt_fog },
        { "o_texture_edge", cc_features.opt_texture_edge },
        { "o_noise", cc_features.opt_noise },
        { "o_2cyc", cc_features.opt_2cyc },
        { "o_alpha_threshold", cc_features.opt_alpha_threshold },
        { "o_invisible", cc_features.opt_invisible },
        { "o_grayscale", cc_features.opt_grayscale },
        { "o_prim_depth", cc_features.opt_prim_depth },
        { "o_mip_lod", cc_features.opt_mip_lod },
        { "o_uses_lod", cc_features.opt_mip_lod || cc_features.uses_lod_frac ||
                            (cc_features.opt_tex_lod && cc_features.usedTextures[0] && cc_features.usedTextures[1]) },
        { "o_two_tile_lod", cc_features.opt_tex_lod && !cc_features.opt_mip_lod && cc_features.usedTextures[0] &&
                                cc_features.usedTextures[1] },
        { "o_shade", cc_features.opt_shade },
        { "o_lighting", cc_features.opt_lighting },
        { "o_point_lighting", cc_features.opt_point_lighting },
        { "o_texgen", cc_features.opt_texgen },
        { "o_texgen_linear", cc_features.opt_texgen_linear },
        { "o_textures", M_ARRAY(cc_features.usedTextures, bool, 2) },
        { "o_palette", M_ARRAY(cc_features.used_palette, bool, 2) },
        { "o_masks", M_ARRAY(cc_features.used_masks, bool, 2) },
        { "o_blend", M_ARRAY(cc_features.used_blend, bool, 2) },
        { "o_clamp", M_ARRAY(cc_features.clamp, bool, 2, 2) },
        { "o_do_mix", M_ARRAY(cc_features.do_mix, bool, 2, 2) },
        { "o_do_single", M_ARRAY(cc_features.do_single, bool, 2, 2) },
        { "o_do_multiply", M_ARRAY(cc_features.do_multiply, bool, 2, 2) },
        { "o_color_alpha_same", M_ARRAY(cc_features.color_alpha_same, bool, 2) },
        { "o_three_point_filtering", threePoint },
        { "FILTER_THREE_POINT", FILTER_THREE_POINT },
        { "FILTER_LINEAR", FILTER_LINEAR },
        { "FILTER_NONE", FILTER_NONE },
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
        { "append_formula", (InvokeFunc)vk_append_formula },
    };
    // Inject current values for @setting-declared tweakables (compile-time)
    for (const auto& [var, value] : Fast::gfx_get_shader_setting_values(cc_features.shader_id)) {
        context[var] = value;
    }
    processor.populate(context);

    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    const char* shaderName = Fast::gfx_get_shader(cc_features.shader_id);
    std::string path = "shaders/vulkan/default.shader.glsl";
    if (shaderName != nullptr) {
        path = std::string(shaderName) + ".glsl";
    }

    auto res = std::static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, false, init));
    if (res == nullptr) {
        SPDLOG_ERROR("Failed to load the Vulkan shader template, missing shaders/vulkan in the o2r?");
        abort();
    }

    processor.load(*static_cast<std::string*>(res->GetRawPointer()));
    processor.bind_include_loader(vulkan_include_fs);
    std::string result = processor.process();
    Fast::gfx_register_shader_settings(cc_features.shader_id, processor.settings());
    return result;
}

static std::vector<uint32_t> CompileGlslToSpirv(const std::string& source, bool vertex) {
    static shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    auto result = compiler.CompileGlslToSpv(source, vertex ? shaderc_vertex_shader : shaderc_fragment_shader,
                                            vertex ? "fast3d.vert" : "fast3d.frag", options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        SPDLOG_ERROR("Vulkan shader compilation failed: {}", result.GetErrorMessage());
        int line = 1;
        std::stringstream ss(source);
        std::string l;
        while (std::getline(ss, l)) {
            fprintf(stderr, "%4d| %s\n", line++, l.c_str());
        }
        abort();
    }
    return { result.cbegin(), result.cend() };
}

// MARK: - Helpers

static VkSamplerAddressMode gfx_cm_to_vk(uint32_t val, bool hasMirrorClamp) {
    switch (val) {
        case G_TX_NOMIRROR | G_TX_CLAMP:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case G_TX_MIRROR | G_TX_WRAP:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case G_TX_MIRROR | G_TX_CLAMP:
            return hasMirrorClamp ? VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
                                  : VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case G_TX_NOMIRROR | G_TX_WRAP:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
}

uint32_t GfxRenderingAPIVK::FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeBits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }
    SPDLOG_ERROR("Vulkan: no suitable memory type (bits {:x}, props {:x})", typeBits, (uint32_t)props);
    return 0;
}

void GfxRenderingAPIVK::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
                                     VkBuffer& buffer, VkDeviceMemory& memory, uint8_t** mapped) {
    VkBufferCreateInfo bi = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bi.size = size;
    bi.usage = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(mDevice, &bi, nullptr, &buffer));

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(mDevice, buffer, &req);
    VkMemoryAllocateInfo ai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, props);
    VK_CHECK(vkAllocateMemory(mDevice, &ai, nullptr, &memory));
    VK_CHECK(vkBindBufferMemory(mDevice, buffer, memory, 0));

    if (mapped != nullptr) {
        VK_CHECK(vkMapMemory(mDevice, memory, 0, VK_WHOLE_SIZE, 0, (void**)mapped));
    }
}

void GfxRenderingAPIVK::CreateImage(uint32_t width, uint32_t height, uint32_t mips, VkSampleCountFlagBits samples,
                                    VkFormat format, VkImageUsageFlags usage, VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo ii = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.extent = { width, height, 1 };
    ii.mipLevels = mips;
    ii.arrayLayers = 1;
    ii.format = format;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ii.usage = usage;
    ii.samples = samples;
    ii.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateImage(mDevice, &ii, nullptr, &image));

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(mDevice, image, &req);
    VkMemoryAllocateInfo ai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK(vkAllocateMemory(mDevice, &ai, nullptr, &memory));
    VK_CHECK(vkBindImageMemory(mDevice, image, memory, 0));
}

VkImageView GfxRenderingAPIVK::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect,
                                               uint32_t mips) {
    VkImageViewCreateInfo vi = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    vi.image = image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = format;
    vi.subresourceRange = { aspect, 0, mips, 0, 1 };
    VkImageView view = VK_NULL_HANDLE;
    VK_CHECK(vkCreateImageView(mDevice, &vi, nullptr, &view));
    return view;
}

void GfxRenderingAPIVK::TransitionToGeneral(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect,
                                            uint32_t mips) {
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = { aspect, 0, mips, 0, 1 };
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);
}

// Conservative everything-to-everything barrier used around blits/copies.
void GfxRenderingAPIVK::FullBarrier(VkCommandBuffer cmd) {
    VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
    barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0,
                         nullptr, 0, nullptr);
}

VkCommandBuffer GfxRenderingAPIVK::BeginOneShot() {
    VkCommandBufferAllocateInfo ai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.commandPool = mFrames[mFrameSlot].commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VkCommandBuffer cmd;
    VK_CHECK(vkAllocateCommandBuffers(mDevice, &ai, &cmd));
    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &bi));
    return cmd;
}

void GfxRenderingAPIVK::EndOneShot(VkCommandBuffer cmd) {
    VK_CHECK(vkEndCommandBuffer(cmd));
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    VK_CHECK(vkQueueSubmit(mQueue, 1, &si, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(mQueue));
    vkFreeCommandBuffers(mDevice, mFrames[mFrameSlot].commandPool, 1, &cmd);
}

// Canonical render pass per (msaa, hasDepth). All attachments LOAD/STORE in
// LAYOUT_GENERAL; MSAA passes add a resolve attachment (the sampled texture).
VkRenderPass GfxRenderingAPIVK::GetRenderPass(uint32_t msaaLevel, bool hasDepth) {
    uint32_t key = msaaLevel | (hasDepth ? 1u << 8 : 0);
    auto it = mRenderPassCache.find(key);
    if (it != mRenderPassCache.end()) {
        return it->second;
    }

    bool msaa = msaaLevel > 1;
    std::vector<VkAttachmentDescription> attachments;

    VkAttachmentDescription color = {};
    color.format = mSwapchainFormat;
    color.samples = (VkSampleCountFlagBits)msaaLevel;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
    color.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
    attachments.push_back(color);

    VkAttachmentReference colorRef = { 0, VK_IMAGE_LAYOUT_GENERAL };
    VkAttachmentReference resolveRef = { VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_GENERAL };
    VkAttachmentReference depthRef = { VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_GENERAL };

    if (msaa) {
        VkAttachmentDescription resolve = color;
        resolve.samples = VK_SAMPLE_COUNT_1_BIT;
        resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments.push_back(resolve);
        resolveRef.attachment = 1;
    }

    if (hasDepth) {
        VkAttachmentDescription depth = {};
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.samples = (VkSampleCountFlagBits)msaaLevel;
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
        depth.finalLayout = VK_IMAGE_LAYOUT_GENERAL;
        depthRef.attachment = (uint32_t)attachments.size();
        attachments.push_back(depth);
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pResolveAttachments = msaa ? &resolveRef : nullptr;
    subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

    // Conservative full external dependencies: framebuffer textures get
    // sampled by later framebuffers' draws within the same submission.
    VkSubpassDependency deps[2] = {};
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    deps[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    deps[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    deps[1].srcSubpass = 0;
    deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    deps[1].dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    deps[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    deps[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

    VkRenderPassCreateInfo rp = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rp.attachmentCount = (uint32_t)attachments.size();
    rp.pAttachments = attachments.data();
    rp.subpassCount = 1;
    rp.pSubpasses = &subpass;
    rp.dependencyCount = 2;
    rp.pDependencies = deps;

    VkRenderPass pass = VK_NULL_HANDLE;
    VK_CHECK(vkCreateRenderPass(mDevice, &rp, nullptr, &pass));
    mRenderPassCache[key] = pass;
    return pass;
}

VkSampler GfxRenderingAPIVK::GetSampler(bool linear, uint32_t cms, uint32_t cmt, bool autoMipmap) {
    uint32_t key = (linear ? 1u : 0) | (cms << 1) | (cmt << 9) | (autoMipmap ? (1u << 17) : 0);
    auto it = mSamplerCache.find(key);
    if (it != mSamplerCache.end()) {
        return it->second;
    }

    VkSamplerCreateInfo si = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    VkFilter filter = linear && mCurrentFilterMode == FILTER_LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    si.magFilter = filter;
    si.minFilter = filter;
    // Native N64 mip chains are sampled with explicit integer LODs (textureLod) in
    // the shader; NEAREST picks the exact level. CPU auto-generated pyramids instead
    // use hardware derivative LOD with trilinear blending (no per-pixel level stipple)
    // plus a negative bias and anisotropy for grazing-angle surfaces.
    si.mipmapMode = autoMipmap ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    si.addressModeU = gfx_cm_to_vk(cms, mHasMirrorClampToEdge);
    si.addressModeV = gfx_cm_to_vk(cmt, mHasMirrorClampToEdge);
    si.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    si.minLod = 0.0f;
    si.maxLod = VK_LOD_CLAMP_NONE;
    si.mipLodBias = autoMipmap ? -1.0f : 0.0f;
    if (autoMipmap && mHasAnisotropy) {
        si.anisotropyEnable = VK_TRUE;
        si.maxAnisotropy = std::min(8.0f, mDeviceProps.limits.maxSamplerAnisotropy);
    }

    VkSampler sampler = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSampler(mDevice, &si, nullptr, &sampler));
    mSamplerCache[key] = sampler;
    return sampler;
}

void GfxRenderingAPIVK::EnsureUploadCmd() {
    if (mUploadCmd != VK_NULL_HANDLE) {
        return;
    }
    VkCommandBufferAllocateInfo ai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.commandPool = mFrames[mFrameSlot].commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(mDevice, &ai, &mUploadCmd));
    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(mUploadCmd, &bi));
}

// Submit whatever uploads have been recorded so far and wait. Used when a
// mid-frame readback needs textures uploaded earlier in the same frame.
void GfxRenderingAPIVK::FlushUploads() {
    if (mUploadCmd == VK_NULL_HANDLE) {
        return;
    }
    VK_CHECK(vkEndCommandBuffer(mUploadCmd));
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers = &mUploadCmd;
    VK_CHECK(vkQueueSubmit(mQueue, 1, &si, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(mQueue));
    mUploadCmd = VK_NULL_HANDLE;
    mStagingOffset = 0;
}

void GfxRenderingAPIVK::GrowStaging(FrameSlot& slot, size_t needed) {
    // Defer-destroy the old buffer: copies already recorded this frame still reference it,
    // and it's freed in CollectGarbage after this slot's fence — so we can swap in a bigger
    // buffer WITHOUT a vkQueueWaitIdle stall. New copies use the new buffer; bytes already
    // staged in [0, mStagingOffset) stay valid in the (deferred) old one.
    size_t newCap = needed + (needed >> 2); // +25% headroom so we don't re-grow next texture
    slot.garbageBuffers.push_back(slot.stagingBuffer);
    slot.garbageMemory.push_back(slot.stagingMemory);
    slot.stagingCapacity = newCap;
    CreateBuffer(newCap, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, slot.stagingBuffer,
                 slot.stagingMemory, &slot.stagingMapped);
    SPDLOG_INFO("Vulkan: grew staging buffer to {} MB", newCap / (1024 * 1024));
}

// MARK: - Swapchain

void GfxRenderingAPIVK::DestroySwapchainViews() {
    for (VkFramebuffer fb : mSwapchainFramebuffers) {
        vkDestroyFramebuffer(mDevice, fb, nullptr);
    }
    mSwapchainFramebuffers.clear();
    for (VkImageView view : mSwapchainViews) {
        vkDestroyImageView(mDevice, view, nullptr);
    }
    mSwapchainViews.clear();
}

void GfxRenderingAPIVK::CreateSwapchain(uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(mDevice);
    DestroySwapchainViews();

    VkSurfaceCapabilitiesKHR caps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &caps));

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, formats.data());

    VkSurfaceFormatKHR chosen = formats[0];
    for (const auto& f : formats) {
        if ((f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM) &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = f;
            break;
        }
    }
    mSwapchainFormat = chosen.format;

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        extent = { std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width),
                   std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height) };
    }
    mSwapchainExtent = extent;

    uint32_t imageCount = std::max(caps.minImageCount + 1, 3u);
    if (caps.maxImageCount > 0) {
        imageCount = std::min(imageCount, caps.maxImageCount);
    }

    VkSwapchainCreateInfoKHR sci = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    sci.surface = mSurface;
    sci.minImageCount = imageCount;
    sci.imageFormat = chosen.format;
    sci.imageColorSpace = chosen.colorSpace;
    sci.imageExtent = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped = VK_TRUE;
    sci.oldSwapchain = mSwapchain;

    VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSwapchainKHR(mDevice, &sci, nullptr, &newSwapchain));
    if (mSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
    }
    mSwapchain = newSwapchain;

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &count, nullptr);
    mSwapchainImages.resize(count);
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &count, mSwapchainImages.data());

    mSwapchainViews.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        mSwapchainViews[i] = CreateImageView(mSwapchainImages[i], mSwapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
    mSwapchainImageUsed.assign(count, false);

    mSwapchainNeedsRecreate = false;
    RebuildScreenFramebuffer();
}

// (Re)creates the screen depth buffer and the per-swapchain-image framebuffers.
void GfxRenderingAPIVK::RebuildScreenFramebuffer() {
    if (mFramebuffers.empty()) {
        return; // Init() hasn't created fb 0 yet; called again afterwards
    }

    FramebufferVK& fb = mFramebuffers[0];
    TextureDataVK& tex = mTextures[fb.mTextureId];

    uint32_t width = mSwapchainExtent.width;
    uint32_t height = mSwapchainExtent.height;

    if (fb.mDepthImage == VK_NULL_HANDLE || fb.mWidth != width || fb.mHeight != height) {
        if (fb.mDepthView != VK_NULL_HANDLE) {
            vkDestroyImageView(mDevice, fb.mDepthView, nullptr);
            vkDestroyImage(mDevice, fb.mDepthImage, nullptr);
            vkFreeMemory(mDevice, fb.mDepthMemory, nullptr);
        }
        CreateImage(width, height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_D32_SFLOAT,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, fb.mDepthImage,
                    fb.mDepthMemory);
        fb.mDepthView = CreateImageView(fb.mDepthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

        VkCommandBuffer cmd = BeginOneShot();
        TransitionToGeneral(cmd, fb.mDepthImage, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        EndOneShot(cmd);
    }

    fb.mWidth = width;
    fb.mHeight = height;
    fb.mMsaaLevel = 1;
    fb.mHasDepthBuffer = true;
    fb.mRenderTarget = true;
    fb.mRenderPass = GetRenderPass(1, true);
    tex.width = width;
    tex.height = height;
    tex.isSwapchainAlias = true;

    for (VkFramebuffer f : mSwapchainFramebuffers) {
        vkDestroyFramebuffer(mDevice, f, nullptr);
    }
    mSwapchainFramebuffers.resize(mSwapchainViews.size());
    for (size_t i = 0; i < mSwapchainViews.size(); i++) {
        VkImageView attachments[2] = { mSwapchainViews[i], fb.mDepthView };
        VkFramebufferCreateInfo fci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fci.renderPass = fb.mRenderPass;
        fci.attachmentCount = 2;
        fci.pAttachments = attachments;
        fci.width = width;
        fci.height = height;
        fci.layers = 1;
        VK_CHECK(vkCreateFramebuffer(mDevice, &fci, nullptr, &mSwapchainFramebuffers[i]));
    }
}

// MARK: - ImGui & SDL hooks (called from Fast3dGui)

bool GfxRenderingAPIVK::VulkanInit(SDL_Window* window) {
    mWindow = window;

    // Instance
    uint32_t sdlExtCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &sdlExtCount, nullptr);
    std::vector<const char*> instanceExts(sdlExtCount);
    SDL_Vulkan_GetInstanceExtensions(window, &sdlExtCount, instanceExts.data());

    uint32_t availExtCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availExtCount, nullptr);
    std::vector<VkExtensionProperties> availExts(availExtCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availExtCount, availExts.data());
    bool hasPortabilityEnum = false;
    for (const auto& e : availExts) {
        if (strcmp(e.extensionName, "VK_KHR_portability_enumeration") == 0) {
            hasPortabilityEnum = true;
        }
    }
    if (hasPortabilityEnum) {
        instanceExts.push_back("VK_KHR_portability_enumeration");
    }

    VkApplicationInfo app = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app.pApplicationName = "libultraship";
    app.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo ici = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = (uint32_t)instanceExts.size();
    ici.ppEnabledExtensionNames = instanceExts.data();
    if (hasPortabilityEnum) {
        ici.flags |= 0x00000001; // VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
    }
    if (vkCreateInstance(&ici, nullptr, &mInstance) != VK_SUCCESS) {
        SPDLOG_ERROR("Vulkan: vkCreateInstance failed");
        return false;
    }

    if (!SDL_Vulkan_CreateSurface(window, mInstance, &mSurface)) {
        SPDLOG_ERROR("Vulkan: SDL_Vulkan_CreateSurface failed: {}", SDL_GetError());
        return false;
    }

    // Physical device + queue family with graphics & present
    uint32_t devCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &devCount, nullptr);
    if (devCount == 0) {
        SPDLOG_ERROR("Vulkan: no physical devices");
        return false;
    }
    std::vector<VkPhysicalDevice> devices(devCount);
    vkEnumeratePhysicalDevices(mInstance, &devCount, devices.data());

    for (VkPhysicalDevice dev : devices) {
        uint32_t qCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, nullptr);
        std::vector<VkQueueFamilyProperties> queues(qCount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, queues.data());
        for (uint32_t i = 0; i < qCount; i++) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, mSurface, &present);
            if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
                mPhysicalDevice = dev;
                mQueueFamily = i;
                break;
            }
        }
        if (mPhysicalDevice != VK_NULL_HANDLE) {
            break;
        }
    }
    if (mPhysicalDevice == VK_NULL_HANDLE) {
        SPDLOG_ERROR("Vulkan: no device with graphics+present support");
        return false;
    }
    vkGetPhysicalDeviceProperties(mPhysicalDevice, &mDeviceProps);
    SPDLOG_INFO("Vulkan device: {}", mDeviceProps.deviceName);

    // Device
    uint32_t devExtCount = 0;
    vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &devExtCount, nullptr);
    std::vector<VkExtensionProperties> devExts(devExtCount);
    vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &devExtCount, devExts.data());
    std::vector<const char*> enabledDevExts = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    for (const auto& e : devExts) {
        if (strcmp(e.extensionName, "VK_KHR_portability_subset") == 0) {
            enabledDevExts.push_back("VK_KHR_portability_subset");
        }
        if (strcmp(e.extensionName, VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME) == 0) {
            enabledDevExts.push_back(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME);
            mHasMirrorClampToEdge = true;
        }
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    qci.queueFamilyIndex = mQueueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures supported = {};
    vkGetPhysicalDeviceFeatures(mPhysicalDevice, &supported);
    VkPhysicalDeviceFeatures features = {};
    features.sampleRateShading = supported.sampleRateShading;
    features.depthClamp = supported.depthClamp;
    features.samplerAnisotropy = supported.samplerAnisotropy;
    mHasDepthClamp = supported.depthClamp;
    mHasAnisotropy = supported.samplerAnisotropy;

    VkDeviceCreateInfo dci = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = (uint32_t)enabledDevExts.size();
    dci.ppEnabledExtensionNames = enabledDevExts.data();
    dci.pEnabledFeatures = &features;
    if (vkCreateDevice(mPhysicalDevice, &dci, nullptr, &mDevice) != VK_SUCCESS) {
        SPDLOG_ERROR("Vulkan: vkCreateDevice failed");
        return false;
    }
    vkGetDeviceQueue(mDevice, mQueueFamily, 0, &mQueue);

    // Descriptor set layouts + pipeline layout
    {
        VkDescriptorSetLayoutBinding set0Bindings[4] = {};
        for (uint32_t i = 0; i < 4; i++) {
            set0Bindings[i].binding = i;
            set0Bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            set0Bindings[i].descriptorCount = 1;
            set0Bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        VkDescriptorSetLayoutCreateInfo l0 = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        l0.bindingCount = 4;
        l0.pBindings = set0Bindings;
        VK_CHECK(vkCreateDescriptorSetLayout(mDevice, &l0, nullptr, &mSet0Layout));

        VkDescriptorSetLayoutBinding set1Bindings[SHADER_MAX_TEXTURES] = {};
        for (uint32_t i = 0; i < SHADER_MAX_TEXTURES; i++) {
            set1Bindings[i].binding = i;
            set1Bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            set1Bindings[i].descriptorCount = 1;
            set1Bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        VkDescriptorSetLayoutCreateInfo l1 = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        l1.bindingCount = SHADER_MAX_TEXTURES;
        l1.pBindings = set1Bindings;
        VK_CHECK(vkCreateDescriptorSetLayout(mDevice, &l1, nullptr, &mSet1Layout));

        VkDescriptorSetLayout layouts[2] = { mSet0Layout, mSet1Layout };
        VkPipelineLayoutCreateInfo pli = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pli.setLayoutCount = 2;
        pli.pSetLayouts = layouts;
        VK_CHECK(vkCreatePipelineLayout(mDevice, &pli, nullptr, &mPipelineLayout));
    }

    // Per-frame slots
    mVertexRingCapacity = 256 * 32 * 3 * sizeof(float) * 50;
    mUniformRingCapacity = 8 * 1024 * 1024;
    for (uint32_t i = 0; i < VK_FRAMES_IN_FLIGHT; i++) {
        FrameSlot& slot = mFrames[i];

        VkCommandPoolCreateInfo cpi = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        cpi.queueFamilyIndex = mQueueFamily;
        cpi.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        VK_CHECK(vkCreateCommandPool(mDevice, &cpi, nullptr, &slot.commandPool));

        VkDescriptorPoolSize poolSizes[2] = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16384 * SHADER_MAX_TEXTURES },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 8 },
        };
        VkDescriptorPoolCreateInfo dpi = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        dpi.maxSets = 16384;
        dpi.poolSizeCount = 2;
        dpi.pPoolSizes = poolSizes;
        VK_CHECK(vkCreateDescriptorPool(mDevice, &dpi, nullptr, &slot.descriptorPool));

        VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        VK_CHECK(vkCreateFence(mDevice, &fci, nullptr, &slot.fence));
        VkSemaphoreCreateInfo sci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CHECK(vkCreateSemaphore(mDevice, &sci, nullptr, &slot.imageAvailable));
        VK_CHECK(vkCreateSemaphore(mDevice, &sci, nullptr, &slot.renderFinished));

        CreateBuffer(mVertexRingCapacity, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, slot.vertexBuffer,
                     slot.vertexMemory, &slot.vertexMapped);
        CreateBuffer(mUniformRingCapacity, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, slot.uniformBuffer,
                     slot.uniformMemory, &slot.uniformMapped);
        slot.stagingCapacity = 32 * 1024 * 1024;
        CreateBuffer(slot.stagingCapacity, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, slot.stagingBuffer,
                     slot.stagingMemory, &slot.stagingMapped);

        // The set 0 descriptor binds the slot's uniform ring at offset 0 with
        // per-draw dynamic offsets. One set per slot, written once.
        VkDescriptorSetAllocateInfo dsa = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        dsa.descriptorPool = slot.descriptorPool;
        dsa.descriptorSetCount = 1;
        dsa.pSetLayouts = &mSet0Layout;
        VK_CHECK(vkAllocateDescriptorSets(mDevice, &dsa, &slot.set0));

        VkDescriptorBufferInfo bufferInfos[4];
        VkWriteDescriptorSet writes[4] = {};
        const VkDeviceSize ranges[4] = { sizeof(VulkanFrameUniforms), sizeof(VulkanDrawUniforms),
                                         sizeof(LightingUniforms), sizeof(TransformUniforms) };
        for (uint32_t b = 0; b < 4; b++) {
            bufferInfos[b] = { slot.uniformBuffer, 0, ranges[b] };
            writes[b].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[b].dstSet = slot.set0;
            writes[b].dstBinding = b;
            writes[b].descriptorCount = 1;
            writes[b].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            writes[b].pBufferInfo = &bufferInfos[b];
        }
        vkUpdateDescriptorSets(mDevice, 4, writes, 0, nullptr);
    }

    // 1x1 white dummy texture for unused sampler slots
    {
        CreateImage(1, 1, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, mDummyTexture.image,
                    mDummyTexture.memory);
        mDummyTexture.view =
            CreateImageView(mDummyTexture.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        mDummyTexture.sampler = GetSampler(false, G_TX_WRAP, G_TX_WRAP);

        VkCommandBuffer cmd = BeginOneShot();
        TransitionToGeneral(cmd, mDummyTexture.image, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        VkClearColorValue white = { { 1.0f, 1.0f, 1.0f, 1.0f } };
        VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdClearColorImage(cmd, mDummyTexture.image, VK_IMAGE_LAYOUT_GENERAL, &white, 1, &range);
        FullBarrier(cmd);
        EndOneShot(cmd);
    }

    // Initial swapchain
    int dw = 0, dh = 0;
    SDL_Vulkan_GetDrawableSize(window, &dw, &dh);
    CreateSwapchain((uint32_t)dw, (uint32_t)dh);

    // ImGui
    ImGui_ImplVulkan_InitInfo info = {};
    info.ApiVersion = VK_API_VERSION_1_1;
    info.Instance = mInstance;
    info.PhysicalDevice = mPhysicalDevice;
    info.Device = mDevice;
    info.QueueFamily = mQueueFamily;
    info.Queue = mQueue;
    info.RenderPass = GetRenderPass(1, true);
    info.MinImageCount = 2;
    info.ImageCount = (uint32_t)mSwapchainImages.size();
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.DescriptorPoolSize = 512;
    if (!ImGui_ImplVulkan_Init(&info)) {
        SPDLOG_ERROR("Vulkan: ImGui_ImplVulkan_Init failed");
        return false;
    }
    mImGuiInitialized = true;
    return true;
}

void GfxRenderingAPIVK::ShutdownImGui() {
    if (mImGuiInitialized) {
        vkDeviceWaitIdle(mDevice);
        ImGui_ImplVulkan_Shutdown();
        mImGuiInitialized = false;
    }
}

void GfxRenderingAPIVK::CollectGarbage(FrameSlot& slot) {
    for (auto& [img, mem] : slot.garbageImages) {
        vkDestroyImage(mDevice, img, nullptr);
        vkFreeMemory(mDevice, mem, nullptr);
    }
    slot.garbageImages.clear();
    for (VkImageView view : slot.garbageViews) {
        vkDestroyImageView(mDevice, view, nullptr);
    }
    slot.garbageViews.clear();
    for (VkBuffer buf : slot.garbageBuffers) {
        vkDestroyBuffer(mDevice, buf, nullptr);
    }
    slot.garbageBuffers.clear();
    for (VkDeviceMemory mem : slot.garbageMemory) {
        vkFreeMemory(mDevice, mem, nullptr);
    }
    slot.garbageMemory.clear();
}

void GfxRenderingAPIVK::NewFrame() {
    FrameSlot& slot = mFrames[mFrameSlot];

    if (slot.fenceInFlight) {
        VK_CHECK(vkWaitForFences(mDevice, 1, &slot.fence, VK_TRUE, UINT64_MAX));
        VK_CHECK(vkResetFences(mDevice, 1, &slot.fence));
        slot.fenceInFlight = false;
    }
    CollectGarbage(slot);
    VK_CHECK(vkResetCommandPool(mDevice, slot.commandPool, 0));
    VK_CHECK(vkResetDescriptorPool(mDevice, slot.descriptorPool, 0));

    // Re-create the static set 0 (pool reset destroyed it)
    {
        VkDescriptorSetAllocateInfo dsa = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        dsa.descriptorPool = slot.descriptorPool;
        dsa.descriptorSetCount = 1;
        dsa.pSetLayouts = &mSet0Layout;
        VK_CHECK(vkAllocateDescriptorSets(mDevice, &dsa, &slot.set0));

        VkDescriptorBufferInfo bufferInfos[4];
        VkWriteDescriptorSet writes[4] = {};
        const VkDeviceSize ranges[4] = { sizeof(VulkanFrameUniforms), sizeof(VulkanDrawUniforms),
                                         sizeof(LightingUniforms), sizeof(TransformUniforms) };
        for (uint32_t b = 0; b < 4; b++) {
            bufferInfos[b] = { slot.uniformBuffer, 0, ranges[b] };
            writes[b].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[b].dstSet = slot.set0;
            writes[b].dstBinding = b;
            writes[b].descriptorCount = 1;
            writes[b].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            writes[b].pBufferInfo = &bufferInfos[b];
        }
        vkUpdateDescriptorSets(mDevice, 4, writes, 0, nullptr);
    }

    mVertexRingOffset = 0;
    mUniformRingOffset = 0;
    mStagingOffset = 0;
    mUploadCmd = VK_NULL_HANDLE;
    mFrameUniformOffset = UINT32_MAX;
    mDrawUniformOffset = UINT32_MAX;
    mLightingUniformOffset = UINT32_MAX;
    mTransformUniformOffset = UINT32_MAX;
    mFrameUniformsDirty = true;

    // Swapchain housekeeping
    int dw = 0, dh = 0;
    SDL_Vulkan_GetDrawableSize(mWindow, &dw, &dh);
    if (mSwapchainNeedsRecreate || (uint32_t)dw != mSwapchainExtent.width || (uint32_t)dh != mSwapchainExtent.height) {
        CreateSwapchain((uint32_t)dw, (uint32_t)dh);
    }

    VkResult acquire = vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX, slot.imageAvailable, VK_NULL_HANDLE,
                                             &mSwapchainImageIndex);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR || acquire == VK_SUBOPTIMAL_KHR) {
        CreateSwapchain((uint32_t)dw, (uint32_t)dh);
        acquire = vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX, slot.imageAvailable, VK_NULL_HANDLE,
                                        &mSwapchainImageIndex);
    }
    if (acquire != VK_SUCCESS) {
        SPDLOG_ERROR("Vulkan: vkAcquireNextImageKHR failed ({})", (int)acquire);
    }

    // fb 0's texture aliases the acquired swapchain image
    if (!mFramebuffers.empty()) {
        TextureDataVK& tex = mTextures[mFramebuffers[0].mTextureId];
        tex.view = mSwapchainViews[mSwapchainImageIndex];
        tex.image = mSwapchainImages[mSwapchainImageIndex];
        tex.width = mSwapchainExtent.width;
        tex.height = mSwapchainExtent.height;
        mFramebuffers[0].mFramebuffer = mSwapchainFramebuffers[mSwapchainImageIndex];
    }

    mFrameActive = true;
    ImGui_ImplVulkan_NewFrame();
}

void GfxRenderingAPIVK::RenderDrawData(ImDrawData* drawData) {
    EnsurePassActive(0);
    FramebufferVK& fb = mFramebuffers[0];
    ImGui_ImplVulkan_RenderDrawData(drawData, fb.mCommandBuffer);
    // ImGui leaves arbitrary pipeline/descriptor/viewport state behind
    fb.mLastPipeline = VK_NULL_HANDLE;
    fb.mLastShaderProgram = nullptr;
    for (uint32_t i = 0; i < 4; i++) {
        fb.mLastSet0Offsets[i] = UINT32_MAX;
    }
    for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
        fb.mLastBoundViews[i] = nullptr;
        fb.mLastBoundSamplers[i] = nullptr;
    }
    vkCmdSetViewport(fb.mCommandBuffer, 0, 1, &fb.mViewport);
    vkCmdSetScissor(fb.mCommandBuffer, 0, 1, &fb.mScissor);
}

// MARK: - GfxRenderingAPI implementation

const char* GfxRenderingAPIVK::GetName() {
    return "Vulkan";
}

int GfxRenderingAPIVK::GetMaxTextureSize() {
    return (int)mDeviceProps.limits.maxImageDimension2D;
}

GfxClipParameters GfxRenderingAPIVK::GetClipParameters() {
    // Same conventions as Metal: z in 0..1, no VS-side y inversion (the
    // negative viewport gives the framebuffer the same orientation).
    return { true, false };
}

void GfxRenderingAPIVK::Init() {
    // fb 0 represents the swapchain
    CreateFramebuffer();
    RebuildScreenFramebuffer();
}

void GfxRenderingAPIVK::UnloadShader(ShaderProgram* oldPrg) {
}

void GfxRenderingAPIVK::LoadShader(ShaderProgram* newPrg) {
    mShaderProgram = (ShaderProgramVK*)newPrg;
}

void GfxRenderingAPIVK::ClearShaderCache() {
    for (auto& [key, prg] : mShaderProgramPool) {
        prg.markedForDeletion = true;
    }
}

ShaderProgram* GfxRenderingAPIVK::CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) {
    CCFeatures cc_features;
    gfx_cc_get_features(shaderId0, shaderId1, &cc_features);

    std::string vsSource = BuildVulkanShader(cc_features, true, mCurrentFilterMode == FILTER_THREE_POINT);
    std::string fsSource = BuildVulkanShader(cc_features, false, mCurrentFilterMode == FILTER_THREE_POINT);

    std::vector<uint32_t> vsSpirv = CompileGlslToSpirv(vsSource, true);
    std::vector<uint32_t> fsSpirv = CompileGlslToSpirv(fsSource, false);

    ShaderProgramVK* prg = &mShaderProgramPool[std::make_pair(shaderId0, shaderId1)];
    prg->shader_id0 = shaderId0;
    prg->shader_id1 = shaderId1;
    prg->usedTextures[0] = cc_features.usedTextures[0];
    prg->usedTextures[1] = cc_features.usedTextures[1];
    prg->usedTextures[2] = cc_features.used_masks[0];
    prg->usedTextures[3] = cc_features.used_masks[1];
    prg->usedTextures[4] = cc_features.used_blend[0];
    prg->usedTextures[5] = cc_features.used_blend[1];
    prg->usedTextures[SHADER_PALETTE_TEXTURE] = cc_features.used_palette[0] || cc_features.used_palette[1];
    prg->numInputs = cc_features.numInputs;
    prg->usedLighting = cc_features.opt_lighting || cc_features.opt_texgen;
    prg->useAlphaBlend = cc_features.opt_alpha;
    prg->markedForDeletion = false;
    prg->pipelines.clear();

    VkShaderModuleCreateInfo smci = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    smci.codeSize = vsSpirv.size() * sizeof(uint32_t);
    smci.pCode = vsSpirv.data();
    VK_CHECK(vkCreateShaderModule(mDevice, &smci, nullptr, &prg->vs));
    smci.codeSize = fsSpirv.size() * sizeof(uint32_t);
    smci.pCode = fsSpirv.data();
    VK_CHECK(vkCreateShaderModule(mDevice, &smci, nullptr, &prg->fs));

    // Vertex layout: matches the interpreter's packing order
    uint32_t offset = 0;
    uint32_t count = 0;
    prg->attribs[count++] = { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offset }; // aVtxPos
    offset += 16;
    prg->attribs[count++] = { 1, 0, VK_FORMAT_R32_SFLOAT, offset }; // aMtxSlot
    offset += 4;
    if (cc_features.usedTextures[0]) {
        prg->attribs[count++] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, offset };
        offset += 8;
    }
    if (cc_features.usedTextures[1]) {
        prg->attribs[count++] = { 3, 0, VK_FORMAT_R32G32_SFLOAT, offset };
        offset += 8;
    }
    if (cc_features.opt_shade || cc_features.opt_lighting) {
        prg->attribs[count++] = { 4, 0,
                                  cc_features.opt_alpha ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R32G32B32_SFLOAT,
                                  offset };
        offset += cc_features.opt_alpha ? 16 : 12;
    }
    prg->attribCount = count;
    prg->numFloats = offset / 4;

    LoadShader((ShaderProgram*)prg);
    return (ShaderProgram*)prg;
}

ShaderProgram* GfxRenderingAPIVK::LookupShader(uint64_t shaderId0, uint64_t shaderId1) {
    auto it = mShaderProgramPool.find(std::make_pair(shaderId0, shaderId1));
    if (it == mShaderProgramPool.end()) {
        return nullptr;
    }
    if (it->second.markedForDeletion) {
        vkDeviceWaitIdle(mDevice);
        for (auto& [key, pipeline] : it->second.pipelines) {
            vkDestroyPipeline(mDevice, pipeline, nullptr);
        }
        vkDestroyShaderModule(mDevice, it->second.vs, nullptr);
        vkDestroyShaderModule(mDevice, it->second.fs, nullptr);
        mShaderProgramPool.erase(it);
        return nullptr;
    }
    return (ShaderProgram*)&it->second;
}

void GfxRenderingAPIVK::ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) {
    ShaderProgramVK* p = (ShaderProgramVK*)prg;
    *numInputs = p->numInputs;
    usedTextures[0] = p->usedTextures[0];
    usedTextures[1] = p->usedTextures[1];
}

uint32_t GfxRenderingAPIVK::NewTexture() {
    mTextures.resize(mTextures.size() + 1);
    return (uint32_t)(mTextures.size() - 1);
}

void GfxRenderingAPIVK::DestroyTextureData(TextureDataVK& tex, bool deferred) {
    if (tex.isSwapchainAlias) {
        return;
    }
    if (tex.imguiSet != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(tex.imguiSet);
        tex.imguiSet = VK_NULL_HANDLE;
    }
    if (tex.view != VK_NULL_HANDLE) {
        if (deferred) {
            FrameSlot& slot = mFrames[mFrameSlot];
            slot.garbageViews.push_back(tex.view);
            slot.garbageImages.push_back({ tex.image, tex.memory });
        } else {
            vkDestroyImageView(mDevice, tex.view, nullptr);
            vkDestroyImage(mDevice, tex.image, nullptr);
            vkFreeMemory(mDevice, tex.memory, nullptr);
        }
    }
    tex.view = VK_NULL_HANDLE;
    tex.image = VK_NULL_HANDLE;
    tex.memory = VK_NULL_HANDLE;
}

void GfxRenderingAPIVK::DeleteTexture(uint32_t texId) {
    if (texId < mTextures.size()) {
        DestroyTextureData(mTextures[texId], true);
    }
}

void GfxRenderingAPIVK::SelectTexture(int tile, uint32_t textureId) {
    mCurrentTile = tile;
    mCurrentTextureIds[tile] = textureId;
}

void GfxRenderingAPIVK::UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) {
    UploadTextureMip(rgba32Buf, width, height, 0, 1);
}

void GfxRenderingAPIVK::UploadTextureMip(const uint8_t* rgba32Buf, uint32_t width, uint32_t height, uint32_t level,
                                         uint32_t totalLevels) {
    if (width == 0 || height == 0) {
        return;
    }

    TextureDataVK& tex = mTextures[mCurrentTextureIds[mCurrentTile]];

    bool oneShot = !mFrameActive;
    VkCommandBuffer cmd;
    if (oneShot) {
        cmd = BeginOneShot();
    } else {
        EnsureUploadCmd();
        cmd = mUploadCmd;
    }

    if (level == 0) {
        if (tex.image == VK_NULL_HANDLE || tex.width != width || tex.height != height ||
            tex.mip_levels != totalLevels) {
            DestroyTextureData(tex, true);
            CreateImage(width, height, totalLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM,
                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, tex.image, tex.memory);
            tex.view = CreateImageView(tex.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, totalLevels);
            tex.width = width;
            tex.height = height;
            tex.mip_levels = totalLevels;
            TransitionToGeneral(cmd, tex.image, VK_IMAGE_ASPECT_COLOR_BIT, totalLevels);
        }
        tex.auto_mipmaps = mNextTextureAutoMipmap;
    }

    size_t dataSize = (size_t)width * height * 4;
    FrameSlot& slot = mFrames[mFrameSlot];
    if (oneShot) {
        // Mid-init upload: stage through the slot's buffer from offset 0
        if (dataSize > slot.stagingCapacity) {
            // Grow to fit (e.g. a 4K HD replacement = 64 MB > the 32 MB default). The
            // image transition recorded so far is submitted first; EndOneShot waits idle.
            EndOneShot(cmd);
            GrowStaging(slot, dataSize);
            cmd = BeginOneShot();
        }
        memcpy(slot.stagingMapped, rgba32Buf, dataSize);
    } else {
        if (mStagingOffset + dataSize > slot.stagingCapacity) {
            // Out of staging space this frame. Grow the buffer (deferred-destroy keeps the
            // already-recorded copies valid) instead of a synchronous FlushUploads, whose
            // vkQueueWaitIdle stalls the whole frame — the source of the upload hitch.
            GrowStaging(slot, mStagingOffset + dataSize);
            cmd = mUploadCmd;
        }
        memcpy(slot.stagingMapped + mStagingOffset, rgba32Buf, dataSize);
    }

    VkBufferImageCopy copy = {};
    copy.bufferOffset = oneShot ? 0 : mStagingOffset;
    copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1 };
    copy.imageExtent = { width, height, 1 };
    vkCmdCopyBufferToImage(cmd, slot.stagingBuffer, tex.image, VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
    FullBarrier(cmd);

    if (oneShot) {
        EndOneShot(cmd);
    } else {
        mStagingOffset += (dataSize + 255) & ~255ull;
    }

    if (level == 0 && totalLevels == 1) {
        tex.mip_levels = 1;
    }
}

void GfxRenderingAPIVK::SetSamplerParameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt) {
    TextureDataVK& tex = mTextures[mCurrentTextureIds[tile]];
    tex.linear_filtering = linear_filter;
    tex.filtering = !linear_filter ? FILTER_LINEAR : FILTER_THREE_POINT;
    tex.sampler = GetSampler(linear_filter, cms, cmt, tex.auto_mipmaps);
}

void GfxRenderingAPIVK::SetCurrentPrimDepth(float depth) {
    if (depth != mCurrentPrimDepth) {
        mCurrentPrimDepth = depth;
        mPrimDepthDirty = true;
    }
}

void GfxRenderingAPIVK::SetCurrentMaxLod(float maxLod) {
    if (maxLod != mCurrentMaxLod) {
        mCurrentMaxLod = maxLod;
        mLodMaxDirty = true;
    }
}

void GfxRenderingAPIVK::SetDepthTestAndMask(bool depth_test, bool z_upd) {
    mCurrentDepthTest = depth_test;
    mCurrentDepthMask = z_upd;
}

void GfxRenderingAPIVK::SetZmodeDecal(bool decal) {
    mCurrentZmodeDecal = decal;
}

void GfxRenderingAPIVK::SetStrictDecal(bool on) {
    mCurrentStrictDecal = on;
}

void GfxRenderingAPIVK::SetViewport(int x, int y, int width, int height) {
    FramebufferVK& fb = mFramebuffers[mCurrentFramebuffer];
    // Negative-height viewport (VK_KHR_maintenance1, core 1.1) flips y so the
    // framebuffer ends up with the same orientation as Metal/GL.
    float top = (float)(mRenderTargetHeight - y - height);
    fb.mViewport.x = (float)x;
    fb.mViewport.y = top + (float)height;
    fb.mViewport.width = (float)width;
    fb.mViewport.height = -(float)height;
    fb.mViewport.minDepth = 0.0f;
    fb.mViewport.maxDepth = 1.0f;
    if (fb.mPassActive) {
        vkCmdSetViewport(fb.mCommandBuffer, 0, 1, &fb.mViewport);
    }
}

void GfxRenderingAPIVK::SetScissor(int x, int y, int width, int height) {
    FramebufferVK& fb = mFramebuffers[mCurrentFramebuffer];
    TextureDataVK& tex = mTextures[fb.mTextureId];
    int32_t sx = std::max(0, std::min<int>(x, tex.width));
    int32_t sy = std::max(0, std::min<int>(mRenderTargetHeight - y - height, tex.height));
    fb.mScissor.offset = { sx, sy };
    fb.mScissor.extent = { (uint32_t)std::max(0, std::min<int>(width, (int)tex.width - sx)),
                           (uint32_t)std::max(0, std::min<int>(height, (int)tex.height - sy)) };
    if (fb.mPassActive) {
        vkCmdSetScissor(fb.mCommandBuffer, 0, 1, &fb.mScissor);
    }
}

void GfxRenderingAPIVK::SetUseAlpha(bool useAlpha) {
    // Blend state comes from the shader's opt_alpha (baked into the pipeline)
}

// Lazily creates the pipeline for the current depth/cull/framebuffer state.
VkPipeline GfxRenderingAPIVK::GetPipelineForState(ShaderProgramVK* prg, FramebufferVK& fb) {
    // Derive the depth compare op the same way Metal does
    uint32_t compareSel; // 0 = ALWAYS, 1 = LESS, 2 = LEQUAL, 3 = EQUAL
    if (!mCurrentDepthTest) {
        compareSel = 0;
    } else if (mCurrentZmodeDecal != 0) {
        compareSel = mCurrentStrictDecal ? 3 : 2;
    } else {
        compareSel = 1;
    }
    uint32_t cullSel = mCurrentCullKeepSign > 0 ? 1 : (mCurrentCullKeepSign < 0 ? 2 : 0);
    bool depthBias = mCurrentZmodeDecal && !mCurrentStrictDecal;

    uint32_t key = fb.mMsaaLevel | ((fb.mHasDepthBuffer ? 1u : 0) << 4) | ((mCurrentDepthMask ? 1u : 0) << 5) |
                   (compareSel << 6) | (cullSel << 8) | ((depthBias ? 1u : 0) << 10);

    auto it = prg->pipelines.find(key);
    if (it != prg->pipelines.end()) {
        return it->second;
    }

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = prg->vs;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = prg->fs;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding = { 0, prg->numFloats * (uint32_t)sizeof(float),
                                                VK_VERTEX_INPUT_RATE_VERTEX };
    VkPipelineVertexInputStateCreateInfo vertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = prg->attribCount;
    vertexInput.pVertexAttributeDescriptions = prg->attribs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
    };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode =
        cullSel == 1 ? VK_CULL_MODE_FRONT_BIT : (cullSel == 2 ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE);
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    raster.depthBiasEnable = depthBias ? VK_TRUE : VK_FALSE;
    raster.depthClampEnable = mHasDepthClamp ? VK_TRUE : VK_FALSE;

    VkPipelineMultisampleStateCreateInfo msaa = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    msaa.rasterizationSamples = (VkSampleCountFlagBits)fb.mMsaaLevel;

    VkPipelineDepthStencilStateCreateInfo depth = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depth.depthTestEnable = fb.mHasDepthBuffer && (mCurrentDepthTest || mCurrentDepthMask) ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = fb.mHasDepthBuffer && mCurrentDepthMask ? VK_TRUE : VK_FALSE;
    static const VkCompareOp compareOps[4] = { VK_COMPARE_OP_ALWAYS, VK_COMPARE_OP_LESS, VK_COMPARE_OP_LESS_OR_EQUAL,
                                               VK_COMPARE_OP_EQUAL };
    depth.depthCompareOp = compareOps[compareSel];

    VkPipelineColorBlendAttachmentState blendAttachment = {};
    blendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if (prg->useAlphaBlend) {
        blendAttachment.blendEnable = VK_TRUE;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo blend = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttachment;

    static const VkDynamicState dynamicStates[3] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
                                                     VK_DYNAMIC_STATE_DEPTH_BIAS };
    VkPipelineDynamicStateCreateInfo dynamic = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamic.dynamicStateCount = 3;
    dynamic.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pci = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pci.stageCount = 2;
    pci.pStages = stages;
    pci.pVertexInputState = &vertexInput;
    pci.pInputAssemblyState = &inputAssembly;
    pci.pViewportState = &viewportState;
    pci.pRasterizationState = &raster;
    pci.pMultisampleState = &msaa;
    pci.pDepthStencilState = &depth;
    pci.pColorBlendState = &blend;
    pci.pDynamicState = &dynamic;
    pci.layout = mPipelineLayout;
    pci.renderPass = GetRenderPass(fb.mMsaaLevel, fb.mHasDepthBuffer);
    pci.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult res = vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline);
    if (res != VK_SUCCESS) {
        SPDLOG_ERROR("Vulkan: pipeline creation failed ({}) for shader {:x}/{:x}", (int)res, prg->shader_id0,
                     prg->shader_id1);
    }
    prg->pipelines[key] = pipeline;
    return pipeline;
}

void GfxRenderingAPIVK::WriteDrawUniformsIfDirty() {
    FrameSlot& slot = mFrames[mFrameSlot];
    const uint32_t align = std::max<uint32_t>(256, (uint32_t)mDeviceProps.limits.minUniformBufferOffsetAlignment);

    auto pushBlock = [&](const void* data, size_t size) -> uint32_t {
        size_t offset = (mUniformRingOffset + align - 1) & ~((size_t)align - 1);
        if (offset + size > mUniformRingCapacity) {
            SPDLOG_ERROR("Vulkan: uniform ring overflow");
            return 0;
        }
        memcpy(slot.uniformMapped + offset, data, size);
        mUniformRingOffset = offset + size;
        return (uint32_t)offset;
    };

    if (mFrameUniformsDirty || mFrameUniformOffset == UINT32_MAX) {
        mFrameUniformOffset = pushBlock(&mFrameUniforms, sizeof(mFrameUniforms));
        mFrameUniformsDirty = false;
    }

    bool texturesChanged = false;
    if (mCurrentFilterMode == FILTER_THREE_POINT) {
        for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
            int32_t f = (int32_t)mTextures[mCurrentTextureIds[i]].filtering;
            if (mDrawUniforms.textureFiltering[i] != f) {
                mDrawUniforms.textureFiltering[i] = f;
                texturesChanged = true;
            }
        }
    }

    if (texturesChanged || mPrimDepthDirty || mLodMaxDirty || mCombinerUniformsDirty || mCustomUniformsDirty ||
        mDrawUniformOffset == UINT32_MAX) {
        mDrawUniforms.misc[0] = mCurrentPrimDepth;
        mDrawUniforms.misc[1] = mCurrentMaxLod;
        memcpy(mDrawUniforms.inputs, mCombinerUniforms.inputs, sizeof(mDrawUniforms.inputs));
        memcpy(mDrawUniforms.fog_color, mCombinerUniforms.fog_color, sizeof(mDrawUniforms.fog_color));
        memcpy(mDrawUniforms.grayscale_color, mCombinerUniforms.grayscale_color, sizeof(mDrawUniforms.grayscale_color));
        memcpy(mDrawUniforms.uv_transform, mCombinerUniforms.uv_transform, sizeof(mDrawUniforms.uv_transform));
        memcpy(mDrawUniforms.texture_clamp, mCombinerUniforms.texture_clamp, sizeof(mDrawUniforms.texture_clamp));
        memcpy(mDrawUniforms.fog_params, mCombinerUniforms.fog_params, sizeof(mDrawUniforms.fog_params));
        memcpy(mDrawUniforms.palette_params, mCombinerUniforms.palette_params, sizeof(mDrawUniforms.palette_params));
        memcpy(mDrawUniforms.lod_params, mCombinerUniforms.lod_params, sizeof(mDrawUniforms.lod_params));
        memcpy(mDrawUniforms.debug_tint, mCombinerUniforms.debug_tint, sizeof(mDrawUniforms.debug_tint));
        memcpy(mDrawUniforms.custom, mCustomUniforms.regs, sizeof(mDrawUniforms.custom));
        mDrawUniformOffset = pushBlock(&mDrawUniforms, sizeof(mDrawUniforms));
        mPrimDepthDirty = false;
        mLodMaxDirty = false;
        mCombinerUniformsDirty = false;
        mCustomUniformsDirty = false;
    }

    if (mLightingUniformsDirty || mLightingUniformOffset == UINT32_MAX) {
        mLightingUniformOffset = pushBlock(&mLightingUniforms, sizeof(mLightingUniforms));
        mLightingUniformsDirty = false;
    }

    if (mTransformUniformsDirty || mTransformUniformOffset == UINT32_MAX) {
        mTransformUniformOffset = pushBlock(&mTransformUniforms, sizeof(mTransformUniforms));
        mTransformUniformsDirty = false;
    }
}

void GfxRenderingAPIVK::DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    if (mShaderProgram == nullptr || !mFrameActive) {
        return;
    }
    EnsurePassActive(mCurrentFramebuffer);
    FramebufferVK& fb = mFramebuffers[mCurrentFramebuffer];
    FrameSlot& slot = mFrames[mFrameSlot];
    VkCommandBuffer cmd = fb.mCommandBuffer;

    // Pipeline (covers shader + depth + cull + blend + msaa state)
    VkPipeline pipeline = GetPipelineForState(mShaderProgram, fb);
    if (pipeline == VK_NULL_HANDLE) {
        return;
    }
    if (pipeline != fb.mLastPipeline) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        fb.mLastPipeline = pipeline;
        fb.mLastShaderProgram = mShaderProgram;
    }

    // Decal depth bias (dynamic state, mirrors the Metal SSDB computation)
    float bias = 0.0f;
    if (mCurrentZmodeDecal && !mCurrentStrictDecal) {
        const int n64modeFactor = 120;
        const int noVanishFactor = 100;
        switch (Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_Z_FIGHTING_MODE, 0)) {
            case 1:
                bias = -1.0f * (float)mRenderTargetHeight / n64modeFactor;
                break;
            case 2:
                bias = -1.0f * (float)mRenderTargetHeight / noVanishFactor;
                break;
            default:
                bias = -2.0f;
        }
        vkCmdSetDepthBias(cmd, 0.0f, 0.0f, bias);
    } else if (fb.mLastDepthBias != 0.0f) {
        vkCmdSetDepthBias(cmd, 0.0f, 0.0f, 0.0f);
    }
    fb.mLastDepthBias = bias;

    // Uniforms (ring + dynamic offsets on set 0)
    WriteDrawUniformsIfDirty();
    uint32_t offsets[4] = { mFrameUniformOffset, mDrawUniformOffset, mLightingUniformOffset, mTransformUniformOffset };
    if (memcmp(offsets, fb.mLastSet0Offsets, sizeof(offsets)) != 0) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &slot.set0, 4, offsets);
        memcpy(fb.mLastSet0Offsets, offsets, sizeof(offsets));
    }

    // Texture descriptor set (set 1): rebuilt when any binding changes
    VkImageView views[SHADER_MAX_TEXTURES];
    VkSampler samplers[SHADER_MAX_TEXTURES];
    bool texChanged = false;
    for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
        const TextureDataVK& tex = mTextures[mCurrentTextureIds[i]];
        bool used = mShaderProgram->usedTextures[i];
        views[i] = used && tex.view != VK_NULL_HANDLE ? tex.view : mDummyTexture.view;
        samplers[i] = used && tex.sampler != VK_NULL_HANDLE ? tex.sampler : mDummyTexture.sampler;
        if (views[i] != fb.mLastBoundViews[i] || samplers[i] != fb.mLastBoundSamplers[i]) {
            texChanged = true;
        }
    }
    if (texChanged) {
        VkDescriptorSetAllocateInfo dsa = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        dsa.descriptorPool = slot.descriptorPool;
        dsa.descriptorSetCount = 1;
        dsa.pSetLayouts = &mSet1Layout;
        VkDescriptorSet set1 = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(mDevice, &dsa, &set1) == VK_SUCCESS) {
            VkDescriptorImageInfo imageInfos[SHADER_MAX_TEXTURES];
            VkWriteDescriptorSet writes[SHADER_MAX_TEXTURES] = {};
            for (uint32_t i = 0; i < SHADER_MAX_TEXTURES; i++) {
                imageInfos[i] = { samplers[i], views[i], VK_IMAGE_LAYOUT_GENERAL };
                writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[i].dstSet = set1;
                writes[i].dstBinding = i;
                writes[i].descriptorCount = 1;
                writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writes[i].pImageInfo = &imageInfos[i];
            }
            vkUpdateDescriptorSets(mDevice, SHADER_MAX_TEXTURES, writes, 0, nullptr);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 1, 1, &set1, 0, nullptr);
            memcpy(fb.mLastBoundViews, views, sizeof(views));
            memcpy(fb.mLastBoundSamplers, samplers, sizeof(samplers));
        }
    }

    // Vertex data
    size_t dataSize = sizeof(float) * buf_vbo_len;
    if (mVertexRingOffset + dataSize > mVertexRingCapacity) {
        SPDLOG_ERROR("Vulkan: vertex ring overflow, dropping draw");
        return;
    }
    memcpy(slot.vertexMapped + mVertexRingOffset, buf_vbo, dataSize);
    VkDeviceSize vbOffset = mVertexRingOffset;
    vkCmdBindVertexBuffers(cmd, 0, 1, &slot.vertexBuffer, &vbOffset);
    mVertexRingOffset += dataSize;

    vkCmdDraw(cmd, (uint32_t)(buf_vbo_num_tris * 3), 1, 0, 0);
}

void GfxRenderingAPIVK::OnResize() {
    mSwapchainNeedsRecreate = true;
}

void GfxRenderingAPIVK::StartFrame() {
    mFrameUniforms.frameCount++;
    if (mFrameUniforms.frameCount > 150) {
        mFrameUniforms.frameCount = 0;
    }
    mFrameUniformsDirty = true;
}

void GfxRenderingAPIVK::EnsurePassActive(int fbId) {
    FramebufferVK& fb = mFramebuffers[fbId];
    if (fb.mPassActive) {
        return;
    }

    if (fb.mCommandBuffer == VK_NULL_HANDLE) {
        VkCommandBufferAllocateInfo ai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        ai.commandPool = mFrames[mFrameSlot].commandPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(mDevice, &ai, &fb.mCommandBuffer));
        VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(fb.mCommandBuffer, &bi));

        if (fbId == 0 && !mSwapchainImageUsed[mSwapchainImageIndex]) {
            // First use of this swapchain image: bring it into GENERAL
            TransitionToGeneral(fb.mCommandBuffer, mSwapchainImages[mSwapchainImageIndex], VK_IMAGE_ASPECT_COLOR_BIT,
                                1);
            mSwapchainImageUsed[mSwapchainImageIndex] = true;
        } else if (fbId == 0) {
            // Coming back from PRESENT_SRC
            VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = mSwapchainImages[mSwapchainImageIndex];
            barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            vkCmdPipelineBarrier(fb.mCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }
    }

    if (fb.mFramebuffer == VK_NULL_HANDLE || fb.mRenderPass == VK_NULL_HANDLE) {
        return; // not a render target
    }

    VkRenderPassBeginInfo rpbi = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rpbi.renderPass = fb.mRenderPass;
    rpbi.framebuffer = fb.mFramebuffer;
    rpbi.renderArea = { { 0, 0 }, { fb.mWidth, fb.mHeight } };
    vkCmdBeginRenderPass(fb.mCommandBuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    fb.mPassActive = true;

    // Dynamic state must be set before the first draw of the pass
    if (fb.mViewport.width == 0.0f && fb.mViewport.height == 0.0f) {
        fb.mViewport = { 0.0f, (float)fb.mHeight, (float)fb.mWidth, -(float)fb.mHeight, 0.0f, 1.0f };
    }
    if (fb.mScissor.extent.width == 0 && fb.mScissor.extent.height == 0) {
        fb.mScissor = { { 0, 0 }, { fb.mWidth, fb.mHeight } };
    }
    vkCmdSetViewport(fb.mCommandBuffer, 0, 1, &fb.mViewport);
    vkCmdSetScissor(fb.mCommandBuffer, 0, 1, &fb.mScissor);
    vkCmdSetDepthBias(fb.mCommandBuffer, 0.0f, 0.0f, fb.mLastDepthBias);

    // Pipeline/descriptor state does not survive across render passes
    fb.mLastPipeline = VK_NULL_HANDLE;
    fb.mLastShaderProgram = nullptr;
    for (uint32_t i = 0; i < 4; i++) {
        fb.mLastSet0Offsets[i] = UINT32_MAX;
    }
    for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
        fb.mLastBoundViews[i] = nullptr;
        fb.mLastBoundSamplers[i] = nullptr;
    }
}

void GfxRenderingAPIVK::EndPass(FramebufferVK& fb) {
    if (fb.mPassActive) {
        vkCmdEndRenderPass(fb.mCommandBuffer);
        fb.mPassActive = false;
    }
}

void GfxRenderingAPIVK::RestartPass(FramebufferVK& fb) {
    int fbId = (int)(&fb - mFramebuffers.data());
    EnsurePassActive(fbId);
}

void GfxRenderingAPIVK::StartDrawToFramebuffer(int fbId, float noiseScale) {
    if (fbId >= (int)mFramebuffers.size()) {
        return;
    }
    FramebufferVK& fb = mFramebuffers[fbId];
    mRenderTargetHeight = (int32_t)mTextures[fb.mTextureId].height;
    mCurrentFramebuffer = fbId;
    if (std::find(mDrawnFramebuffers.begin(), mDrawnFramebuffers.end(), fbId) == mDrawnFramebuffers.end()) {
        mDrawnFramebuffers.push_back(fbId);
    }

    if (noiseScale != 0.0f) {
        float inv = 1.0f / noiseScale;
        if (mFrameUniforms.noiseScale != inv) {
            mFrameUniforms.noiseScale = inv;
            mFrameUniformsDirty = true;
        }
    }

    if (mFrameActive && fb.mRenderTarget) {
        EnsurePassActive(fbId);
    }
}

void GfxRenderingAPIVK::ClearFramebuffer(bool color, bool depth) {
    if (!color && !depth) {
        return;
    }
    FramebufferVK& fb = mFramebuffers[mCurrentFramebuffer];
    if (!fb.mPassActive) {
        return;
    }

    VkClearAttachment clears[2];
    uint32_t count = 0;
    if (color) {
        clears[count].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clears[count].colorAttachment = 0;
        clears[count].clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        count++;
    }
    if (depth && fb.mHasDepthBuffer) {
        clears[count].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        clears[count].colorAttachment = 0;
        clears[count].clearValue.depthStencil = { 1.0f, 0 };
        count++;
    }
    if (count == 0) {
        return;
    }
    VkClearRect rect = { { { 0, 0 }, { fb.mWidth, fb.mHeight } }, 0, 1 };
    vkCmdClearAttachments(fb.mCommandBuffer, count, clears, 1, &rect);
}

void GfxRenderingAPIVK::ClearDepthRegion(int x, int y, int w, int h) {
    FramebufferVK& fb = mFramebuffers[mCurrentFramebuffer];
    if (!fb.mPassActive || !fb.mHasDepthBuffer) {
        return;
    }
    int32_t sy = std::max(0, (int)mRenderTargetHeight - y - h);
    VkClearAttachment clear = {};
    clear.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    clear.clearValue.depthStencil = { 1.0f, 0 };
    VkClearRect rect = { { { std::max(0, x), sy },
                           { (uint32_t)std::max(0, std::min(w, (int)fb.mWidth - x)),
                             (uint32_t)std::max(0, std::min(h, (int)fb.mHeight - sy)) } },
                         0,
                         1 };
    if (rect.rect.extent.width == 0 || rect.rect.extent.height == 0) {
        return;
    }
    vkCmdClearAttachments(fb.mCommandBuffer, 1, &clear, 1, &rect);
}

int GfxRenderingAPIVK::CreateFramebuffer() {
    uint32_t textureId = NewTexture();
    mTextures[textureId].sampler = GetSampler(true, G_TX_WRAP, G_TX_WRAP);

    size_t index = mFramebuffers.size();
    mFramebuffers.resize(mFramebuffers.size() + 1);
    FramebufferVK& fb = mFramebuffers.back();
    fb.mTextureId = textureId;
    fb.mMsaaLevel = 1;
    return (int)index;
}

void GfxRenderingAPIVK::UpdateFramebufferParameters(int fbId, uint32_t width, uint32_t height, uint32_t msaaLevel,
                                                    bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                                    bool can_extract_depth) {
    if (fbId == 0) {
        // The swapchain tracks the drawable size in NewFrame
        return;
    }
    if (fbId >= (int)mFramebuffers.size()) {
        return;
    }

    FramebufferVK& fb = mFramebuffers[fbId];
    TextureDataVK& tex = mTextures[fb.mTextureId];

    width = std::max(width, 1u);
    height = std::max(height, 1u);
    msaaLevel = std::max(msaaLevel, 1u);
    while (msaaLevel > 1 && (mDeviceProps.limits.framebufferColorSampleCounts & msaaLevel) == 0) {
        msaaLevel--;
    }

    bool diff = fb.mWidth != width || fb.mHeight != height || fb.mMsaaLevel != msaaLevel ||
                fb.mHasDepthBuffer != has_depth_buffer || fb.mRenderTarget != render_target;
    if (!diff) {
        return;
    }

    vkDeviceWaitIdle(mDevice);

    // Color texture (sampled + render target + blit source/destination)
    DestroyTextureData(tex, false);
    CreateImage(width, height, 1, VK_SAMPLE_COUNT_1_BIT, mSwapchainFormat,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                tex.image, tex.memory);
    tex.view = CreateImageView(tex.image, mSwapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    tex.width = width;
    tex.height = height;
    tex.mip_levels = 1;

    VkCommandBuffer cmd = BeginOneShot();
    TransitionToGeneral(cmd, tex.image, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    auto destroyTarget = [&](VkImage& img, VkDeviceMemory& mem, VkImageView& view) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(mDevice, view, nullptr);
            vkDestroyImage(mDevice, img, nullptr);
            vkFreeMemory(mDevice, mem, nullptr);
            view = VK_NULL_HANDLE;
            img = VK_NULL_HANDLE;
            mem = VK_NULL_HANDLE;
        }
    };
    destroyTarget(fb.mDepthImage, fb.mDepthMemory, fb.mDepthView);
    destroyTarget(fb.mMsaaImage, fb.mMsaaMemory, fb.mMsaaView);
    destroyTarget(fb.mMsaaDepthImage, fb.mMsaaDepthMemory, fb.mMsaaDepthView);

    if (has_depth_buffer) {
        CreateImage(width, height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_D32_SFLOAT,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, fb.mDepthImage,
                    fb.mDepthMemory);
        fb.mDepthView = CreateImageView(fb.mDepthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        TransitionToGeneral(cmd, fb.mDepthImage, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    if (msaaLevel > 1) {
        CreateImage(width, height, 1, (VkSampleCountFlagBits)msaaLevel, mSwapchainFormat,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, fb.mMsaaImage,
                    fb.mMsaaMemory);
        fb.mMsaaView = CreateImageView(fb.mMsaaImage, mSwapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        TransitionToGeneral(cmd, fb.mMsaaImage, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        if (has_depth_buffer) {
            CreateImage(width, height, 1, (VkSampleCountFlagBits)msaaLevel, VK_FORMAT_D32_SFLOAT,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, fb.mMsaaDepthImage, fb.mMsaaDepthMemory);
            fb.mMsaaDepthView = CreateImageView(fb.mMsaaDepthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
            TransitionToGeneral(cmd, fb.mMsaaDepthImage, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        }
    }
    EndOneShot(cmd);

    fb.mWidth = width;
    fb.mHeight = height;
    fb.mMsaaLevel = msaaLevel;
    fb.mHasDepthBuffer = has_depth_buffer;
    fb.mRenderTarget = render_target;

    if (fb.mFramebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(mDevice, fb.mFramebuffer, nullptr);
        fb.mFramebuffer = VK_NULL_HANDLE;
    }

    if (render_target) {
        fb.mRenderPass = GetRenderPass(msaaLevel, has_depth_buffer);
        std::vector<VkImageView> attachments;
        if (msaaLevel > 1) {
            attachments.push_back(fb.mMsaaView);
            attachments.push_back(tex.view);
            if (has_depth_buffer) {
                attachments.push_back(fb.mMsaaDepthView);
            }
        } else {
            attachments.push_back(tex.view);
            if (has_depth_buffer) {
                attachments.push_back(fb.mDepthView);
            }
        }
        VkFramebufferCreateInfo fci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fci.renderPass = fb.mRenderPass;
        fci.attachmentCount = (uint32_t)attachments.size();
        fci.pAttachments = attachments.data();
        fci.width = width;
        fci.height = height;
        fci.layers = 1;
        VK_CHECK(vkCreateFramebuffer(mDevice, &fci, nullptr, &fb.mFramebuffer));
    } else {
        fb.mRenderPass = VK_NULL_HANDLE;
    }
}

void GfxRenderingAPIVK::CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0,
                                        int dstY0, int dstX1, int dstY1) {
    if (fbSrcId >= (int)mFramebuffers.size() || fbDstId >= (int)mFramebuffers.size() || !mFrameActive) {
        return;
    }
    FramebufferVK& src = mFramebuffers[fbSrcId];
    TextureDataVK& srcTex = mTextures[src.mTextureId];
    TextureDataVK& dstTex = mTextures[mFramebuffers[fbDstId].mTextureId];
    if (srcTex.image == VK_NULL_HANDLE || dstTex.image == VK_NULL_HANDLE || src.mCommandBuffer == VK_NULL_HANDLE) {
        return;
    }

    bool wasActive = src.mPassActive;
    EndPass(src);
    FullBarrier(src.mCommandBuffer);

    VkImageBlit blit = {};
    blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    blit.srcOffsets[0] = { srcX0, srcY0, 0 };
    blit.srcOffsets[1] = { srcX1, srcY1, 1 };
    blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    blit.dstOffsets[0] = { dstX0, dstY0, 0 };
    blit.dstOffsets[1] = { dstX1, dstY1, 1 };
    vkCmdBlitImage(src.mCommandBuffer, srcTex.image, VK_IMAGE_LAYOUT_GENERAL, dstTex.image, VK_IMAGE_LAYOUT_GENERAL, 1,
                   &blit, VK_FILTER_NEAREST);
    FullBarrier(src.mCommandBuffer);

    if (wasActive) {
        RestartPass(src);
    }
}

void GfxRenderingAPIVK::ResolveMSAAColorBuffer(int fbIdTarget, int fbIdSrc) {
    if (fbIdSrc >= (int)mFramebuffers.size() || fbIdTarget >= (int)mFramebuffers.size() || !mFrameActive) {
        return;
    }
    FramebufferVK& src = mFramebuffers[fbIdSrc];
    TextureDataVK& srcTex = mTextures[src.mTextureId];
    TextureDataVK& dstTex = mTextures[mFramebuffers[fbIdTarget].mTextureId];

    if (srcTex.width != dstTex.width || srcTex.height != dstTex.height) {
        return;
    }

    // Ending the src render pass executes its MSAA resolve into the sampled
    // texture; then copy that into the target.
    if (fbIdTarget != 0) {
        if (src.mCommandBuffer == VK_NULL_HANDLE) {
            return;
        }
        EndPass(src);
        src.mHasEnded = true; // mirrors Metal: src no longer recorded into
        FullBarrier(src.mCommandBuffer);
        VkImageCopy copy = {};
        copy.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copy.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copy.extent = { srcTex.width, srcTex.height, 1 };
        vkCmdCopyImage(src.mCommandBuffer, srcTex.image, VK_IMAGE_LAYOUT_GENERAL, dstTex.image, VK_IMAGE_LAYOUT_GENERAL,
                       1, &copy);
        FullBarrier(src.mCommandBuffer);
    } else {
        FramebufferVK& dst = mFramebuffers[0];
        EnsurePassActive(0);
        EndPass(dst);
        FullBarrier(dst.mCommandBuffer);
        VkImageCopy copy = {};
        copy.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copy.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copy.extent = { srcTex.width, srcTex.height, 1 };
        vkCmdCopyImage(dst.mCommandBuffer, srcTex.image, VK_IMAGE_LAYOUT_GENERAL, dstTex.image, VK_IMAGE_LAYOUT_GENERAL,
                       1, &copy);
        FullBarrier(dst.mCommandBuffer);
        RestartPass(dst);
    }
}

// Submits the framebuffer's command buffer (preceded by pending uploads) and
// waits for completion. Used by the synchronous readback paths.
void GfxRenderingAPIVK::SubmitFramebufferAndWait(int fbId) {
    FramebufferVK& fb = mFramebuffers[fbId];
    EndPass(fb);
    VK_CHECK(vkEndCommandBuffer(fb.mCommandBuffer));

    std::vector<VkCommandBuffer> cmds;
    if (mUploadCmd != VK_NULL_HANDLE) {
        VK_CHECK(vkEndCommandBuffer(mUploadCmd));
        cmds.push_back(mUploadCmd);
        mUploadCmd = VK_NULL_HANDLE;
        mStagingOffset = 0;
    }
    cmds.push_back(fb.mCommandBuffer);

    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = (uint32_t)cmds.size();
    si.pCommandBuffers = cmds.data();
    VK_CHECK(vkQueueSubmit(mQueue, 1, &si, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(mQueue));

    fb.mCommandBuffer = VK_NULL_HANDLE;
    fb.mPassActive = false;
    fb.mHasEnded = false;
    auto it = std::find(mDrawnFramebuffers.begin(), mDrawnFramebuffers.end(), fbId);
    if (it != mDrawnFramebuffers.end()) {
        mDrawnFramebuffers.erase(it);
    }
}

void GfxRenderingAPIVK::ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) {
    if (fbId >= (int)mFramebuffers.size() || !mFrameActive) {
        return;
    }
    FramebufferVK& fb = mFramebuffers[fbId];
    TextureDataVK& tex = mTextures[fb.mTextureId];
    const bool bgr = mSwapchainFormat == VK_FORMAT_B8G8R8A8_UNORM;

    auto convert = [&](const uint8_t* px, uint32_t count) {
        for (uint32_t i = 0; i < count; i++) {
            uint8_t c0 = px[i * 4 + 0];
            uint8_t g = px[i * 4 + 1];
            uint8_t c2 = px[i * 4 + 2];
            uint8_t a = px[i * 4 + 3];
            uint8_t r = bgr ? c2 : c0;
            uint8_t b = bgr ? c0 : c2;
            rgba16Buf[i] = ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | (a >> 7);
        }
    };

    size_t needed = (size_t)width * height * 4;
    if (mScreenReadbackCapacity < needed) {
        if (mScreenReadbackBuffer != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(mDevice);
            vkDestroyBuffer(mDevice, mScreenReadbackBuffer, nullptr);
            vkFreeMemory(mDevice, mScreenReadbackMemory, nullptr);
        }
        CreateBuffer(needed, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mScreenReadbackBuffer,
                     mScreenReadbackMemory, &mScreenReadbackMapped);
        mScreenReadbackCapacity = needed;
    }

    if (fbId == 0) {
        // Deferred path (mirrors Metal): serve last frame's pixels, request a
        // copy at the end of this frame.
        if (mScreenReadbackDataReady) {
            convert(mScreenReadbackMapped, mScreenReadbackWidth * mScreenReadbackHeight);
            mScreenReadbackDataReady = false;
        }
        mScreenReadbackRequested = true;
        mScreenReadbackWidth = width;
        mScreenReadbackHeight = height;
        return;
    }

    // Offscreen framebuffer (e.g. Paper Mario's shading palette): synchronous
    if (fb.mCommandBuffer == VK_NULL_HANDLE || tex.image == VK_NULL_HANDLE) {
        memset(rgba16Buf, 0, sizeof(uint16_t) * width * height);
        return;
    }

    EndPass(fb);
    FullBarrier(fb.mCommandBuffer);
    VkBufferImageCopy copy = {};
    copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copy.imageExtent = { std::min(width, tex.width), std::min(height, tex.height), 1 };
    vkCmdCopyImageToBuffer(fb.mCommandBuffer, tex.image, VK_IMAGE_LAYOUT_GENERAL, mScreenReadbackBuffer, 1, &copy);
    FullBarrier(fb.mCommandBuffer);
    SubmitFramebufferAndWait(fbId);

    convert(mScreenReadbackMapped, width * height);
}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
GfxRenderingAPIVK::GetPixelDepth(int fbId, const std::set<std::pair<float, float>>& coordinates) {
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> res;
    if (fbId >= (int)mFramebuffers.size() || coordinates.empty() || !mFrameActive) {
        return res;
    }
    FramebufferVK& fb = mFramebuffers[fbId];
    if (fb.mDepthImage == VK_NULL_HANDLE || fb.mCommandBuffer == VK_NULL_HANDLE) {
        for (const auto& coord : coordinates) {
            res.emplace(coord, 0);
        }
        return res;
    }

    size_t needed = coordinates.size() * sizeof(float);
    if (mScreenReadbackCapacity < needed) {
        if (mScreenReadbackBuffer != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(mDevice);
            vkDestroyBuffer(mDevice, mScreenReadbackBuffer, nullptr);
            vkFreeMemory(mDevice, mScreenReadbackMemory, nullptr);
        }
        CreateBuffer(needed, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, mScreenReadbackBuffer,
                     mScreenReadbackMemory, &mScreenReadbackMapped);
        mScreenReadbackCapacity = needed;
    }

    bool wasActive = fb.mPassActive;
    EndPass(fb);
    FullBarrier(fb.mCommandBuffer);

    std::vector<VkBufferImageCopy> regions;
    regions.reserve(coordinates.size());
    size_t i = 0;
    for (const auto& coord : coordinates) {
        VkBufferImageCopy copy = {};
        copy.bufferOffset = i * sizeof(float);
        copy.imageSubresource = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1 };
        int32_t cx = std::clamp((int32_t)coord.first, 0, (int32_t)fb.mWidth - 1);
        int32_t cy = std::clamp((int32_t)(fb.mHeight - 1 - coord.second), 0, (int32_t)fb.mHeight - 1);
        copy.imageOffset = { cx, cy, 0 };
        copy.imageExtent = { 1, 1, 1 };
        regions.push_back(copy);
        i++;
    }
    vkCmdCopyImageToBuffer(fb.mCommandBuffer, fb.mDepthImage, VK_IMAGE_LAYOUT_GENERAL, mScreenReadbackBuffer,
                           (uint32_t)regions.size(), regions.data());
    FullBarrier(fb.mCommandBuffer);
    SubmitFramebufferAndWait(fbId);

    const float* depthValues = (const float*)mScreenReadbackMapped;
    i = 0;
    for (const auto& coord : coordinates) {
        res.emplace(coord, (uint16_t)(depthValues[i++] * 65532.0f));
    }

    if (wasActive) {
        // Caller may keep drawing to this framebuffer afterwards
        StartDrawToFramebuffer(fbId, 0.0f);
    }
    return res;
}

void GfxRenderingAPIVK::EndFrame() {
    if (!mFrameActive) {
        return;
    }
    FrameSlot& slot = mFrames[mFrameSlot];

    std::vector<VkCommandBuffer> cmds;
    if (mUploadCmd != VK_NULL_HANDLE) {
        VK_CHECK(vkEndCommandBuffer(mUploadCmd));
        cmds.push_back(mUploadCmd);
        mUploadCmd = VK_NULL_HANDLE;
    }

    std::sort(mDrawnFramebuffers.begin(), mDrawnFramebuffers.end());
    for (int fbId : mDrawnFramebuffers) {
        if (fbId == 0) {
            continue;
        }
        FramebufferVK& fb = mFramebuffers[fbId];
        if (fb.mCommandBuffer == VK_NULL_HANDLE) {
            continue;
        }
        EndPass(fb);
        VK_CHECK(vkEndCommandBuffer(fb.mCommandBuffer));
        cmds.push_back(fb.mCommandBuffer);
    }

    // Screen framebuffer last
    EnsurePassActive(0);
    FramebufferVK& screen = mFramebuffers[0];
    EndPass(screen);

    if (mScreenReadbackRequested && mScreenReadbackBuffer != VK_NULL_HANDLE) {
        FullBarrier(screen.mCommandBuffer);
        VkBufferImageCopy copy = {};
        copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copy.imageExtent = { std::min(mScreenReadbackWidth, mSwapchainExtent.width),
                             std::min(mScreenReadbackHeight, mSwapchainExtent.height), 1 };
        vkCmdCopyImageToBuffer(screen.mCommandBuffer, mSwapchainImages[mSwapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL,
                               mScreenReadbackBuffer, 1, &copy);
        mScreenReadbackRequested = false;
        mScreenReadbackDataReady = true;
    }

    // GENERAL -> PRESENT_SRC for the swapchain image
    {
        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = mSwapchainImages[mSwapchainImageIndex];
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask = 0;
        vkCmdPipelineBarrier(screen.mCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
    VK_CHECK(vkEndCommandBuffer(screen.mCommandBuffer));
    cmds.push_back(screen.mCommandBuffer);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &slot.imageAvailable;
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = (uint32_t)cmds.size();
    si.pCommandBuffers = cmds.data();
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &slot.renderFinished;
    VK_CHECK(vkQueueSubmit(mQueue, 1, &si, slot.fence));
    slot.fenceInFlight = true;

    VkPresentInfoKHR pi = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &slot.renderFinished;
    pi.swapchainCount = 1;
    pi.pSwapchains = &mSwapchain;
    pi.pImageIndices = &mSwapchainImageIndex;
    VkResult present = vkQueuePresentKHR(mQueue, &pi);
    if (present == VK_ERROR_OUT_OF_DATE_KHR || present == VK_SUBOPTIMAL_KHR) {
        mSwapchainNeedsRecreate = true;
    } else if (present != VK_SUCCESS) {
        SPDLOG_ERROR("Vulkan: vkQueuePresentKHR failed ({})", (int)present);
    }

    // Reset per-framebuffer recording state
    for (auto& fb : mFramebuffers) {
        fb.mCommandBuffer = VK_NULL_HANDLE;
        fb.mPassActive = false;
        fb.mHasEnded = false;
        fb.mLastPipeline = VK_NULL_HANDLE;
        fb.mLastShaderProgram = nullptr;
        for (uint32_t i = 0; i < 4; i++) {
            fb.mLastSet0Offsets[i] = UINT32_MAX;
        }
        for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
            fb.mLastBoundViews[i] = nullptr;
            fb.mLastBoundSamplers[i] = nullptr;
        }
        memset(&fb.mViewport, 0, sizeof(fb.mViewport));
        memset(&fb.mScissor, 0, sizeof(fb.mScissor));
        fb.mLastDepthTest = -1;
        fb.mLastDepthMask = -1;
        fb.mLastZmodeDecal = -1;
        fb.mLastStrictDecal = -1;
        fb.mLastDepthBias = 0.0f;
    }
    mDrawnFramebuffers.clear();
    mFrameActive = false;
    mFrameSlot = (mFrameSlot + 1) % VK_FRAMES_IN_FLIGHT;
}

void GfxRenderingAPIVK::FinishRender() {
}

void* GfxRenderingAPIVK::GetFramebufferTextureId(int fbId) {
    return GetTextureById(mFramebuffers[fbId].mTextureId);
}

void GfxRenderingAPIVK::SelectTextureFb(int fbId) {
    SelectTexture(0, mFramebuffers[fbId].mTextureId);
}

ImTextureID GfxRenderingAPIVK::GetTextureById(int id) {
    TextureDataVK& tex = mTextures[id];
    if (tex.view == VK_NULL_HANDLE) {
        return (ImTextureID) nullptr;
    }
    if (tex.imguiSet == VK_NULL_HANDLE) {
        VkSampler sampler = tex.sampler != VK_NULL_HANDLE ? tex.sampler : mDummyTexture.sampler;
        tex.imguiSet = ImGui_ImplVulkan_AddTexture(sampler, tex.view, VK_IMAGE_LAYOUT_GENERAL);
    }
    return (ImTextureID)tex.imguiSet;
}

void GfxRenderingAPIVK::SetTextureFilter(FilteringMode mode) {
    mCurrentFilterMode = mode;
    gfx_texture_cache_clear();
}

FilteringMode GfxRenderingAPIVK::GetTextureFilter() {
    return mCurrentFilterMode;
}

} // namespace Fast

bool Vulkan_IsSupported() {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        return false;
    }
    if (SDL_Vulkan_LoadLibrary(nullptr) != 0) {
        return false;
    }
    return true;
}

#endif
