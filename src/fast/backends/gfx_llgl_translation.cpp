// gfx_llgl_translation.cpp
//
// GLSL → SPIR-V → target shading language translation layer.
// See gfx_llgl_translation.h for the public API.
#include "gfx_llgl_translation.h"
#ifdef ENABLE_LLGL

// ---- LLGL (must come before any X11 / WinAPI headers) ---------------------
#include <LLGL/LLGL.h>
#ifdef True
#  undef True
#endif
#ifdef False
#  undef False
#endif

// ---- glslang (GLSL → SPIR-V) ----------------------------------------------
#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>

// ---- SPIRV-Cross (SPIR-V → GLSL / HLSL / MSL) ----------------------------
// Headers live in the root of the SPIRV-Cross source tree (no subdirectory).
#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_msl.hpp>

#include <spdlog/spdlog.h>

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

namespace Fast {

// ---------------------------------------------------------------------------
// Default TBuiltInResource limits (permissive values for a modern GPU).
// ---------------------------------------------------------------------------
static const TBuiltInResource kDefaultResources = []() {
    TBuiltInResource r{};
    r.maxLights                                   = 32;
    r.maxClipPlanes                               = 6;
    r.maxTextureUnits                             = 32;
    r.maxTextureCoords                            = 32;
    r.maxVertexAttribs                            = 64;
    r.maxVertexUniformComponents                  = 4096;
    r.maxVaryingFloats                            = 64;
    r.maxVertexTextureImageUnits                  = 32;
    r.maxCombinedTextureImageUnits                = 80;
    r.maxTextureImageUnits                        = 32;
    r.maxFragmentUniformComponents                = 4096;
    r.maxDrawBuffers                              = 32;
    r.maxVertexUniformVectors                     = 128;
    r.maxVaryingVectors                           = 8;
    r.maxFragmentUniformVectors                   = 16;
    r.maxVertexOutputVectors                      = 16;
    r.maxFragmentInputVectors                     = 15;
    r.minProgramTexelOffset                       = -8;
    r.maxProgramTexelOffset                       = 7;
    r.maxClipDistances                            = 8;
    r.maxComputeWorkGroupCountX                   = 65535;
    r.maxComputeWorkGroupCountY                   = 65535;
    r.maxComputeWorkGroupCountZ                   = 65535;
    r.maxComputeWorkGroupSizeX                    = 1024;
    r.maxComputeWorkGroupSizeY                    = 1024;
    r.maxComputeWorkGroupSizeZ                    = 64;
    r.maxComputeUniformComponents                 = 1024;
    r.maxComputeTextureImageUnits                 = 16;
    r.maxComputeImageUniforms                     = 8;
    r.maxComputeAtomicCounters                    = 8;
    r.maxComputeAtomicCounterBuffers              = 1;
    r.maxVaryingComponents                        = 60;
    r.maxVertexOutputComponents                   = 64;
    r.maxGeometryInputComponents                  = 64;
    r.maxGeometryOutputComponents                 = 128;
    r.maxFragmentInputComponents                  = 128;
    r.maxImageUnits                               = 8;
    r.maxCombinedImageUnitsAndFragmentOutputs     = 8;
    r.maxCombinedShaderOutputResources            = 8;
    r.maxImageSamples                             = 0;
    r.maxVertexImageUniforms                      = 0;
    r.maxTessControlImageUniforms                 = 0;
    r.maxTessEvaluationImageUniforms              = 0;
    r.maxGeometryImageUniforms                    = 0;
    r.maxFragmentImageUniforms                    = 8;
    r.maxCombinedImageUniforms                    = 8;
    r.maxGeometryTextureImageUnits                = 16;
    r.maxGeometryOutputVertices                   = 256;
    r.maxGeometryTotalOutputComponents            = 1024;
    r.maxGeometryUniformComponents                = 1024;
    r.maxGeometryVaryingComponents                = 64;
    r.maxTessControlInputComponents               = 128;
    r.maxTessControlOutputComponents              = 128;
    r.maxTessControlTextureImageUnits             = 16;
    r.maxTessControlUniformComponents             = 1024;
    r.maxTessControlTotalOutputComponents         = 4096;
    r.maxTessEvaluationInputComponents            = 128;
    r.maxTessEvaluationOutputComponents           = 128;
    r.maxTessEvaluationTextureImageUnits          = 16;
    r.maxTessEvaluationUniformComponents          = 1024;
    r.maxTessPatchComponents                      = 120;
    r.maxPatchVertices                            = 32;
    r.maxTessGenLevel                             = 64;
    r.maxViewports                                = 16;
    r.maxVertexAtomicCounters                     = 0;
    r.maxTessControlAtomicCounters                = 0;
    r.maxTessEvaluationAtomicCounters             = 0;
    r.maxGeometryAtomicCounters                   = 0;
    r.maxFragmentAtomicCounters                   = 8;
    r.maxCombinedAtomicCounters                   = 8;
    r.maxAtomicCounterBindings                    = 1;
    r.maxVertexAtomicCounterBuffers               = 0;
    r.maxTessControlAtomicCounterBuffers          = 0;
    r.maxTessEvaluationAtomicCounterBuffers       = 0;
    r.maxGeometryAtomicCounterBuffers             = 0;
    r.maxFragmentAtomicCounterBuffers             = 1;
    r.maxCombinedAtomicCounterBuffers             = 1;
    r.maxAtomicCounterBufferSize                  = 16384;
    r.maxTransformFeedbackBuffers                 = 4;
    r.maxTransformFeedbackInterleavedComponents   = 64;
    r.maxCullDistances                            = 8;
    r.maxCombinedClipAndCullDistances             = 8;
    r.maxSamples                                  = 4;
    r.maxMeshOutputVerticesNV                     = 256;
    r.maxMeshOutputPrimitivesNV                   = 512;
    r.maxMeshWorkGroupSizeX_NV                    = 32;
    r.maxMeshWorkGroupSizeY_NV                    = 1;
    r.maxMeshWorkGroupSizeZ_NV                    = 1;
    r.maxTaskWorkGroupSizeX_NV                    = 32;
    r.maxTaskWorkGroupSizeY_NV                    = 1;
    r.maxTaskWorkGroupSizeZ_NV                    = 1;
    r.maxMeshViewCountNV                          = 4;
    r.limits.nonInductiveForLoops                 = true;
    r.limits.whileLoops                           = true;
    r.limits.doWhileLoops                         = true;
    r.limits.generalUniformIndexing               = true;
    r.limits.generalAttributeMatrixVectorIndexing = true;
    r.limits.generalVaryingIndexing               = true;
    r.limits.generalSamplerIndexing               = true;
    r.limits.generalVariableIndexing              = true;
    r.limits.generalConstantMatrixVectorIndexing  = true;
    return r;
}();

// ---------------------------------------------------------------------------
// GlslToSpirv
// ---------------------------------------------------------------------------
bool GlslToSpirv(const std::string& source,
                 bool               isVertex,
                 std::vector<uint32_t>& spirvOut,
                 std::string&           errOut) {
    glslang::InitializeProcess();

    EShLanguage stage = isVertex ? EShLangVertex : EShLangFragment;
    glslang::TShader shader(stage);

    const char* src = source.c_str();
    shader.setStrings(&src, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage,
                       glslang::EShClientVulkan, 100);
    shader.setEnvTarget(glslang::EShTargetSpv,
                        glslang::EShTargetSpv_1_5);
    shader.setEnvClient(glslang::EShClientVulkan,
                        glslang::EShTargetVulkan_1_2);

    const EShMessages msgs = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(&kDefaultResources, 450, false, msgs)) {
        errOut = shader.getInfoLog();
        glslang::FinalizeProcess();
        return false;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(msgs)) {
        errOut = program.getInfoLog();
        glslang::FinalizeProcess();
        return false;
    }

    glslang::SpvOptions opts;
    opts.disableOptimizer = false;
    opts.optimizeSize     = false;
    glslang::GlslangToSpv(*program.getIntermediate(stage), spirvOut, &opts);

    glslang::FinalizeProcess();
    return true;
}

// ---------------------------------------------------------------------------
// Language detection helpers
// ---------------------------------------------------------------------------
static bool IsGlsl(const std::vector<LLGL::ShadingLanguage>& langs, int& outVersion) {
    for (auto l : langs) {
        if (l == LLGL::ShadingLanguage::GLSL) { outVersion = 330; return true; }
        if ((static_cast<int>(l) & static_cast<int>(LLGL::ShadingLanguage::GLSL)) != 0 &&
            l != LLGL::ShadingLanguage::ESSL) {
            outVersion = static_cast<int>(l) &
                         static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (outVersion == 0) outVersion = 330;
            return true;
        }
    }
    return false;
}

static bool IsGlslEs(const std::vector<LLGL::ShadingLanguage>& langs, int& outVersion) {
    for (auto l : langs) {
        if (l == LLGL::ShadingLanguage::ESSL) { outVersion = 300; return true; }
        if ((static_cast<int>(l) & static_cast<int>(LLGL::ShadingLanguage::ESSL)) != 0) {
            outVersion = static_cast<int>(l) &
                         static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (outVersion == 0) outVersion = 300;
            return true;
        }
    }
    return false;
}

static bool IsSpirv(const std::vector<LLGL::ShadingLanguage>& langs) {
    for (auto l : langs)
        if ((static_cast<int>(l) & static_cast<int>(LLGL::ShadingLanguage::SPIRV)) != 0)
            return true;
    return false;
}

static bool IsHlsl(const std::vector<LLGL::ShadingLanguage>& langs, int& outVersion) {
    for (auto l : langs) {
        if (l == LLGL::ShadingLanguage::HLSL) { outVersion = 50; return true; }
        if ((static_cast<int>(l) & static_cast<int>(LLGL::ShadingLanguage::HLSL)) != 0) {
            outVersion = static_cast<int>(l) &
                         static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (outVersion == 0) outVersion = 50;
            return true;
        }
    }
    return false;
}

static bool IsMsl(const std::vector<LLGL::ShadingLanguage>& langs, int& outVersion) {
    for (auto l : langs) {
        if (l == LLGL::ShadingLanguage::Metal) { outVersion = 21; return true; }
        if ((static_cast<int>(l) & static_cast<int>(LLGL::ShadingLanguage::Metal)) != 0) {
            outVersion = static_cast<int>(l) &
                         static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (outVersion == 0) outVersion = 21;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// SpirvToLLGL
// ---------------------------------------------------------------------------
bool SpirvToLLGL(const std::vector<uint32_t>& spirv,
                 bool                          isVertex,
                 const void*                   langCodesPtr,
                 void*                         vtxFmtPtr,
                 void*                         descPtr,
                 std::vector<uint32_t>&        spirvBuf,
                 std::string&                  sourceBuf,
                 std::string&                  errOut) {
    const auto& langs =
        *static_cast<const std::vector<LLGL::ShadingLanguage>*>(langCodesPtr);
    auto& desc   = *static_cast<LLGL::ShaderDescriptor*>(descPtr);
    auto& vtxFmt = *static_cast<LLGL::VertexFormat*>(vtxFmtPtr);

    desc.type = isVertex ? LLGL::ShaderType::Vertex : LLGL::ShaderType::Fragment;

    int version = 0;

    // ----------------------------------------------------------------
    // Priority: SPIR-V binary > GLSL > GLSL ES > HLSL > MSL
    // ----------------------------------------------------------------
    if (IsSpirv(langs)) {
        // Vulkan passthrough – no cross-compilation needed.
        spirvBuf = spirv;
        desc.sourceType = LLGL::ShaderSourceType::BinaryBuffer;
        desc.source     = reinterpret_cast<const char*>(spirvBuf.data());
        desc.sourceSize = spirvBuf.size() * sizeof(uint32_t);
        desc.entryPoint = "main";
        return true;
    }

    if (IsGlsl(langs, version)) {
        // GLSL target (OpenGL backend).
        spirv_cross::CompilerGLSL::Options opts;
        opts.version = version;
        opts.es      = false;
#ifdef __APPLE__
        opts.enable_420pack_extension = false;
#endif

        spirv_cross::CompilerGLSL comp(spirv);
        comp.set_common_options(opts);
        // Combined image samplers are required for GLSL (no separate texture2D/sampler).
        comp.build_combined_image_samplers();
        // Preserve the original sampler names so LLGL can match by name/slot.
        for (const auto& s : comp.get_combined_image_samplers())
            comp.set_name(s.combined_id, comp.get_name(s.image_id));

        try {
            sourceBuf = comp.compile();
        } catch (const spirv_cross::CompilerError& e) {
            errOut = e.what();
            return false;
        }

        desc.sourceType = LLGL::ShaderSourceType::CodeString;
        desc.source     = sourceBuf.c_str();
        desc.sourceSize = sourceBuf.size();
        desc.entryPoint = "main";
        return true;
    }

    if (IsGlslEs(langs, version)) {
        // GLSL ES target (OpenGL ES / Android backend).
        spirv_cross::CompilerGLSL::Options opts;
        opts.version = version;
        opts.es      = true;

        spirv_cross::CompilerGLSL comp(spirv);
        comp.set_common_options(opts);
        comp.build_combined_image_samplers();
        for (const auto& s : comp.get_combined_image_samplers())
            comp.set_name(s.combined_id, comp.get_name(s.image_id));

        try {
            sourceBuf = comp.compile();
        } catch (const spirv_cross::CompilerError& e) {
            errOut = e.what();
            return false;
        }

        desc.sourceType = LLGL::ShaderSourceType::CodeString;
        desc.source     = sourceBuf.c_str();
        desc.sourceSize = sourceBuf.size();
        desc.entryPoint = "main";
        return true;
    }

    if (IsHlsl(langs, version)) {
        // HLSL target (Direct3D 11/12 backend).
        spirv_cross::CompilerHLSL::Options hlslOpts;
        hlslOpts.shader_model = version / 10;  // version is e.g. 50 → SM 5.0

        spirv_cross::CompilerHLSL comp(spirv);
        comp.set_hlsl_options(hlslOpts);

        // Remap vertex attribute semantics for D3D input layout.
        // Convention: location 0 → POSITION, remaining → TEXCOORD0, TEXCOORD1, …
        if (isVertex) {
            int texIdx = 0;
            for (size_t i = 0; i < vtxFmt.attributes.size(); i++) {
                spirv_cross::HLSLVertexAttributeRemap remap;
                remap.index    = static_cast<uint32_t>(i);
                remap.semantic = (i == 0) ? "POSITION"
                                          : ("TEXCOORD" + std::to_string(texIdx++));
                comp.add_vertex_attribute_remap(remap);
            }
        }

        try {
            sourceBuf = comp.compile();
        } catch (const spirv_cross::CompilerError& e) {
            errOut = e.what();
            return false;
        }

        desc.sourceType = LLGL::ShaderSourceType::CodeString;
        desc.source     = sourceBuf.c_str();
        desc.sourceSize = sourceBuf.size();
        desc.entryPoint = "main";
        // Profile stored in sourceBuf2 (abuse for profile string).
        // We store it in a static to avoid dangling pointer; this is safe
        // because BuildPipelineState consumes it before the next call.
        static std::string sHlslProfile;
        sHlslProfile = isVertex ? ("vs_" + std::to_string(version / 10) + "_0")
                                : ("ps_" + std::to_string(version / 10) + "_0");
        desc.profile = sHlslProfile.c_str();

        // NOTE: LLGL::VertexAttribute.name must use D3D semantic names for
        // Direct3D backends.  We signal this need to the caller by setting
        // semanticIndex to UINT32_MAX on the first attribute (the caller
        // (BuildPipelineState) will patch all names from prog->attrSemanticNames).
        // This avoids dependency on LLGL::CopyTag / StringLiteral copy constructor.
        if (isVertex && !vtxFmt.attributes.empty()) {
            int texIdx = 0;
            for (size_t i = 0; i < vtxFmt.attributes.size(); i++) {
                vtxFmt.attributes[i].semanticIndex =
                    (i == 0) ? 0 : static_cast<uint32_t>(texIdx++);
            }
            // Mark as HLSL-patched so the caller knows to set .name strings.
            vtxFmt.attributes[0].semanticIndex = UINT32_MAX;
        }
        return true;
    }

    if (IsMsl(langs, version)) {
        // MSL target (Metal backend).
        spirv_cross::CompilerMSL::Options mslOpts;
        mslOpts.enable_decoration_binding = true;

        spirv_cross::CompilerMSL comp(spirv);
        comp.set_msl_options(mslOpts);

        try {
            sourceBuf = comp.compile();
        } catch (const spirv_cross::CompilerError& e) {
            errOut = e.what();
            return false;
        }

        desc.sourceType = LLGL::ShaderSourceType::CodeString;
        desc.source     = sourceBuf.c_str();
        desc.sourceSize = sourceBuf.size();
        desc.entryPoint = "main0";
        desc.profile    = (version / 10 > 0)
                          ? (std::to_string(version / 10) + "." +
                             std::to_string(version % 10))
                          : "2.1";
        return true;
    }

    errOut = "[LLGL translation] No supported shading language found for this renderer.";
    return false;
}

} // namespace Fast
#endif // ENABLE_LLGL
