//
//  gfx_metal_shader.cpp
//  libultraship
//
//  Created by David Chavez on 16.08.22.
//

#ifdef __APPLE__

#include <Context.h>
#include <resource/factory/ShaderFactory.h>
// This is a workaround for conflicting defines on Metal.hpp
#define TRUE 1
#define FALSE 0
#include <Metal/Metal.hpp>
#undef TRUE
#undef FALSE
#include <public/bridge/consolevariablebridge.h>
#include "gfx_metal_shader.h"
#include <prism/processor.h>

// MARK: - String Helpers

#define RAND_NOISE                                                                                                  \
    "((random(float3(floor(in.position.xy * frameUniforms.noiseScale), float(frameUniforms.frameCount))) + 1.0) / " \
    "2.0)"

static const char* p_shader_item_to_str(uint32_t item, bool with_alpha, bool only_alpha, bool inputs_have_alpha,
                                        bool first_cycle, bool hint_single_element) {
    if (!only_alpha) {
        switch (item) {
            case SHADER_0:
                return with_alpha ? "float4(0.0, 0.0, 0.0, 0.0)" : "float3(0.0, 0.0, 0.0)";
            case SHADER_1:
                return with_alpha ? "float4(1.0, 1.0, 1.0, 1.0)" : "float3(1.0, 1.0, 1.0)";
            case SHADER_INPUT_1:
                return with_alpha || !inputs_have_alpha ? "in.input1" : "in.input1.xyz";
            case SHADER_INPUT_2:
                return with_alpha || !inputs_have_alpha ? "in.input2" : "in.input2.xyz";
            case SHADER_INPUT_3:
                return with_alpha || !inputs_have_alpha ? "in.input3" : "in.input3.xyz";
            case SHADER_INPUT_4:
                return with_alpha || !inputs_have_alpha ? "in.input4" : "in.input4.xyz";
            case SHADER_TEXEL0:
                return first_cycle ? (with_alpha ? "texVal0" : "texVal0.xyz")
                                   : (with_alpha ? "texVal1" : "texVal1.xyz");
            case SHADER_TEXEL0A:
                return first_cycle
                           ? (hint_single_element ? "texVal0.w"
                                                  : (with_alpha ? "float4(texVal0.w, texVal0.w, texVal0.w, texVal0.w)"
                                                                : "float3(texVal0.w, texVal0.w, texVal0.w)"))
                           : (hint_single_element ? "texVal1.w"
                                                  : (with_alpha ? "float4(texVal1.w, texVal1.w, texVal1.w, texVal1.w)"
                                                                : "float3(texVal1.w, texVal1.w, texVal1.w)"));
            case SHADER_TEXEL1A:
                return first_cycle
                           ? (hint_single_element ? "texVal1.w"
                                                  : (with_alpha ? "float4(texVal1.w, texVal1.w, texVal1.w, texVal1.w)"
                                                                : "float3(texVal1.w, texVal1.w, texVal1.w)"))
                           : (hint_single_element ? "texVal0.w"
                                                  : (with_alpha ? "float4(texVal0.w, texVal0.w, texVal0.w, texVal0.w)"
                                                                : "float3(texVal0.w, texVal0.w, texVal0.w)"));
            case SHADER_TEXEL1:
                return first_cycle ? (with_alpha ? "texVal1" : "texVal1.xyz")
                                   : (with_alpha ? "texVal0" : "texVal0.xyz");
            case SHADER_COMBINED:
                return with_alpha ? "texel" : "texel.xyz";
            case SHADER_NOISE:
                return with_alpha ? "float4(" RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ")"
                                  : "float3(" RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ")";
        }
    } else {
        switch (item) {
            case SHADER_0:
                return "0.0";
            case SHADER_1:
                return "1.0";
            case SHADER_INPUT_1:
                return "in.input1.w";
            case SHADER_INPUT_2:
                return "in.input2.w";
            case SHADER_INPUT_3:
                return "in.input3.w";
            case SHADER_INPUT_4:
                return "in.input4.w";
            case SHADER_TEXEL0:
                return first_cycle ? "texVal0.w" : "texVal1.w";
            case SHADER_TEXEL0A:
                return first_cycle ? "texVal0.w" : "texVal1.w";
            case SHADER_TEXEL1A:
                return first_cycle ? "texVal1.w" : "texVal0.w";
            case SHADER_TEXEL1:
                return first_cycle ? "texVal1.w" : "texVal0.w";
            case SHADER_COMBINED:
                return "texel.w";
            case SHADER_NOISE:
                return RAND_NOISE;
        }
    }
    return "";
}

bool p_get_bool(prism::ContextTypes* value) {
    if (std::holds_alternative<bool>(*value)) {
        return std::get<bool>(*value);
    } else if (std::holds_alternative<int>(*value)) {
        return std::get<int>(*value) == 1;
    }
    return false;
}

#undef RAND_NOISE

prism::ContextTypes* p_append_formula(prism::ContextTypes* a_arg, prism::ContextTypes* a_single,
                                      prism::ContextTypes* a_mult, prism::ContextTypes* a_mix,
                                      prism::ContextTypes* a_with_alpha, prism::ContextTypes* a_only_alpha,
                                      prism::ContextTypes* a_alpha, prism::ContextTypes* a_first_cycle) {
    auto c = std::get<prism::MTDArray<int>>(*a_arg);
    bool do_single = p_get_bool(a_single);
    bool do_multiply = p_get_bool(a_mult);
    bool do_mix = p_get_bool(a_mix);
    bool with_alpha = p_get_bool(a_with_alpha);
    bool only_alpha = p_get_bool(a_only_alpha);
    bool opt_alpha = p_get_bool(a_alpha);
    bool first_cycle = p_get_bool(a_first_cycle);
    std::string out = "";
    if (do_single) {
        out += p_shader_item_to_str(c.at(only_alpha, 3), with_alpha, only_alpha, opt_alpha, first_cycle, false);
    } else if (do_multiply) {
        out += p_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += " * ";
        out += p_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
    } else if (do_mix) {
        out += "mix(";
        out += p_shader_item_to_str(c.at(only_alpha, 1), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ", ";
        out += p_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ", ";
        out += p_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
        out += ")";
    } else {
        out += "(";
        out += p_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += " - ";
        out += p_shader_item_to_str(c.at(only_alpha, 1), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ") * ";
        out += p_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
        out += " + ";
        out += p_shader_item_to_str(c.at(only_alpha, 3), with_alpha, only_alpha, opt_alpha, first_cycle, false);
    }
    return new prism::ContextTypes{ out };
}

static int vertex_index;
static size_t raw_num_floats = 0;
static MTL::VertexDescriptor* vertex_descriptor;

prism::ContextTypes* update_raw_floats(prism::ContextTypes* num) {
    MTL::VertexFormat format;
    int size = std::get<int>(*num);
    switch (size) {
        case 4:
            format = MTL::VertexFormatFloat4;
            break;
        case 3:
            format = MTL::VertexFormatFloat3;
            break;
        case 2:
            format = MTL::VertexFormatFloat2;
            break;
        case 1:
            format = MTL::VertexFormatFloat;
            break;
    }
    vertex_descriptor->attributes()->object(vertex_index)->setFormat(format);
    vertex_descriptor->attributes()->object(vertex_index)->setBufferIndex(0);
    vertex_descriptor->attributes()->object(vertex_index++)->setOffset(raw_num_floats * sizeof(float));
    raw_num_floats += size;

    return nullptr;
}

prism::ContextTypes* get_vertex_index() {
    return new prism::ContextTypes{ vertex_index };
}

std::optional<std::string> metal_include_fs(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));
    if (res == nullptr) {
        return std::nullopt;
    }
    auto inc = static_cast<std::string*>(res->GetRawPointer());
    return *inc;
}

// MARK: - Public Methods

MTL::VertexDescriptor* gfx_metal_build_shader(std::string& result, size_t& num_floats, const CCFeatures& cc_features,
                                              bool three_point_filtering) {

    vertex_descriptor = MTL::VertexDescriptor::vertexDescriptor();
    vertex_index = 0;
    raw_num_floats = 0;

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
        { "o_c", M_ARRAY(cc_features.c, int, 2, 2, 4) },
        { "o_alpha", cc_features.opt_alpha },
        { "o_fog", cc_features.opt_fog },
        { "o_texture_edge", cc_features.opt_texture_edge },
        { "o_noise", cc_features.opt_noise },
        { "o_2cyc", cc_features.opt_2cyc },
        { "o_alpha_threshold", cc_features.opt_alpha_threshold },
        { "o_invisible", cc_features.opt_invisible },
        { "o_grayscale", cc_features.opt_grayscale },
        { "o_textures", M_ARRAY(cc_features.used_textures, bool, 2) },
        { "o_masks", M_ARRAY(cc_features.used_masks, bool, 2) },
        { "o_blend", M_ARRAY(cc_features.used_blend, bool, 2) },
        { "o_clamp", M_ARRAY(cc_features.clamp, bool, 2, 2) },
        { "o_inputs", cc_features.num_inputs },
        { "o_do_mix", M_ARRAY(cc_features.do_mix, bool, 2, 2) },
        { "o_do_single", M_ARRAY(cc_features.do_single, bool, 2, 2) },
        { "o_do_multiply", M_ARRAY(cc_features.do_multiply, bool, 2, 2) },
        { "o_color_alpha_same", M_ARRAY(cc_features.color_alpha_same, bool, 2) },
        { "o_three_point_filtering", three_point_filtering },
        { "get_vertex_index", (InvokeFunc)get_vertex_index },
        { "append_formula", (InvokeFunc)p_append_formula },
        { "update_floats", (InvokeFunc)update_raw_floats },
    };
    processor.populate(context);
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    auto res = static_pointer_cast<Ship::Shader>(Ship::Context::GetInstance()->GetResourceManager()->LoadResource(
        "shaders/metal/default.shader.metal", true, init));

    if (res == nullptr) {
        SPDLOG_ERROR("Failed to load default metal shader, missing f3d.o2r?");
        abort();
    }

    auto shader = static_cast<std::string*>(res->GetRawPointer());
    processor.load(*shader);
    processor.bind_include_loader(metal_include_fs);
    result = processor.process();
    // SPDLOG_INFO("=========== METAL SHADER ============");
    // SPDLOG_INFO(result);
    // SPDLOG_INFO("====================================");
    vertex_descriptor->layouts()->object(0)->setStride(raw_num_floats * sizeof(float));
    vertex_descriptor->layouts()->object(0)->setStepFunction(MTL::VertexStepFunctionPerVertex);
    num_floats = raw_num_floats;
    return vertex_descriptor;
}

#endif
