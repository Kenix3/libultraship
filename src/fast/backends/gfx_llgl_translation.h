// gfx_llgl_translation.h
//
// Shader translation layer for the LLGL rendering backend.
//
// Converts Vulkan-flavoured GLSL (#version 450 core) to the shading language
// that the active LLGL renderer actually accepts:
//   • GLSL 3.x / 4.x  – OpenGL backend
//   • GLSL ES           – OpenGL ES / Android backend
//   • SPIR-V binary     – Vulkan backend  (passthrough, no cross-compilation)
//   • HLSL              – Direct3D 11 / 12 backend
//   • MSL               – Metal backend
//
// The two-step pipeline is:
//   1.  GlslToSpirv()  – compile Vulkan GLSL to SPIR-V using glslang
//   2.  SpirvToLLGL()  – cross-compile SPIR-V to the target using SPIRV-Cross
//                        and fill an LLGL::ShaderDescriptor that is ready to
//                        pass to LLGL::RenderSystem::CreateShader().
//
// Both functions are intentionally free of LLGL headers in this header file
// so that it can be included early in gfx_llgl.cpp (before the X11 header
// workaround block).  The LLGL types are accessed through opaque void* in the
// public API; the casts back to the concrete types live in the .cpp file.
#pragma once
#ifdef ENABLE_LLGL

#include <cstdint>
#include <string>
#include <vector>

namespace Fast {

// ---------------------------------------------------------------------------
// GlslToSpirv
//
// Compile a Vulkan-flavoured GLSL source string into a SPIR-V binary.
//
//   source   – complete GLSL source (#version 450 core, explicit bindings …)
//   isVertex – true for vertex stage, false for fragment stage
//   spirvOut – receives the SPIR-V words on success
//   errOut   – receives a human-readable error message on failure
//
// Returns true on success.
// ---------------------------------------------------------------------------
bool GlslToSpirv(const std::string& source,
                 bool               isVertex,
                 std::vector<uint32_t>& spirvOut,
                 std::string&           errOut);

// ---------------------------------------------------------------------------
// SpirvToLLGL
//
// Translate SPIR-V binary to the best shading language supported by the
// active LLGL renderer and populate the LLGL::ShaderDescriptor.
//
//   spirv         – SPIR-V words produced by GlslToSpirv()
//   isVertex      – true for vertex stage, false for fragment stage
//   langCodesPtr  – opaque pointer to std::vector<LLGL::ShadingLanguage>
//                   obtained from LLGL::RenderSystem::GetRenderingCaps()
//   vtxFmtPtr     – opaque pointer to LLGL::VertexFormat (vertex stage only);
//                   when the HLSL path is selected the function patches the
//                   attribute names/semanticIndex for the D3D input layout
//   descPtr       – opaque pointer to LLGL::ShaderDescriptor to fill;
//                   desc.source / desc.sourceSize / desc.sourceType / etc.
//                   are set by this function
//   spirvBuf      – storage that owns the SPIR-V words when the Vulkan
//                   binary passthrough path is chosen; must outlive `desc`
//   sourceBuf     – storage that owns the text source for GLSL / HLSL / MSL;
//                   must outlive `desc`
//   errOut        – receives a human-readable error message on failure
//
// Returns true on success.
// ---------------------------------------------------------------------------
bool SpirvToLLGL(const std::vector<uint32_t>& spirv,
                 bool                          isVertex,
                 const void*                   langCodesPtr,
                 void*                         vtxFmtPtr,
                 void*                         descPtr,
                 std::vector<uint32_t>&        spirvBuf,
                 std::string&                  sourceBuf,
                 std::string&                  errOut);

} // namespace Fast
#endif // ENABLE_LLGL
