#include "ship/window/Window.h"
#ifdef ENABLE_LLGL

// ============================================================
// IMPORTANT: Include prism *before* LLGL so that X11 headers
// (pulled in by LLGL's Linux OpenGL/GLX back-end) do not yet
// define "True 1" / "False 0" macros that collide with the
// prism::lexer::TokenType enum values.
// ============================================================
#include "fast/backends/gfx_llgl.h"
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

// ---------------------------------------------------------------------------
// Shared GLSL shader generation helpers (prism-based, same as OpenGL backend)
// ---------------------------------------------------------------------------
#include "gfx_glsl_helpers.h"

namespace {

// Use prism's own InvokeFunc typedef (typedef uintptr_t (*InvokeFunc)(); from
// prism/utils/invoke.h).  Do NOT redeclare a local alias — it would conflict
// with the global one and cause an "ambiguous" error in clang/GCC.

static size_t sNumFloats = 0;
static prism::ContextTypes* UpdateFloatsLLGL(prism::ContextTypes* _, prism::ContextTypes* num) {
    sNumFloats += std::get<int>(*num);
    return nullptr;
}

static std::optional<std::string> LlglIncludeFs(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type       = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder  = Ship::Endianness::Native;
    init->Format     = RESOURCE_FORMAT_BINARY;
    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));
    if (!res) return std::nullopt;
    return *static_cast<std::string*>(res->GetRawPointer());
}

} // anonymous namespace

namespace Fast {

const char* const GfxRenderingAPILLGL::kPreferredModules[] = {
#ifdef _WIN32
    "Direct3D11",
#elif defined(__APPLE__)
    "Metal",
#endif
    "OpenGL",
    nullptr
};

// ---------------------------------------------------------------------------
// Shader generation helpers
// ---------------------------------------------------------------------------
std::string GfxRenderingAPILLGL::BuildFragShader(const CCFeatures& cc) const {
    prism::Processor proc;
    prism::ContextItems ctx = {
        { "VERTEX_SHADER", false },
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
        { "FILTER_THREE_POINT", FILTER_THREE_POINT },
        { "FILTER_LINEAR",  FILTER_LINEAR },
        { "FILTER_NONE",    FILTER_NONE },
        { "srgb_mode",      mSrgbMode },
        { "SHADER_0",       SHADER_0 },
        { "SHADER_INPUT_1", SHADER_INPUT_1 },
        { "SHADER_INPUT_2", SHADER_INPUT_2 },
        { "SHADER_INPUT_3", SHADER_INPUT_3 },
        { "SHADER_INPUT_4", SHADER_INPUT_4 },
        { "SHADER_INPUT_5", SHADER_INPUT_5 },
        { "SHADER_INPUT_6", SHADER_INPUT_6 },
        { "SHADER_INPUT_7", SHADER_INPUT_7 },
        { "SHADER_TEXEL0",  SHADER_TEXEL0 },
        { "SHADER_TEXEL0A", SHADER_TEXEL0A },
        { "SHADER_TEXEL1",  SHADER_TEXEL1 },
        { "SHADER_TEXEL1A", SHADER_TEXEL1A },
        { "SHADER_1",       SHADER_1 },
        { "SHADER_COMBINED",SHADER_COMBINED },
        { "SHADER_NOISE",   SHADER_NOISE },
        { "o_three_point_filtering", mFilterMode == FILTER_THREE_POINT },
        { "append_formula", (InvokeFunc)gfx_append_formula },
        // Use modern GLSL core profile for LLGL
        { "GLSL_VERSION",   "#version 330 core" },
        { "attr",           "in" },
        { "opengles",       false },
        { "core_opengl",    true },
        { "texture",        "texture" },
        { "vOutColor",      "vOutColor" },
    };
    proc.populate(ctx);

    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type      = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format    = RESOURCE_FORMAT_BINARY;
    const char* shaderName = gfx_get_shader(cc.shader_id);
    std::string path = "shaders/opengl/default.shader.glsl";
    if (shaderName) path = std::string(shaderName) + ".glsl";

    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));
    if (!res) {
        SPDLOG_ERROR("[LLGL] Failed to load fragment shader template");
        return {};
    }
    proc.load(*static_cast<std::string*>(res->GetRawPointer()));
    proc.bind_include_loader(LlglIncludeFs);
    return proc.process();
}

std::string GfxRenderingAPILLGL::BuildVertShader(const CCFeatures& cc, size_t& outNumFloats,
                                                   std::vector<LLGLShaderProgram::AttribInfo>& outAttribs) const {
    sNumFloats = 4; // position vec4 always present
    prism::Processor proc;
    prism::ContextItems ctx = {
        { "VERTEX_SHADER", true },
        { "o_textures",   M_ARRAY(cc.usedTextures, bool, 2) },
        { "o_clamp",      M_ARRAY(cc.clamp, bool, 2, 2) },
        { "o_fog",        cc.opt_fog },
        { "o_grayscale",  cc.opt_grayscale },
        { "o_alpha",      cc.opt_alpha },
        { "o_inputs",     cc.numInputs },
        { "update_floats",(InvokeFunc)UpdateFloatsLLGL },
        { "GLSL_VERSION", "#version 330 core" },
        { "attr",         "in" },
        { "out",          "out" },
        { "opengles",     false },
    };
    proc.populate(ctx);

    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type      = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format    = RESOURCE_FORMAT_BINARY;
    const char* shaderName = gfx_get_shader(cc.shader_id);
    std::string path = "shaders/opengl/default.shader.glsl";
    if (shaderName) path = std::string(shaderName) + ".glsl";

    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));
    if (!res) {
        SPDLOG_ERROR("[LLGL] Failed to load vertex shader template");
        return {};
    }
    proc.load(*static_cast<std::string*>(res->GetRawPointer()));
    proc.bind_include_loader(LlglIncludeFs);
    auto result = proc.process();
    outNumFloats = sNumFloats;

    // Build attrib info from the cc_features (mirrors OpenGL VertexArraySetAttribs)
    outAttribs.clear();
    uint32_t loc = 0;
    uint8_t  off = 0;

    auto addAttrib = [&](uint8_t sz) {
        outAttribs.push_back({ off, sz, loc++ });
        off += sz;
    };

    addAttrib(4); // aVtxPos (vec4)
    for (int i = 0; i < 2; i++) {
        if (cc.usedTextures[i]) {
            addAttrib(2); // aTexCoordN (vec2)
            if (cc.clamp[i][0]) addAttrib(1); // aTexClampSN (float)
            if (cc.clamp[i][1]) addAttrib(1); // aTexClampTN (float)
        }
    }
    if (cc.opt_fog)       addAttrib(4); // aFog (vec4)
    if (cc.opt_grayscale) addAttrib(4); // aGrayscaleColor (vec4)
    for (int i = 0; i < cc.numInputs; i++)
        addAttrib(cc.opt_alpha ? 4 : 3); // aInputN

    return result;
}

// ---------------------------------------------------------------------------
// PSO construction
// ---------------------------------------------------------------------------
bool GfxRenderingAPILLGL::BuildPipelineState(LLGLShaderProgram* prog,
                                              const std::string& vsGlsl,
                                              const std::string& fsGlsl) {
    if (!mRenderer) return false;

    // --- Vertex shader ---
    LLGL::ShaderDescriptor vsDesc;
    vsDesc.type         = LLGL::ShaderType::Vertex;
    vsDesc.source       = vsGlsl.c_str();
    vsDesc.sourceSize   = vsGlsl.size();
    vsDesc.sourceType   = LLGL::ShaderSourceType::CodeString;
    vsDesc.entryPoint   = "main";
    // Vertex attributes (name + location + stride)
    size_t stride = prog->numFloats * sizeof(float);
    size_t byteOff = 0;
    for (auto& a : prog->attribs) {
        LLGL::VertexAttribute va;
        va.location = a.location;
        va.offset   = static_cast<uint32_t>(a.offset * sizeof(float));
        va.stride   = static_cast<uint32_t>(stride);
        switch (a.size) {
            case 1: va.format = LLGL::Format::R32Float;    break;
            case 2: va.format = LLGL::Format::RG32Float;   break;
            case 3: va.format = LLGL::Format::RGB32Float;  break;
            default: va.format = LLGL::Format::RGBA32Float; break;
        }
        vsDesc.vertex.inputAttribs.push_back(va);
    }
    auto* vs = mRenderer->CreateShader(vsDesc);
    if (vs->GetReport() && vs->GetReport()->HasErrors()) {
        SPDLOG_ERROR("[LLGL] Vertex shader compilation error: {}", vs->GetReport()->GetText());
        mRenderer->Release(*vs);
        return false;
    }

    // --- Fragment shader ---
    LLGL::ShaderDescriptor fsDesc;
    fsDesc.type       = LLGL::ShaderType::Fragment;
    fsDesc.source     = fsGlsl.c_str();
    fsDesc.sourceSize = fsGlsl.size();
    fsDesc.sourceType = LLGL::ShaderSourceType::CodeString;
    fsDesc.entryPoint = "main";
    // Fragment output
    LLGL::FragmentAttribute fragOut;
    fragOut.name     = "vOutColor";
    fragOut.location = 0;
    fragOut.format   = LLGL::Format::RGBA8UNorm;
    fsDesc.fragment.outputAttribs.push_back(fragOut);
    auto* fs = mRenderer->CreateShader(fsDesc);
    if (fs->GetReport() && fs->GetReport()->HasErrors()) {
        SPDLOG_ERROR("[LLGL] Fragment shader compilation error: {}", fs->GetReport()->GetText());
        mRenderer->Release(*vs);
        mRenderer->Release(*fs);
        return false;
    }

    // --- Pipeline layout ---
    LLGL::PipelineLayoutDescriptor layoutDesc;
    if (prog->usedTextures[0] || prog->usedTextures[1]) {
        // sampler2D at binding 0 and 1
        LLGL::BindingDescriptor bd0;
        bd0.type       = LLGL::ResourceType::Texture;
        bd0.bindFlags  = LLGL::BindFlags::Sampled;
        bd0.stageFlags = LLGL::StageFlags::FragmentStage;
        bd0.slot       = LLGL::BindingSlot{ 0 };
        layoutDesc.bindings.push_back(bd0);

        LLGL::BindingDescriptor bd1 = bd0;
        bd1.slot = LLGL::BindingSlot{ 1 };
        layoutDesc.bindings.push_back(bd1);

        // Samplers
        LLGL::BindingDescriptor sd0;
        sd0.type       = LLGL::ResourceType::Sampler;
        sd0.stageFlags = LLGL::StageFlags::FragmentStage;
        sd0.slot       = LLGL::BindingSlot{ 0 };
        layoutDesc.bindings.push_back(sd0);
        LLGL::BindingDescriptor sd1 = sd0;
        sd1.slot = LLGL::BindingSlot{ 1 };
        layoutDesc.bindings.push_back(sd1);
    }
    auto* layout = mRenderer->CreatePipelineLayout(layoutDesc);
    prog->pipelineLayout = layout;

    // --- Blend state ---
    LLGL::BlendDescriptor blend;
    blend.targets[0].blendEnabled = prog->hasAlpha;
    blend.targets[0].srcColor     = LLGL::BlendOp::SrcAlpha;
    blend.targets[0].dstColor     = LLGL::BlendOp::InvSrcAlpha;
    blend.targets[0].srcAlpha     = LLGL::BlendOp::One;
    blend.targets[0].dstAlpha     = LLGL::BlendOp::Zero;

    // --- Graphics pipeline ---
    LLGL::GraphicsPipelineDescriptor psoDesc;
    psoDesc.pipelineLayout    = layout;
    psoDesc.vertexShader      = vs;
    psoDesc.fragmentShader    = fs;
    psoDesc.primitiveTopology = LLGL::PrimitiveTopology::TriangleList;
    psoDesc.blend             = blend;

    // Depth (enabled per draw via SetDepthTestAndMask — rebuild PSO per change
    // would be expensive; we accept a forward-looking approach here).
    psoDesc.depth.testEnabled  = mPendingDepthTest;
    psoDesc.depth.writeEnabled = mPendingDepthMask;

    LLGL::Report report;
    auto* pso = mRenderer->CreatePipelineState(psoDesc);
    if (pso && pso->GetReport() && pso->GetReport()->HasErrors()) {
        SPDLOG_ERROR("[LLGL] PSO creation error: {}", pso->GetReport()->GetText());
        mRenderer->Release(*pso);
        mRenderer->Release(*vs);
        mRenderer->Release(*fs);
        mRenderer->Release(*layout);
        return false;
    }
    if (!pso) {
        const auto* rsReport = mRenderer->GetReport();
        SPDLOG_ERROR("[LLGL] PSO creation failed: {}",
                     rsReport ? rsReport->GetText() : "(no report)");
        mRenderer->Release(*vs);
        mRenderer->Release(*fs);
        mRenderer->Release(*layout);
        return false;
    }
    prog->pso = pso;

    // Shaders can be released after PSO creation
    mRenderer->Release(*vs);
    mRenderer->Release(*fs);
    return true;
}

// ---------------------------------------------------------------------------
// GfxRenderingAPI implementation
// ---------------------------------------------------------------------------
GfxRenderingAPILLGL::~GfxRenderingAPILLGL() {
    if (!mRenderer) return;

    if (mCmdBuf)      mRenderer->Release(*mCmdBuf);
    if (mFence)       mRenderer->Release(*mFence);
    if (mVertexBuffer) mRenderer->Release(*mVertexBuffer);

    for (auto& fb : mFBs) {
        if (fb.renderTarget) mRenderer->Release(*fb.renderTarget);
        if (fb.colorTex)     mRenderer->Release(*fb.colorTex);
        if (fb.depthTex)     mRenderer->Release(*fb.depthTex);
    }
    for (auto& ti : mTextures) {
        if (ti.texture) mRenderer->Release(*ti.texture);
    }
    for (auto& kv : mSamplerCache) {
        if (kv.second) mRenderer->Release(*kv.second);
    }
    for (auto& kv : mShaderCache) {
        auto& p = kv.second;
        if (p.pso)            mRenderer->Release(*p.pso);
        if (p.pipelineLayout) mRenderer->Release(*p.pipelineLayout);
        if (p.resourceHeap)   mRenderer->Release(*p.resourceHeap);
    }
}

const char* GfxRenderingAPILLGL::GetName() {
    return mModuleName.empty() ? "LLGL" : mModuleName.c_str();
}

int GfxRenderingAPILLGL::GetMaxTextureSize() { return 8192; }

GfxClipParameters GfxRenderingAPILLGL::GetClipParameters() {
    bool invertY = !mFBs.empty() && mFBs[mCurrentFB].invertY;
    return { false, invertY };
}

// ---------- Shader ----------
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

    auto key = std::make_pair(id0, id1);
    auto& prog = mShaderCache[key];

    std::vector<LLGLShaderProgram::AttribInfo> attribs;
    size_t numFloats = 4;
    auto vs = BuildVertShader(cc, numFloats, attribs);
    auto fs = BuildFragShader(cc);
    if (vs.empty() || fs.empty()) return nullptr;

    prog.numFloats   = static_cast<uint8_t>(numFloats);
    prog.numInputs   = static_cast<uint8_t>(cc.numInputs);
    prog.usedTextures[0] = cc.usedTextures[0];
    prog.usedTextures[1] = cc.usedTextures[1];
    prog.hasAlpha    = cc.opt_alpha;
    prog.attribs     = std::move(attribs);

    if (!BuildPipelineState(&prog, vs, fs)) {
        mShaderCache.erase(key);
        return nullptr;
    }

    mCurrentProg = &prog;
    return reinterpret_cast<ShaderProgram*>(&prog);
}

ShaderProgram* GfxRenderingAPILLGL::LookupShader(uint64_t id0, uint64_t id1) {
    auto it = mShaderCache.find({ id0, id1 });
    if (it == mShaderCache.end()) return nullptr;
    return reinterpret_cast<ShaderProgram*>(&it->second);
}

void GfxRenderingAPILLGL::ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) {
    auto* p = reinterpret_cast<LLGLShaderProgram*>(prg);
    *numInputs = p->numInputs;
    usedTextures[0] = p->usedTextures[0];
    usedTextures[1] = p->usedTextures[1];
}

// ---------- Textures ----------
uint32_t GfxRenderingAPILLGL::NewTexture() {
    mTextures.push_back({});
    return static_cast<uint32_t>(mTextures.size() - 1);
}

void GfxRenderingAPILLGL::SelectTexture(int tile, uint32_t textureId) {
    mCurrentTile = tile;
    mCurrentTexIds[tile] = textureId;
}

void GfxRenderingAPILLGL::UploadTexture(const uint8_t* rgba32Buf, uint32_t w, uint32_t h) {
    if (!mRenderer) return;
    auto& ti = mTextures[mCurrentTexIds[mCurrentTile]];

    // Release previous texture if dimensions changed
    if (ti.texture && (ti.width != w || ti.height != h)) {
        mRenderer->Release(*ti.texture);
        ti.texture = nullptr;
    }

    LLGL::TextureDescriptor texDesc;
    texDesc.type      = LLGL::TextureType::Texture2D;
    texDesc.format    = LLGL::Format::RGBA8UNorm;
    texDesc.extent    = { w, h, 1 };
    texDesc.bindFlags = LLGL::BindFlags::Sampled;
    texDesc.mipLevels = 1;

    LLGL::ImageView imgView;
    imgView.format   = LLGL::ImageFormat::RGBA;
    imgView.dataType = LLGL::DataType::UInt8;
    imgView.data     = rgba32Buf;
    imgView.dataSize = w * h * 4;

    if (ti.texture) {
        // Update existing texture
        LLGL::TextureRegion region;
        region.subresource.baseMipLevel   = 0;
        region.subresource.numMipLevels   = 1;
        region.subresource.baseArrayLayer = 0;
        region.subresource.numArrayLayers = 1;
        region.offset = { 0, 0, 0 };
        region.extent = { w, h, 1 };
        mRenderer->WriteTexture(*ti.texture, region, imgView);
    } else {
        ti.texture = mRenderer->CreateTexture(texDesc, &imgView);
        ti.width   = static_cast<uint16_t>(w);
        ti.height  = static_cast<uint16_t>(h);
    }
}

void GfxRenderingAPILLGL::SetSamplerParameters(int sampler, bool linearFilter,
                                                 uint32_t cms, uint32_t cmt) {
    auto* samp = GetOrCreateSampler(linearFilter, cms, cmt);
    mTextures[mCurrentTexIds[sampler]].sampler = samp;
}

void GfxRenderingAPILLGL::DeleteTexture(uint32_t texId) {
    if (!mRenderer || texId >= mTextures.size()) return;
    auto& ti = mTextures[texId];
    if (ti.texture) { mRenderer->Release(*ti.texture); ti.texture = nullptr; }
    ti = {};
}

LLGL::Sampler* GfxRenderingAPILLGL::GetOrCreateSampler(bool linearFilter,
                                                         uint32_t cms, uint32_t cmt) {
    SamplerKey key{ linearFilter, cms, cmt };
    auto it = mSamplerCache.find(key);
    if (it != mSamplerCache.end()) return it->second;

    auto toAddressMode = [](uint32_t cm) -> LLGL::SamplerAddressMode {
        return (cm & 1) ? LLGL::SamplerAddressMode::Clamp
                        : LLGL::SamplerAddressMode::Repeat;
    };
    LLGL::SamplerDescriptor sd;
    sd.minFilter  = linearFilter ? LLGL::SamplerFilter::Linear : LLGL::SamplerFilter::Nearest;
    sd.magFilter  = linearFilter ? LLGL::SamplerFilter::Linear : LLGL::SamplerFilter::Nearest;
    sd.mipMapFilter = LLGL::SamplerFilter::Nearest;
    sd.addressModeU = toAddressMode(cms);
    sd.addressModeV = toAddressMode(cmt);
    sd.addressModeW = LLGL::SamplerAddressMode::Repeat;
    sd.maxAnisotropy = 1;
    auto* samp = mRenderer->CreateSampler(sd);
    mSamplerCache[key] = samp;
    return samp;
}

// ---------- Render state ----------
void GfxRenderingAPILLGL::SetDepthTestAndMask(bool dt, bool dm) {
    mPendingDepthTest = dt;
    mPendingDepthMask = dm;
}
void GfxRenderingAPILLGL::SetZmodeDecal(bool d) { mPendingDecal = d; }
void GfxRenderingAPILLGL::SetViewport(int x, int y, int w, int h) {
    mViewportX = x; mViewportY = y; mViewportW = w; mViewportH = h;
}
void GfxRenderingAPILLGL::SetScissor(int x, int y, int w, int h) {
    mScissorX = x; mScissorY = y; mScissorW = w; mScissorH = h;
}
void GfxRenderingAPILLGL::SetUseAlpha(bool a) { mPendingAlpha = a; }

// ---------- Vertex buffer ----------
void GfxRenderingAPILLGL::EnsureVertexBufferSize(size_t bytes) {
    if (bytes <= mVertexBufferBytes) return;
    if (mVertexBuffer) mRenderer->Release(*mVertexBuffer);

    LLGL::BufferDescriptor bd;
    bd.size      = static_cast<uint64_t>(bytes * 2); // over-allocate to reduce rebuilds
    bd.bindFlags = LLGL::BindFlags::VertexBuffer;
    bd.cpuAccessFlags = LLGL::CPUAccessFlags::Write;
    mVertexBuffer      = mRenderer->CreateBuffer(bd);
    mVertexBufferBytes = bytes * 2;
}

// ---------- Draw ----------
void GfxRenderingAPILLGL::DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    if (!mRenderer || !mCurrentProg || !mInRenderPass) return;

    const size_t bytes = buf_vbo_len * sizeof(float);
    EnsureVertexBufferSize(bytes);
    mRenderer->WriteBuffer(*mVertexBuffer, 0, buf_vbo, bytes);

    mCmdBuf->SetVertexBuffer(*mVertexBuffer);
    mCmdBuf->SetPipelineState(*mCurrentProg->pso);

    // Update resource heap for current textures
    if (mCurrentProg->usedTextures[0] || mCurrentProg->usedTextures[1]) {
        // Build resource heap per draw — rebuild only on change (simplified: rebuild every draw)
        if (mCurrentProg->resourceHeap) {
            mRenderer->Release(*mCurrentProg->resourceHeap);
            mCurrentProg->resourceHeap = nullptr;
        }
        LLGL::ResourceHeapDescriptor rhd;
        rhd.pipelineLayout  = mCurrentProg->pipelineLayout;
        rhd.numResourceViews = 4; // 2 textures + 2 samplers
        std::vector<LLGL::ResourceViewDescriptor> rvDescs;
        // Textures
        for (int t = 0; t < 2; t++) {
            auto& ti = mTextures[mCurrentTexIds[t]];
            LLGL::ResourceViewDescriptor rvd;
            rvd.resource = ti.texture ? ti.texture : mTextures[0].texture;
            rvDescs.push_back(rvd);
        }
        // Samplers
        for (int t = 0; t < 2; t++) {
            auto& ti = mTextures[mCurrentTexIds[t]];
            LLGL::ResourceViewDescriptor rvd;
            rvd.resource = ti.sampler ? ti.sampler
                                       : GetOrCreateSampler(false, 0, 0);
            rvDescs.push_back(rvd);
        }
        mCurrentProg->resourceHeap = mRenderer->CreateResourceHeap(rhd, rvDescs);
        mCmdBuf->SetResourceHeap(*mCurrentProg->resourceHeap);
    }

    // Viewport and scissor
    LLGL::Viewport vp{
        (float)mViewportX, (float)mViewportY,
        (float)mViewportW, (float)mViewportH,
        0.0f, 1.0f
    };
    mCmdBuf->SetViewport(vp);
    LLGL::Scissor sc{ mScissorX, mScissorY, mScissorW, mScissorH };
    mCmdBuf->SetScissor(sc);

    mCmdBuf->Draw(static_cast<uint32_t>(buf_vbo_num_tris * 3), 0);
}

// ---------- Lifecycle ----------
void GfxRenderingAPILLGL::Init() {
    LLGL::Report report;
    for (const char* const* m = kPreferredModules; *m; m++) {
        auto ptr = LLGL::RenderSystem::Load(*m, &report);
        if (ptr) {
            mModuleName = *m;
            mRenderer   = ptr.get();
            // Store lifetime ownership as void* unique_ptr.
            // With LLGL_BUILD_STATIC_LIB the deleter just calls delete; for
            // dynamic builds we use the proper Unload path.
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
        SPDLOG_WARN("[LLGL] Cannot load '{}' renderer: {}", *m,
                    report.GetText() ? report.GetText() : "(unknown)");
    }
    if (!mRenderer) {
        SPDLOG_ERROR("[LLGL] No renderer backend could be initialised.");
        return;
    }
    SPDLOG_INFO("[LLGL] Loaded renderer: {}", mRenderer->GetName());

    LLGL::CommandBufferDescriptor cbDesc;
    cbDesc.flags = LLGL::CommandBufferFlags::ImmediateSubmit;
    mCmdBuf    = mRenderer->CreateCommandBuffer(cbDesc);
    mCmdQueue  = mRenderer->GetCommandQueue();
    mFence     = mRenderer->CreateFence();

    // Default framebuffer slot
    mFBs.resize(1);
}

void GfxRenderingAPILLGL::OnResize() {}

void GfxRenderingAPILLGL::StartFrame() {
    mFrameCount++;
}

void GfxRenderingAPILLGL::EndFrame() {
    if (mInRenderPass) {
        mCmdBuf->EndRenderPass();
        mCmdBuf->End();
        mCmdQueue->Submit(*mCmdBuf);
        mCmdQueue->WaitIdle();
        mInRenderPass = false;
    }
}

void GfxRenderingAPILLGL::FinishRender() {}

// ---------- Framebuffers ----------
int GfxRenderingAPILLGL::CreateFramebuffer() {
    mFBs.push_back({});
    return static_cast<int>(mFBs.size() - 1);
}

void GfxRenderingAPILLGL::UpdateFramebufferParameters(int fb_id, uint32_t w, uint32_t h,
                                                        uint32_t /*msaa*/, bool invertY,
                                                        bool /*render_target*/,
                                                        bool has_depth, bool /*can_depth*/) {
    if (!mRenderer || fb_id < 0 || fb_id >= (int)mFBs.size()) return;
    auto& fb = mFBs[fb_id];

    if (fb.width == w && fb.height == h && fb.invertY == invertY &&
        fb.hasDepth == has_depth)
        return;

    // Release old resources
    if (fb.renderTarget) { mRenderer->Release(*fb.renderTarget); fb.renderTarget = nullptr; }
    if (fb.colorTex)     { mRenderer->Release(*fb.colorTex); fb.colorTex = nullptr; }
    if (fb.depthTex)     { mRenderer->Release(*fb.depthTex); fb.depthTex = nullptr; }

    fb.width    = w;
    fb.height   = h;
    fb.invertY  = invertY;
    fb.hasDepth = has_depth;

    // Color texture
    LLGL::TextureDescriptor colorDesc;
    colorDesc.type      = LLGL::TextureType::Texture2D;
    colorDesc.format    = LLGL::Format::RGBA8UNorm;
    colorDesc.extent    = { w, h, 1 };
    colorDesc.bindFlags = LLGL::BindFlags::ColorAttachment | LLGL::BindFlags::Sampled
                        | LLGL::BindFlags::CopySrc;
    colorDesc.mipLevels = 1;
    fb.colorTex = mRenderer->CreateTexture(colorDesc);

    // Depth texture (optional)
    if (has_depth) {
        LLGL::TextureDescriptor depthDesc = colorDesc;
        depthDesc.format    = LLGL::Format::D32Float;
        depthDesc.bindFlags = LLGL::BindFlags::DepthStencilAttachment;
        fb.depthTex = mRenderer->CreateTexture(depthDesc);
    }

    // Render target
    LLGL::RenderTargetDescriptor rtDesc;
    rtDesc.resolution = { w, h };
    rtDesc.colorAttachments[0] = LLGL::AttachmentDescriptor{ fb.colorTex };
    if (has_depth)
        rtDesc.depthStencilAttachment = LLGL::AttachmentDescriptor{ fb.depthTex };
    fb.renderTarget = mRenderer->CreateRenderTarget(rtDesc);
}

void GfxRenderingAPILLGL::StartDrawToFramebuffer(int fbId, float noiseScale) {
    if (!mRenderer || fbId < 0 || fbId >= (int)mFBs.size()) return;
    auto& fb = mFBs[fbId];
    if (!fb.renderTarget) return;

    if (mInRenderPass) {
        mCmdBuf->EndRenderPass();
        mCmdBuf->End();
        mCmdQueue->Submit(*mCmdBuf);
        mCmdQueue->WaitIdle();
        mInRenderPass = false;
    }

    mCurrentFB = fbId;
    mCurrentNoiseScale = noiseScale;

    mCmdBuf->Begin();
    mCmdBuf->BeginRenderPass(*fb.renderTarget);
    mInRenderPass = true;
}

void GfxRenderingAPILLGL::CopyFramebuffer(int fbDst, int fbSrc,
                                           int srcX0, int srcY0, int srcX1, int srcY1,
                                           int dstX0, int dstY0, int dstX1, int dstY1) {
    if (!mRenderer) return;
    if (fbDst < 0 || fbDst >= (int)mFBs.size()) return;
    if (fbSrc < 0 || fbSrc >= (int)mFBs.size()) return;
    auto& src = mFBs[fbSrc];
    auto& dst = mFBs[fbDst];
    if (!src.colorTex || !dst.colorTex) return;

    if (mInRenderPass) {
        mCmdBuf->EndRenderPass();
        mInRenderPass = false;
    }
    LLGL::TextureLocation srcLoc{
        LLGL::Offset3D{ srcX0, srcY0, 0 }, 0, 0
    };
    LLGL::TextureLocation dstLoc{
        LLGL::Offset3D{ dstX0, dstY0, 0 }, 0, 0
    };
    LLGL::Extent3D ext{
        static_cast<uint32_t>(srcX1 - srcX0),
        static_cast<uint32_t>(srcY1 - srcY0),
        1u
    };
    mCmdBuf->CopyTexture(*dst.colorTex, dstLoc, *src.colorTex, srcLoc, ext);
}

void GfxRenderingAPILLGL::ClearFramebuffer(bool color, bool depth) {
    if (!mRenderer || !mInRenderPass) return;
    uint32_t flags = 0;
    if (color) flags |= LLGL::ClearFlags::Color;
    if (depth) flags |= LLGL::ClearFlags::Depth;
    LLGL::ClearValue cv{ 0.0f, 0.0f, 0.0f, 1.0f, 1.0f };
    mCmdBuf->Clear(flags, cv);
}

void GfxRenderingAPILLGL::ReadFramebufferToCPU(int fbId, uint32_t w, uint32_t h,
                                                 uint16_t* rgba16Buf) {
    if (!mRenderer || fbId < 0 || fbId >= (int)mFBs.size()) return;
    auto& fb = mFBs[fbId];
    if (!fb.colorTex) return;

    // Finish any in-progress render pass
    if (mInRenderPass) {
        mCmdBuf->EndRenderPass();
        mCmdBuf->End();
        mCmdQueue->Submit(*mCmdBuf);
        mCmdQueue->WaitIdle();
        mInRenderPass = false;
    }

    // Read back as RGBA8 then convert to RGBA16 (5-5-5-1)
    std::vector<uint8_t> rgba8(w * h * 4);
    LLGL::MutableImageView imgView{
        LLGL::ImageFormat::RGBA, LLGL::DataType::UInt8,
        rgba8.data(), rgba8.size()
    };
    LLGL::TextureRegion region;
    region.offset = { 0, 0, 0 };
    region.extent = { w, h, 1 };
    mRenderer->ReadTexture(*fb.colorTex, region, imgView);

    for (uint32_t i = 0; i < w * h; i++) {
        uint8_t r = (rgba8[i*4+0] >> 3) & 0x1F;
        uint8_t g = (rgba8[i*4+1] >> 3) & 0x1F;
        uint8_t b = (rgba8[i*4+2] >> 3) & 0x1F;
        uint8_t a = rgba8[i*4+3] ? 1 : 0;
        rgba16Buf[i] = (r << 11) | (g << 6) | (b << 1) | a;
    }
}

void GfxRenderingAPILLGL::ResolveMSAAColorBuffer(int, int) {}

std::unordered_map<std::pair<float,float>, uint16_t, hash_pair_ff>
GfxRenderingAPILLGL::GetPixelDepth(int, const std::set<std::pair<float,float>>&) { return {}; }

void* GfxRenderingAPILLGL::GetFramebufferTextureId(int fbId) {
    if (fbId < 0 || fbId >= (int)mFBs.size()) return nullptr;
    return mFBs[fbId].colorTex;
}

void GfxRenderingAPILLGL::SelectTextureFb(int fbId) {
    if (fbId < 0 || fbId >= (int)mFBs.size()) return;
    mCurrentTexIds[mCurrentTile] = static_cast<uint32_t>(fbId);
}

void GfxRenderingAPILLGL::SetTextureFilter(FilteringMode mode) { mFilterMode = mode; }
FilteringMode GfxRenderingAPILLGL::GetTextureFilter() { return mFilterMode; }
void GfxRenderingAPILLGL::SetSrgbMode() { mSrgbMode = true; }
ImTextureID GfxRenderingAPILLGL::GetTextureById(int id) {
    if (id < 0 || id >= (int)mTextures.size()) return nullptr;
    return mTextures[id].texture;
}

} // namespace Fast
#endif // ENABLE_LLGL
