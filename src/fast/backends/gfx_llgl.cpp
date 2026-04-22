#include "ship/window/Window.h"
#ifdef ENABLE_LLGL

// ============================================================
// IMPORTANT: Include prism *before* LLGL so that X11 headers
// (pulled in by LLGL's Linux OpenGL/GLX back-end) do not yet
// define "True 1" / "False 0" macros that collide with the
// prism::lexer::TokenType enum values.
// ============================================================
#include "fast/backends/gfx_llgl.h"
#include "fast/backends/gfx_llgl_translation.h"
#include "fast/interpreter.h"
#include "ship/Context.h"
#include "ship/resource/factory/ShaderFactory.h"
#include "ship/config/ConsoleVariable.h"

#include <prism/processor.h>

// Now safe to pull in LLGL (may transitively include X11/OpenGL headers).
#include <LLGL/LLGL.h>
// Undo X11's True/False macros so the rest of this file uses C++ booleans.
#ifdef True
#  undef True
#endif
#ifdef False
#  undef False
#endif

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Shared prism formula helpers (gfx_append_formula, gfx_cc_get_features, etc.)
#include "gfx_glsl_helpers.h"

namespace {

// ---------------------------------------------------------------------------
// Static state for prism callbacks (reset before each proc.process() call).
// Single-threaded shader compilation makes this safe.
// ---------------------------------------------------------------------------
static LLGL::VertexFormat*                      sVtxFmt     = nullptr;
static LLGL::PipelineLayoutDescriptor*          sLayoutDesc = nullptr;
static std::vector<Fast::LLGLBindingSlot>*      sHeapSlots  = nullptr;

static int    sVsInputIdx  = 0;  // current VS input location
static int    sVsOutputIdx = 0;  // current VS→FS interpolant location
static int    sFragInIdx   = 0;  // current FS input location (must mirror VS output)
static int    sBindingIdx  = 0;  // current layout binding slot
static size_t sFloatCount  = 0;  // running total floats per vertex
static size_t sByteOffset  = 0;  // running byte offset within a vertex record

// ---------------------------------------------------------------------------
// Vertex shader callbacks
// ---------------------------------------------------------------------------

// vs_in_loc(fmt)  – record a vertex attribute and return its input location.
// fmt codes: 1=R32Float, 2=RG32Float, 3=RGB32Float, 4=RGBA32Float.
static prism::ContextTypes* VsInLoc(prism::ContextTypes* /*ctx*/,
                                     prism::ContextTypes* pFmt) {
    int fmt = std::get<int>(*pFmt);
    int loc = sVsInputIdx++;

    LLGL::Format llglFmt;
    size_t       numF;
    switch (fmt) {
        case 1: llglFmt = LLGL::Format::R32Float;    numF = 1; break;
        case 2: llglFmt = LLGL::Format::RG32Float;   numF = 2; break;
        case 3: llglFmt = LLGL::Format::RGB32Float;  numF = 3; break;
        default: llglFmt = LLGL::Format::RGBA32Float; numF = 4; break;
    }

    LLGL::VertexAttribute va;
    va.location = static_cast<uint32_t>(loc);
    va.format   = llglFmt;
    va.offset   = static_cast<uint32_t>(sByteOffset);
    va.stride   = 0; // patched after all attributes are known
    sVtxFmt->attributes.push_back(va);

    sByteOffset += numF * sizeof(float);
    sFloatCount += numF;

    return new prism::ContextTypes{ loc };
}

// vs_out_loc(0)  – return and increment the VS→FS interpolant location counter.
static prism::ContextTypes* VsOutLoc(prism::ContextTypes* /*ctx*/,
                                      prism::ContextTypes* /*dummy*/) {
    return new prism::ContextTypes{ sVsOutputIdx++ };
}

// ---------------------------------------------------------------------------
// Fragment shader callbacks
// ---------------------------------------------------------------------------

// fc_binding(0)  – allocate a layout binding for the frame_count UBO.
static prism::ContextTypes* FcBinding(prism::ContextTypes* /*ctx*/,
                                       prism::ContextTypes* /*dummy*/) {
    int b = sBindingIdx++;
    LLGL::BindingDescriptor bd;
    bd.type       = LLGL::ResourceType::Buffer;
    bd.bindFlags  = LLGL::BindFlags::ConstantBuffer;
    bd.stageFlags = LLGL::StageFlags::FragmentStage;
    bd.slot       = LLGL::BindingSlot{ static_cast<uint32_t>(b) };
    sLayoutDesc->bindings.push_back(bd);
    sHeapSlots->push_back({ Fast::LLGLBindingType::FrameCount, 0 });
    return new prism::ContextTypes{ b };
}

// ns_binding(0)  – allocate a layout binding for the noise_scale UBO.
static prism::ContextTypes* NsBinding(prism::ContextTypes* /*ctx*/,
                                       prism::ContextTypes* /*dummy*/) {
    int b = sBindingIdx++;
    LLGL::BindingDescriptor bd;
    bd.type       = LLGL::ResourceType::Buffer;
    bd.bindFlags  = LLGL::BindFlags::ConstantBuffer;
    bd.stageFlags = LLGL::StageFlags::FragmentStage;
    bd.slot       = LLGL::BindingSlot{ static_cast<uint32_t>(b) };
    sLayoutDesc->bindings.push_back(bd);
    sHeapSlots->push_back({ Fast::LLGLBindingType::NoiseScale, 0 });
    return new prism::ContextTypes{ b };
}

// tex_binding(i, type)  – allocate a layout binding for a texture2D uniform.
// type: 0=regular, 1=mask, 2=blend.
static prism::ContextTypes* TexBinding(prism::ContextTypes* /*ctx*/,
                                        prism::ContextTypes* pIdx,
                                        prism::ContextTypes* pType) {
    int texIdx  = std::get<int>(*pIdx);
    int texType = std::get<int>(*pType);
    int b = sBindingIdx++;

    LLGL::BindingDescriptor bd;
    bd.type       = LLGL::ResourceType::Texture;
    bd.bindFlags  = LLGL::BindFlags::Sampled;
    bd.stageFlags = LLGL::StageFlags::FragmentStage;
    bd.slot       = LLGL::BindingSlot{ static_cast<uint32_t>(b) };
    sLayoutDesc->bindings.push_back(bd);

    Fast::LLGLBindingType bt;
    switch (texType) {
        case 1:  bt = Fast::LLGLBindingType::MaskTex;  break;
        case 2:  bt = Fast::LLGLBindingType::BlendTex; break;
        default: bt = Fast::LLGLBindingType::Texture;  break;
    }
    sHeapSlots->push_back({ bt, static_cast<uint8_t>(texIdx) });
    return new prism::ContextTypes{ b };
}

// samp_binding(i, type)  – allocate a layout binding for a sampler uniform.
static prism::ContextTypes* SampBinding(prism::ContextTypes* /*ctx*/,
                                          prism::ContextTypes* pIdx,
                                          prism::ContextTypes* pType) {
    int texIdx  = std::get<int>(*pIdx);
    int texType = std::get<int>(*pType);
    int b = sBindingIdx++;

    LLGL::BindingDescriptor bd;
    bd.type       = LLGL::ResourceType::Sampler;
    bd.bindFlags  = 0;
    bd.stageFlags = LLGL::StageFlags::FragmentStage;
    bd.slot       = LLGL::BindingSlot{ static_cast<uint32_t>(b) };
    sLayoutDesc->bindings.push_back(bd);

    Fast::LLGLBindingType bt;
    switch (texType) {
        case 1:  bt = Fast::LLGLBindingType::MaskSampler;  break;
        case 2:  bt = Fast::LLGLBindingType::BlendSampler; break;
        default: bt = Fast::LLGLBindingType::Sampler;      break;
    }
    sHeapSlots->push_back({ bt, static_cast<uint8_t>(texIdx) });
    return new prism::ContextTypes{ b };
}

// frag_in_loc(0)  – return and increment the FS input location counter.
static prism::ContextTypes* FragInLoc(prism::ContextTypes* /*ctx*/,
                                       prism::ContextTypes* /*dummy*/) {
    return new prism::ContextTypes{ sFragInIdx++ };
}

// ---------------------------------------------------------------------------
// @include loader for prism
// ---------------------------------------------------------------------------
static std::optional<std::string> LlglInclude(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type      = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format    = RESOURCE_FORMAT_BINARY;
    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));
    if (!res) return std::nullopt;
    return *static_cast<std::string*>(res->GetRawPointer());
}

} // anonymous namespace

namespace Fast {

const char* const GfxRenderingAPILLGL::kPreferredModules[] = {
#ifdef _WIN32
    "Direct3D12",
    "Direct3D11",
#elif defined(__APPLE__)
    "Metal",
#endif
    "OpenGL",
    nullptr
};

// ---------------------------------------------------------------------------
// BuildVertGlsl  –  generate Vulkan GLSL vertex source via prism template
// ---------------------------------------------------------------------------
std::string GfxRenderingAPILLGL::BuildVertGlsl(const CCFeatures& cc,
                                                void* vtxFmtPtr) const {
    sVtxFmt     = static_cast<LLGL::VertexFormat*>(vtxFmtPtr);
    sVsInputIdx = 0;
    sVsOutputIdx = 0;
    sFloatCount = 0;
    sByteOffset = 0;
    sVtxFmt->attributes.clear();

    prism::Processor proc;
    prism::ContextItems ctx = {
        { "o_textures",  M_ARRAY(cc.usedTextures, bool, 2) },
        { "o_clamp",     M_ARRAY(cc.clamp, bool, 2, 2) },
        { "o_fog",       cc.opt_fog },
        { "o_grayscale", cc.opt_grayscale },
        { "o_alpha",     cc.opt_alpha },
        { "o_inputs",    cc.numInputs },
        { "vs_in_loc",   (InvokeFunc)VsInLoc },
        { "vs_out_loc",  (InvokeFunc)VsOutLoc },
    };
    proc.populate(ctx);

    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type      = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format    = RESOURCE_FORMAT_BINARY;

    const char* shaderName = gfx_get_shader(cc.shader_id);
    std::string path = "shaders/llgl/default.shader.vs";
    if (shaderName) path = std::string(shaderName) + ".llgl.vs";

    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));
    if (!res) {
        SPDLOG_ERROR("[LLGL] Failed to load VS template: {}", path);
        return {};
    }
    proc.load(*static_cast<std::string*>(res->GetRawPointer()));
    proc.bind_include_loader(LlglInclude);
    return proc.process();
}

// ---------------------------------------------------------------------------
// BuildFragGlsl  –  generate Vulkan GLSL fragment source via prism template
// ---------------------------------------------------------------------------
std::string GfxRenderingAPILLGL::BuildFragGlsl(
        const CCFeatures& cc,
        void* layoutDescPtr,
        std::vector<LLGLBindingSlot>& outSlots) const {

    sLayoutDesc = static_cast<LLGL::PipelineLayoutDescriptor*>(layoutDescPtr);
    sHeapSlots  = &outSlots;
    sFragInIdx  = 0;
    sBindingIdx = 0;
    outSlots.clear();
    sLayoutDesc->bindings.clear();

    prism::Processor proc;
    prism::ContextItems ctx = {
        { "o_c",           M_ARRAY(cc.c, int, 2, 2, 4) },
        { "o_alpha",       cc.opt_alpha },
        { "o_fog",         cc.opt_fog },
        { "o_texture_edge",cc.opt_texture_edge },
        { "o_noise",       cc.opt_noise },
        { "o_2cyc",        cc.opt_2cyc },
        { "o_alpha_threshold", cc.opt_alpha_threshold },
        { "o_invisible",   cc.opt_invisible },
        { "o_grayscale",   cc.opt_grayscale },
        { "o_textures",    M_ARRAY(cc.usedTextures, bool, 2) },
        { "o_masks",       M_ARRAY(cc.used_masks, bool, 2) },
        { "o_blend",       M_ARRAY(cc.used_blend, bool, 2) },
        { "o_clamp",       M_ARRAY(cc.clamp, bool, 2, 2) },
        { "o_inputs",      cc.numInputs },
        { "o_do_mix",      M_ARRAY(cc.do_mix, bool, 2, 2) },
        { "o_do_single",   M_ARRAY(cc.do_single, bool, 2, 2) },
        { "o_do_multiply", M_ARRAY(cc.do_multiply, bool, 2, 2) },
        { "o_color_alpha_same", M_ARRAY(cc.color_alpha_same, bool, 2) },
        { "o_three_point_filtering", mFilterMode == FILTER_THREE_POINT },
        { "srgb_mode",     mSrgbMode },
        { "SHADER_0",        SHADER_0 },
        { "SHADER_INPUT_1",  SHADER_INPUT_1 },
        { "SHADER_INPUT_2",  SHADER_INPUT_2 },
        { "SHADER_INPUT_3",  SHADER_INPUT_3 },
        { "SHADER_INPUT_4",  SHADER_INPUT_4 },
        { "SHADER_INPUT_5",  SHADER_INPUT_5 },
        { "SHADER_INPUT_6",  SHADER_INPUT_6 },
        { "SHADER_INPUT_7",  SHADER_INPUT_7 },
        { "SHADER_TEXEL0",   SHADER_TEXEL0 },
        { "SHADER_TEXEL0A",  SHADER_TEXEL0A },
        { "SHADER_TEXEL1",   SHADER_TEXEL1 },
        { "SHADER_TEXEL1A",  SHADER_TEXEL1A },
        { "SHADER_1",        SHADER_1 },
        { "SHADER_COMBINED", SHADER_COMBINED },
        { "SHADER_NOISE",    SHADER_NOISE },
        { "append_formula",  (InvokeFunc)gfx_append_formula },
        { "fc_binding",      (InvokeFunc)FcBinding   },
        { "ns_binding",      (InvokeFunc)NsBinding   },
        { "tex_binding",     (InvokeFunc)TexBinding  },
        { "samp_binding",    (InvokeFunc)SampBinding },
        { "frag_in_loc",     (InvokeFunc)FragInLoc   },
    };
    proc.populate(ctx);

    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type      = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format    = RESOURCE_FORMAT_BINARY;

    const char* shaderName = gfx_get_shader(cc.shader_id);
    std::string path = "shaders/llgl/default.shader.fs";
    if (shaderName) path = std::string(shaderName) + ".llgl.fs";

    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));
    if (!res) {
        SPDLOG_ERROR("[LLGL] Failed to load FS template: {}", path);
        return {};
    }
    proc.load(*static_cast<std::string*>(res->GetRawPointer()));
    proc.bind_include_loader(LlglInclude);
    return proc.process();
}

// ---------------------------------------------------------------------------
// BuildPipelineState  –  GLSL→SPIR-V→target, create LLGL PSO
// ---------------------------------------------------------------------------
bool GfxRenderingAPILLGL::BuildPipelineState(LLGLShaderProgram* prog,
                                              const std::string& vsGlsl,
                                              const std::string& fsGlsl,
                                              void* vtxFmtPtr,
                                              void* layoutDescPtr) {
    if (!mRenderer) return false;

    auto& vtxFmt     = *static_cast<LLGL::VertexFormat*>(vtxFmtPtr);
    auto& layoutDesc = *static_cast<LLGL::PipelineLayoutDescriptor*>(layoutDescPtr);

    // Patch vertex attribute strides (total vertex size is now known).
    const uint32_t stride = prog->numFloats * sizeof(float);
    for (auto& va : vtxFmt.attributes)
        va.stride = stride;

    // Query shading languages supported by the active renderer.
    const auto& langs = mRenderer->GetRenderingCaps().shadingLanguages;

    // --- Compile GLSL → SPIR-V ---
    std::vector<uint32_t> vsSPIRV, fsSPIRV;
    std::string err;

    if (!GlslToSpirv(vsGlsl, /*isVertex=*/true,  vsSPIRV, err)) {
        SPDLOG_ERROR("[LLGL] VS GLSL→SPIR-V:\n{}", err);
        return false;
    }
    if (!GlslToSpirv(fsGlsl, /*isVertex=*/false, fsSPIRV, err)) {
        SPDLOG_ERROR("[LLGL] FS GLSL→SPIR-V:\n{}", err);
        return false;
    }

    // --- Translate SPIR-V → renderer target language ---
    LLGL::ShaderDescriptor vsDesc{}, fsDesc{};
    std::vector<uint32_t> vsSpirvBuf, fsSpirvBuf;
    std::string           vsSrcBuf,   fsSrcBuf;

    if (!SpirvToLLGL(vsSPIRV, true,  &langs, &vtxFmt, &vsDesc, vsSpirvBuf, vsSrcBuf, err)) {
        SPDLOG_ERROR("[LLGL] VS translation: {}", err);
        return false;
    }
    if (!SpirvToLLGL(fsSPIRV, false, &langs, &vtxFmt, &fsDesc, fsSpirvBuf, fsSrcBuf, err)) {
        SPDLOG_ERROR("[LLGL] FS translation: {}", err);
        return false;
    }

    // Add vertex input attributes and fragment output to the descriptors.
    // If SpirvToLLGL set vtxFmt.attributes[0].semanticIndex == UINT32_MAX, the
    // HLSL path was selected; patch attribute names from prog->attrSemanticNames.
    bool hlslPath = !vtxFmt.attributes.empty() &&
                    vtxFmt.attributes[0].semanticIndex == UINT32_MAX;
    if (hlslPath) {
        prog->attrSemanticNames.clear();
        uint32_t texIdx = 0;
        for (size_t i = 0; i < vtxFmt.attributes.size(); i++) {
            if (i == 0) {
                prog->attrSemanticNames.push_back("POSITION");
                vtxFmt.attributes[i].semanticIndex = 0;
            } else {
                prog->attrSemanticNames.push_back("TEXCOORD");
                vtxFmt.attributes[i].semanticIndex = texIdx++;
            }
            vtxFmt.attributes[i].name = prog->attrSemanticNames.back().c_str();
        }
    }

    vsDesc.type = LLGL::ShaderType::Vertex;
    for (const auto& va : vtxFmt.attributes)
        vsDesc.vertex.inputAttribs.push_back(va);

    fsDesc.type = LLGL::ShaderType::Fragment;
    {
        LLGL::FragmentAttribute fo;
        fo.name     = "vOutColor";
        fo.location = 0;
        fo.format   = LLGL::Format::RGBA8UNorm;
        fsDesc.fragment.outputAttribs.push_back(fo);
    }

    // --- Create shader objects ---
    auto* vs = mRenderer->CreateShader(vsDesc);
    if (vs && vs->GetReport() && vs->GetReport()->HasErrors()) {
        SPDLOG_ERROR("[LLGL] VS error: {}", vs->GetReport()->GetText());
        mRenderer->Release(*vs);
        return false;
    }
    auto* fs = mRenderer->CreateShader(fsDesc);
    if (fs && fs->GetReport() && fs->GetReport()->HasErrors()) {
        SPDLOG_ERROR("[LLGL] FS error: {}", fs->GetReport()->GetText());
        if (vs) mRenderer->Release(*vs);
        mRenderer->Release(*fs);
        return false;
    }
    if (!vs || !fs) {
        SPDLOG_ERROR("[LLGL] CreateShader returned null");
        if (vs) mRenderer->Release(*vs);
        if (fs) mRenderer->Release(*fs);
        return false;
    }

    // --- Pipeline layout ---
    prog->pipelineLayout = mRenderer->CreatePipelineLayout(layoutDesc);

    // --- Blend ---
    LLGL::BlendDescriptor blend;
    blend.targets[0].blendEnabled = prog->hasAlpha;
    blend.targets[0].srcColor     = LLGL::BlendOp::SrcAlpha;
    blend.targets[0].dstColor     = LLGL::BlendOp::InvSrcAlpha;
    blend.targets[0].srcAlpha     = LLGL::BlendOp::One;
    blend.targets[0].dstAlpha     = LLGL::BlendOp::Zero;

    // --- Graphics PSO ---
    LLGL::GraphicsPipelineDescriptor psoDesc;
    psoDesc.pipelineLayout    = prog->pipelineLayout;
    psoDesc.vertexShader      = vs;
    psoDesc.fragmentShader    = fs;
    psoDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleList;
    psoDesc.blend             = blend;
    psoDesc.depth.testEnabled  = mPendingDepthTest;
    psoDesc.depth.writeEnabled = mPendingDepthMask;

    prog->pso = mRenderer->CreatePipelineState(psoDesc);

    mRenderer->Release(*vs);
    mRenderer->Release(*fs);

    if (prog->pso && prog->pso->GetReport() && prog->pso->GetReport()->HasErrors()) {
        SPDLOG_ERROR("[LLGL] PSO error: {}", prog->pso->GetReport()->GetText());
        mRenderer->Release(*prog->pso);
        mRenderer->Release(*prog->pipelineLayout);
        prog->pso = nullptr;
        prog->pipelineLayout = nullptr;
        return false;
    }
    return prog->pso != nullptr;
}

// ===========================================================================
// GfxRenderingAPI implementation
// ===========================================================================

GfxRenderingAPILLGL::~GfxRenderingAPILLGL() {
    if (!mRenderer) return;
    if (mCmdBuf)        mRenderer->Release(*mCmdBuf);
    if (mFence)         mRenderer->Release(*mFence);
    if (mVertexBuffer)  mRenderer->Release(*mVertexBuffer);
    if (mFrameCountBuf) mRenderer->Release(*mFrameCountBuf);
    if (mNoiseScaleBuf) mRenderer->Release(*mNoiseScaleBuf);
    for (auto& fb : mFBs) {
        if (fb.renderTarget) mRenderer->Release(*fb.renderTarget);
        if (fb.colorTex)     mRenderer->Release(*fb.colorTex);
        if (fb.depthTex)     mRenderer->Release(*fb.depthTex);
    }
    for (auto& ti : mTextures)
        if (ti.texture) mRenderer->Release(*ti.texture);
    for (auto& kv : mSamplerCache)
        if (kv.second) mRenderer->Release(*kv.second);
    for (auto& kv : mShaderCache) {
        auto& p = kv.second;
        if (p.pso)            mRenderer->Release(*p.pso);
        if (p.pipelineLayout) mRenderer->Release(*p.pipelineLayout);
        if (p.resourceHeap)   mRenderer->Release(*p.resourceHeap);
    }
}

const char*        GfxRenderingAPILLGL::GetName()          { return mModuleName.empty() ? "LLGL" : mModuleName.c_str(); }
int                GfxRenderingAPILLGL::GetMaxTextureSize() { return 8192; }
GfxClipParameters  GfxRenderingAPILLGL::GetClipParameters() {
    bool inv = !mFBs.empty() && mFBs[mCurrentFB].invertY;
    return { false, inv };
}

// ---- Shader ----------------------------------------------------------------
void GfxRenderingAPILLGL::UnloadShader(ShaderProgram*) {}
void GfxRenderingAPILLGL::LoadShader(ShaderProgram* prg) {
    mCurrentProg = reinterpret_cast<LLGLShaderProgram*>(prg);
}
void GfxRenderingAPILLGL::ClearShaderCache() {
    if (mRenderer) {
        for (auto& kv : mShaderCache) {
            auto& p = kv.second;
            if (p.pso)            mRenderer->Release(*p.pso);
            if (p.pipelineLayout) mRenderer->Release(*p.pipelineLayout);
            if (p.resourceHeap)   mRenderer->Release(*p.resourceHeap);
        }
    }
    mShaderCache.clear();
    mCurrentProg = nullptr;
}

ShaderProgram* GfxRenderingAPILLGL::CreateAndLoadNewShader(uint64_t id0, uint64_t id1) {
    if (!mRenderer) return nullptr;

    CCFeatures cc;
    gfx_cc_get_features(id0, id1, &cc);

    auto key  = std::make_pair(id0, id1);
    auto& prog = mShaderCache[key];

    // Build vertex GLSL + populate VertexFormat.
    LLGL::VertexFormat vtxFmt;
    auto vs = BuildVertGlsl(cc, &vtxFmt);
    if (vs.empty()) { mShaderCache.erase(key); return nullptr; }

    prog.numFloats       = static_cast<uint8_t>(sFloatCount);
    prog.numInputs       = static_cast<uint8_t>(cc.numInputs);
    prog.usedTextures[0] = cc.usedTextures[0];
    prog.usedTextures[1] = cc.usedTextures[1];
    prog.usedMasks[0]    = cc.used_masks[0];
    prog.usedMasks[1]    = cc.used_masks[1];
    prog.usedBlend[0]    = cc.used_blend[0];
    prog.usedBlend[1]    = cc.used_blend[1];
    prog.hasAlpha        = cc.opt_alpha;

    // Build fragment GLSL + populate PipelineLayoutDescriptor.
    LLGL::PipelineLayoutDescriptor layoutDesc;
    auto fs = BuildFragGlsl(cc, &layoutDesc, prog.heapSlots);
    if (fs.empty()) { mShaderCache.erase(key); return nullptr; }

    if (!BuildPipelineState(&prog, vs, fs, &vtxFmt, &layoutDesc)) {
        mShaderCache.erase(key);
        return nullptr;
    }

    mCurrentProg = &prog;
    return reinterpret_cast<ShaderProgram*>(&prog);
}

ShaderProgram* GfxRenderingAPILLGL::LookupShader(uint64_t id0, uint64_t id1) {
    auto it = mShaderCache.find({ id0, id1 });
    return (it == mShaderCache.end()) ? nullptr
                                      : reinterpret_cast<ShaderProgram*>(&it->second);
}

void GfxRenderingAPILLGL::ShaderGetInfo(ShaderProgram* prg,
                                          uint8_t* numInputs, bool usedTextures[2]) {
    auto* p = reinterpret_cast<LLGLShaderProgram*>(prg);
    *numInputs = p->numInputs;
    usedTextures[0] = p->usedTextures[0];
    usedTextures[1] = p->usedTextures[1];
}

// ---- Textures --------------------------------------------------------------
uint32_t GfxRenderingAPILLGL::NewTexture() {
    mTextures.push_back({});
    return static_cast<uint32_t>(mTextures.size() - 1);
}
void GfxRenderingAPILLGL::SelectTexture(int tile, uint32_t texId) {
    mCurrentTile = tile;
    mCurrentTexIds[tile] = texId;
}
void GfxRenderingAPILLGL::UploadTexture(const uint8_t* rgba32Buf, uint32_t w, uint32_t h) {
    if (!mRenderer) return;
    auto& ti = mTextures[mCurrentTexIds[mCurrentTile]];
    if (ti.texture && (ti.width != w || ti.height != h)) {
        mRenderer->Release(*ti.texture);
        ti.texture = nullptr;
    }
    LLGL::TextureDescriptor td;
    td.type      = LLGL::TextureType::Texture2D;
    td.format    = LLGL::Format::RGBA8UNorm;
    td.extent    = { w, h, 1 };
    td.bindFlags = LLGL::BindFlags::Sampled;
    td.mipLevels = 1;
    LLGL::ImageView iv;
    iv.format   = LLGL::ImageFormat::RGBA;
    iv.dataType = LLGL::DataType::UInt8;
    iv.data     = rgba32Buf;
    iv.dataSize = w * h * 4;
    if (ti.texture) {
        LLGL::TextureRegion r;
        r.subresource = { 0, 1, 0, 1 };
        r.offset = { 0, 0, 0 };
        r.extent = { w, h, 1 };
        mRenderer->WriteTexture(*ti.texture, r, iv);
    } else {
        ti.texture = mRenderer->CreateTexture(td, &iv);
        ti.width   = static_cast<uint16_t>(w);
        ti.height  = static_cast<uint16_t>(h);
    }
}
void GfxRenderingAPILLGL::SetSamplerParameters(int sampler, bool linear,
                                                  uint32_t cms, uint32_t cmt) {
    mTextures[mCurrentTexIds[sampler]].sampler = GetOrCreateSampler(linear, cms, cmt);
}
void GfxRenderingAPILLGL::DeleteTexture(uint32_t texId) {
    if (!mRenderer || texId >= mTextures.size()) return;
    auto& ti = mTextures[texId];
    if (ti.texture) { mRenderer->Release(*ti.texture); ti.texture = nullptr; }
    ti = {};
}

LLGL::Sampler* GfxRenderingAPILLGL::GetOrCreateSampler(bool linear, uint32_t cms, uint32_t cmt) {
    SamplerKey k{ linear, cms, cmt };
    auto it = mSamplerCache.find(k);
    if (it != mSamplerCache.end()) return it->second;
    auto toAddr = [](uint32_t cm) {
        return (cm & 1) ? LLGL::SamplerAddressMode::Clamp : LLGL::SamplerAddressMode::Repeat;
    };
    LLGL::SamplerDescriptor sd;
    sd.minFilter     = linear ? LLGL::SamplerFilter::Linear : LLGL::SamplerFilter::Nearest;
    sd.magFilter     = linear ? LLGL::SamplerFilter::Linear : LLGL::SamplerFilter::Nearest;
    sd.mipMapFilter  = LLGL::SamplerFilter::Nearest;
    sd.addressModeU  = toAddr(cms);
    sd.addressModeV  = toAddr(cmt);
    sd.addressModeW  = LLGL::SamplerAddressMode::Repeat;
    sd.maxAnisotropy = 1;
    auto* s = mRenderer->CreateSampler(sd);
    mSamplerCache[k] = s;
    return s;
}

// ---- Render state ----------------------------------------------------------
void GfxRenderingAPILLGL::SetDepthTestAndMask(bool dt, bool dm) { mPendingDepthTest=dt; mPendingDepthMask=dm; }
void GfxRenderingAPILLGL::SetZmodeDecal(bool d)                  { mPendingDecal=d; }
void GfxRenderingAPILLGL::SetViewport(int x,int y,int w,int h)   { mViewportX=x; mViewportY=y; mViewportW=w; mViewportH=h; }
void GfxRenderingAPILLGL::SetScissor(int x,int y,int w,int h)    { mScissorX=x; mScissorY=y; mScissorW=w; mScissorH=h; }
void GfxRenderingAPILLGL::SetUseAlpha(bool a)                     { mPendingAlpha=a; }

// ---- Vertex buffer ---------------------------------------------------------
void GfxRenderingAPILLGL::EnsureVertexBufferSize(size_t bytes) {
    if (bytes <= mVertexBufferBytes) return;
    if (mVertexBuffer) mRenderer->Release(*mVertexBuffer);
    LLGL::BufferDescriptor bd;
    bd.size           = static_cast<uint64_t>(bytes * 2);
    bd.bindFlags      = LLGL::BindFlags::VertexBuffer;
    bd.cpuAccessFlags = LLGL::CPUAccessFlags::Write;
    mVertexBuffer      = mRenderer->CreateBuffer(bd);
    mVertexBufferBytes = bytes * 2;
}

// ---- Constant buffers ------------------------------------------------------
void GfxRenderingAPILLGL::EnsureConstantBuffers() {
    if (mFrameCountBuf && mNoiseScaleBuf) return;
    auto make = [&](size_t sz) -> LLGL::Buffer* {
        LLGL::BufferDescriptor bd;
        bd.size           = static_cast<uint64_t>(sz);
        bd.bindFlags      = LLGL::BindFlags::ConstantBuffer;
        bd.cpuAccessFlags = LLGL::CPUAccessFlags::Write;
        return mRenderer->CreateBuffer(bd);
    };
    if (!mFrameCountBuf) mFrameCountBuf = make(sizeof(int));
    if (!mNoiseScaleBuf) mNoiseScaleBuf = make(sizeof(float));
}

// ---- Resource heap ---------------------------------------------------------
void GfxRenderingAPILLGL::UpdateResourceHeap(LLGLShaderProgram* prog) {
    if (prog->heapSlots.empty()) return;
    if (prog->resourceHeap) {
        mRenderer->Release(*prog->resourceHeap);
        prog->resourceHeap = nullptr;
    }

    LLGL::Texture* fallbackTex  = mTextures.empty() ? nullptr : mTextures[0].texture;
    LLGL::Sampler* fallbackSamp = GetOrCreateSampler(false, 0, 0);

    std::vector<LLGL::ResourceViewDescriptor> rv;
    rv.reserve(prog->heapSlots.size());
    for (const auto& slot : prog->heapSlots) {
        switch (slot.type) {
        case LLGLBindingType::FrameCount:
            rv.push_back({ mFrameCountBuf });
            break;
        case LLGLBindingType::NoiseScale:
            rv.push_back({ mNoiseScaleBuf });
            break;
        case LLGLBindingType::Texture:
        case LLGLBindingType::MaskTex:
        case LLGLBindingType::BlendTex: {
            auto& ti = mTextures[mCurrentTexIds[slot.index]];
            rv.push_back({ ti.texture ? ti.texture : fallbackTex });
            break;
        }
        case LLGLBindingType::Sampler:
        case LLGLBindingType::MaskSampler:
        case LLGLBindingType::BlendSampler: {
            auto& ti = mTextures[mCurrentTexIds[slot.index]];
            rv.push_back({ ti.sampler ? ti.sampler : fallbackSamp });
            break;
        }
        }
    }

    LLGL::ResourceHeapDescriptor rhd;
    rhd.pipelineLayout   = prog->pipelineLayout;
    rhd.numResourceViews = static_cast<uint32_t>(rv.size());
    prog->resourceHeap   = mRenderer->CreateResourceHeap(rhd, rv);
}

// ---- Draw ------------------------------------------------------------------
void GfxRenderingAPILLGL::DrawTriangles(float buf_vbo[], size_t buf_vbo_len,
                                          size_t buf_vbo_num_tris) {
    if (!mRenderer || !mCurrentProg || !mInRenderPass) return;

    const size_t bytes = buf_vbo_len * sizeof(float);
    EnsureVertexBufferSize(bytes);
    mRenderer->WriteBuffer(*mVertexBuffer, 0, buf_vbo, bytes);

    // Upload per-frame constants.
    EnsureConstantBuffers();
    int   fc = static_cast<int>(mFrameCount);
    float ns = mCurrentNoiseScale;
    mRenderer->WriteBuffer(*mFrameCountBuf, 0, &fc, sizeof(int));
    mRenderer->WriteBuffer(*mNoiseScaleBuf, 0, &ns, sizeof(float));

    mCmdBuf->SetVertexBuffer(*mVertexBuffer);
    mCmdBuf->SetPipelineState(*mCurrentProg->pso);

    // Rebuild resource heap (textures / samplers can change between draws).
    UpdateResourceHeap(mCurrentProg);
    if (mCurrentProg->resourceHeap)
        mCmdBuf->SetResourceHeap(*mCurrentProg->resourceHeap);

    mCmdBuf->SetViewport({ (float)mViewportX, (float)mViewportY,
                           (float)mViewportW, (float)mViewportH, 0.0f, 1.0f });
    mCmdBuf->SetScissor({ mScissorX, mScissorY, mScissorW, mScissorH });
    mCmdBuf->Draw(static_cast<uint32_t>(buf_vbo_num_tris * 3), 0);
}

// ---- Lifecycle -------------------------------------------------------------
void GfxRenderingAPILLGL::Init() {
    LLGL::Report report;
    for (const char* const* m = kPreferredModules; *m; m++) {
        auto ptr = LLGL::RenderSystem::Load(*m, &report);
        if (ptr) {
            mModuleName = *m;
            mRenderer   = ptr.get();
            auto* raw = ptr.release();
            mRendererLifetime = std::unique_ptr<void, void(*)(void*)>{
                raw, [](void* p) {
                    LLGL::RenderSystemPtr owner{
                        static_cast<LLGL::RenderSystem*>(p),
                        LLGL::RenderSystemDeleter{}
                    };
                    LLGL::RenderSystem::Unload(std::move(owner));
                }
            };
            break;
        }
        SPDLOG_WARN("[LLGL] Cannot load '{}': {}", *m,
                    report.GetText() ? report.GetText() : "(unknown)");
    }
    if (!mRenderer) { SPDLOG_ERROR("[LLGL] No renderer available."); return; }
    SPDLOG_INFO("[LLGL] Loaded: {}", mRenderer->GetName());

    LLGL::CommandBufferDescriptor cbDesc;
    cbDesc.flags = LLGL::CommandBufferFlags::ImmediateSubmit;
    mCmdBuf   = mRenderer->CreateCommandBuffer(cbDesc);
    mCmdQueue = mRenderer->GetCommandQueue();
    mFence    = mRenderer->CreateFence();

    EnsureConstantBuffers();
    mFBs.resize(1);
}

void GfxRenderingAPILLGL::OnResize()     {}
void GfxRenderingAPILLGL::StartFrame()   { mFrameCount++; }
void GfxRenderingAPILLGL::FinishRender() {}

void GfxRenderingAPILLGL::EndFrame() {
    if (mInRenderPass) {
        mCmdBuf->EndRenderPass();
        mCmdBuf->End();
        mCmdQueue->Submit(*mCmdBuf);
        mCmdQueue->WaitIdle();
        mInRenderPass = false;
    }
}

// ---- Framebuffers ----------------------------------------------------------
int GfxRenderingAPILLGL::CreateFramebuffer() {
    mFBs.push_back({});
    return static_cast<int>(mFBs.size() - 1);
}

void GfxRenderingAPILLGL::UpdateFramebufferParameters(int fb_id, uint32_t w, uint32_t h,
                                                         uint32_t, bool invertY,
                                                         bool, bool has_depth, bool) {
    if (!mRenderer || fb_id < 0 || fb_id >= (int)mFBs.size()) return;
    auto& fb = mFBs[fb_id];
    if (fb.width==w && fb.height==h && fb.invertY==invertY && fb.hasDepth==has_depth) return;

    if (fb.renderTarget) { mRenderer->Release(*fb.renderTarget); fb.renderTarget=nullptr; }
    if (fb.colorTex)     { mRenderer->Release(*fb.colorTex);     fb.colorTex=nullptr;     }
    if (fb.depthTex)     { mRenderer->Release(*fb.depthTex);     fb.depthTex=nullptr;     }

    fb.width=w; fb.height=h; fb.invertY=invertY; fb.hasDepth=has_depth;

    LLGL::TextureDescriptor cd;
    cd.type=LLGL::TextureType::Texture2D; cd.format=LLGL::Format::RGBA8UNorm;
    cd.extent={w,h,1};
    cd.bindFlags=LLGL::BindFlags::ColorAttachment|LLGL::BindFlags::Sampled|LLGL::BindFlags::CopySrc;
    cd.mipLevels=1;
    fb.colorTex = mRenderer->CreateTexture(cd);

    if (has_depth) {
        auto dd = cd;
        dd.format    = LLGL::Format::D32Float;
        dd.bindFlags = LLGL::BindFlags::DepthStencilAttachment;
        fb.depthTex  = mRenderer->CreateTexture(dd);
    }

    LLGL::RenderTargetDescriptor rt;
    rt.resolution = {w,h};
    rt.colorAttachments[0] = LLGL::AttachmentDescriptor{ fb.colorTex };
    if (has_depth) rt.depthStencilAttachment = LLGL::AttachmentDescriptor{ fb.depthTex };
    fb.renderTarget = mRenderer->CreateRenderTarget(rt);
}

void GfxRenderingAPILLGL::StartDrawToFramebuffer(int fbId, float noiseScale) {
    if (!mRenderer || fbId<0 || fbId>=(int)mFBs.size()) return;
    auto& fb = mFBs[fbId];
    if (!fb.renderTarget) return;

    if (mInRenderPass) {
        mCmdBuf->EndRenderPass(); mCmdBuf->End();
        mCmdQueue->Submit(*mCmdBuf); mCmdQueue->WaitIdle();
        mInRenderPass = false;
    }
    mCurrentFB = fbId;
    mCurrentNoiseScale = (noiseScale != 0.0f) ? (1.0f / noiseScale) : 1.0f;

    mCmdBuf->Begin();
    mCmdBuf->BeginRenderPass(*fb.renderTarget);
    mInRenderPass = true;
}

void GfxRenderingAPILLGL::CopyFramebuffer(int dst, int src,
                                            int sx0,int sy0,int sx1,int sy1,
                                            int dx0,int dy0,int,int) {
    if (!mRenderer || dst<0||dst>=(int)mFBs.size() || src<0||src>=(int)mFBs.size()) return;
    auto& s = mFBs[src]; auto& d = mFBs[dst];
    if (!s.colorTex || !d.colorTex) return;
    if (mInRenderPass) { mCmdBuf->EndRenderPass(); mInRenderPass=false; }
    LLGL::TextureLocation sl{{ sx0, sy0, 0 }, 0, 0};
    LLGL::TextureLocation dl{{ dx0, dy0, 0 }, 0, 0};
    LLGL::Extent3D ext{ (uint32_t)(sx1-sx0), (uint32_t)(sy1-sy0), 1u };
    mCmdBuf->CopyTexture(*d.colorTex, dl, *s.colorTex, sl, ext);
}

void GfxRenderingAPILLGL::ClearFramebuffer(bool color, bool depth) {
    if (!mRenderer || !mInRenderPass) return;
    uint32_t flags = 0;
    if (color) flags |= LLGL::ClearFlags::Color;
    if (depth) flags |= LLGL::ClearFlags::Depth;
    mCmdBuf->Clear(flags, LLGL::ClearValue{ 0.0f,0.0f,0.0f,1.0f,1.0f });
}

void GfxRenderingAPILLGL::ReadFramebufferToCPU(int fbId, uint32_t w, uint32_t h,
                                                  uint16_t* rgba16Buf) {
    if (!mRenderer || fbId<0||fbId>=(int)mFBs.size()) return;
    auto& fb = mFBs[fbId];
    if (!fb.colorTex) return;
    if (mInRenderPass) {
        mCmdBuf->EndRenderPass(); mCmdBuf->End();
        mCmdQueue->Submit(*mCmdBuf); mCmdQueue->WaitIdle();
        mInRenderPass = false;
    }
    std::vector<uint8_t> rgba8(w*h*4);
    LLGL::MutableImageView iv{ LLGL::ImageFormat::RGBA, LLGL::DataType::UInt8, rgba8.data(), rgba8.size() };
    LLGL::TextureRegion r; r.offset={0,0,0}; r.extent={w,h,1};
    mRenderer->ReadTexture(*fb.colorTex, r, iv);
    for (uint32_t i=0; i<w*h; i++) {
        uint8_t rv = (rgba8[i*4+0]>>3)&0x1F;
        uint8_t gv = (rgba8[i*4+1]>>3)&0x1F;
        uint8_t bv = (rgba8[i*4+2]>>3)&0x1F;
        uint8_t av = rgba8[i*4+3] ? 1 : 0;
        rgba16Buf[i] = (rv<<11)|(gv<<6)|(bv<<1)|av;
    }
}

void GfxRenderingAPILLGL::ResolveMSAAColorBuffer(int, int) {}

std::unordered_map<std::pair<float,float>, uint16_t, hash_pair_ff>
GfxRenderingAPILLGL::GetPixelDepth(int, const std::set<std::pair<float,float>>&) { return {}; }

void* GfxRenderingAPILLGL::GetFramebufferTextureId(int fbId) {
    return (fbId>=0 && fbId<(int)mFBs.size()) ? mFBs[fbId].colorTex : nullptr;
}
void GfxRenderingAPILLGL::SelectTextureFb(int fbId) {
    if (fbId>=0 && fbId<(int)mFBs.size())
        mCurrentTexIds[mCurrentTile] = static_cast<uint32_t>(fbId);
}

void          GfxRenderingAPILLGL::SetTextureFilter(FilteringMode m)  { mFilterMode = m; }
FilteringMode GfxRenderingAPILLGL::GetTextureFilter()                 { return mFilterMode; }
void          GfxRenderingAPILLGL::SetSrgbMode()                      { mSrgbMode = true; }
ImTextureID   GfxRenderingAPILLGL::GetTextureById(int id) {
    return (id>=0 && id<(int)mTextures.size()) ? mTextures[id].texture : nullptr;
}

} // namespace Fast
#endif // ENABLE_LLGL
