// Null / recording Fast3D backends — implementation.
// See include/fast/backends/gfx_null.h for documentation.

#include "fast/backends/gfx_null.h"

#include <algorithm>
#include <cstring>

namespace Fast {

// ---------------------------------------------------------------------------
// Helpers — opaque ShaderProgram* ↔ ShaderEntry* conversion
// ---------------------------------------------------------------------------

ShaderProgram* RecordingRenderingAPI::EntryToSP(ShaderEntry* e) {
    return reinterpret_cast<ShaderProgram*>(e);
}

RecordingRenderingAPI::ShaderEntry* RecordingRenderingAPI::SPToEntry(ShaderProgram* prg) {
    return reinterpret_cast<ShaderEntry*>(prg);
}

// ---------------------------------------------------------------------------
// RecordingRenderingAPI — shader management
// ---------------------------------------------------------------------------

RecordedShader* RecordingRenderingAPI::FindShader(uint64_t id0, uint64_t id1) {
    for (auto& e : mShaders) {
        if (e.info.id0 == id0 && e.info.id1 == id1) {
            return &e.info;
        }
    }
    return nullptr;
}

void RecordingRenderingAPI::UnloadShader(ShaderProgram* /*prg*/) {
    RCall c;
    c.type = RCallType::UnloadShader;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::LoadShader(ShaderProgram* prg) {
    RCall c;
    c.type = RCallType::LoadShader;
    mCalls.push_back(c);
    if (prg) {
        auto* e = SPToEntry(prg);
        mCurrentShader = &e->info;
    } else {
        mCurrentShader = nullptr;
    }
}

void RecordingRenderingAPI::ClearShaderCache() {
    RCall c;
    c.type = RCallType::ClearShaderCache;
    mCalls.push_back(c);
    mShaders.clear();
    mCurrentShader = nullptr;
}

ShaderProgram* RecordingRenderingAPI::CreateAndLoadNewShader(uint64_t id0, uint64_t id1) {
    RCall c;
    c.type = RCallType::CreateAndLoadNewShader;
    // Store the 64-bit IDs split into two 32-bit halves each
    c.i[0] = static_cast<int>(id0 >> 32);
    c.i[1] = static_cast<int>(id0 & 0xFFFFFFFF);
    c.i[2] = static_cast<int>(id1 >> 32);
    c.i[3] = static_cast<int>(id1 & 0xFFFFFFFF);
    mCalls.push_back(c);

    // Reserve extra capacity before push_back so the reallocation (if any)
    // happens before we take a pointer — preventing pointer invalidation.
    mShaders.reserve(mShaders.size() + 1);
    mShaders.push_back({});
    ShaderEntry& e = mShaders.back();
    e.info.id0 = id0;
    e.info.id1 = id1;
    // numInputs and usedTextures default to 0/false — tests may override via
    // a cast to RecordedShader* if they need specific values.
    mCurrentShader = &e.info;
    return EntryToSP(&e);
}

ShaderProgram* RecordingRenderingAPI::LookupShader(uint64_t id0, uint64_t id1) {
    RCall c;
    c.type = RCallType::LookupShader;
    mCalls.push_back(c);
    for (auto& e : mShaders) {
        if (e.info.id0 == id0 && e.info.id1 == id1) {
            return EntryToSP(&e);
        }
    }
    return nullptr;
}

void RecordingRenderingAPI::ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) {
    RCall c;
    c.type = RCallType::ShaderGetInfo;
    mCalls.push_back(c);
    if (!prg) {
        if (numInputs)
            *numInputs = 0;
        if (usedTextures) {
            usedTextures[0] = false;
            usedTextures[1] = false;
        }
        return;
    }
    auto* e = SPToEntry(prg);
    if (numInputs)
        *numInputs = e->info.numInputs;
    if (usedTextures) {
        usedTextures[0] = e->info.usedTextures[0];
        usedTextures[1] = e->info.usedTextures[1];
    }
}

// ---------------------------------------------------------------------------
// Texture management
// ---------------------------------------------------------------------------

uint32_t RecordingRenderingAPI::NewTexture() {
    RCall c;
    c.type = RCallType::NewTexture;
    int id = mNextTexId++;
    c.i[0] = id;
    mCalls.push_back(c);
    return static_cast<uint32_t>(id);
}

void RecordingRenderingAPI::SelectTexture(int tile, uint32_t texId) {
    RCall c;
    c.type = RCallType::SelectTexture;
    c.i[0] = tile;
    c.i[1] = static_cast<int>(texId);
    mCalls.push_back(c);
}

void RecordingRenderingAPI::UploadTexture(const uint8_t* /*rgba32Buf*/, uint32_t w, uint32_t h) {
    RCall c;
    c.type = RCallType::UploadTexture;
    c.i[0] = static_cast<int>(w);
    c.i[1] = static_cast<int>(h);
    mCalls.push_back(c);
}

void RecordingRenderingAPI::SetSamplerParameters(int sampler, bool linear, uint32_t cms, uint32_t cmt) {
    RCall c;
    c.type = RCallType::SetSamplerParameters;
    c.i[0] = sampler;
    c.i[1] = linear ? 1 : 0;
    c.i[2] = static_cast<int>(cms);
    c.i[3] = static_cast<int>(cmt);
    mCalls.push_back(c);
}

void RecordingRenderingAPI::DeleteTexture(uint32_t texId) {
    RCall c;
    c.type = RCallType::DeleteTexture;
    c.i[0] = static_cast<int>(texId);
    mCalls.push_back(c);
}

void RecordingRenderingAPI::SelectTextureFb(int fbId) {
    RCall c;
    c.type = RCallType::SelectTextureFb;
    c.i[0] = fbId;
    mCalls.push_back(c);
}

// ---------------------------------------------------------------------------
// State setters
// ---------------------------------------------------------------------------

void RecordingRenderingAPI::SetDepthTestAndMask(bool depthTest, bool zUpd) {
    RCall c;
    c.type = RCallType::SetDepthTestAndMask;
    c.i[0] = depthTest ? 1 : 0;
    c.i[1] = zUpd ? 1 : 0;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::SetZmodeDecal(bool decal) {
    RCall c;
    c.type = RCallType::SetZmodeDecal;
    c.i[0] = decal ? 1 : 0;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::SetViewport(int x, int y, int w, int h) {
    RCall c;
    c.type = RCallType::SetViewport;
    c.i[0] = x;
    c.i[1] = y;
    c.i[2] = w;
    c.i[3] = h;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::SetScissor(int x, int y, int w, int h) {
    RCall c;
    c.type = RCallType::SetScissor;
    c.i[0] = x;
    c.i[1] = y;
    c.i[2] = w;
    c.i[3] = h;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::SetUseAlpha(bool useAlpha) {
    RCall c;
    c.type = RCallType::SetUseAlpha;
    c.i[0] = useAlpha ? 1 : 0;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::SetTextureFilter(FilteringMode mode) {
    RCall c;
    c.type = RCallType::SetTextureFilter;
    c.i[0] = static_cast<int>(mode);
    mCalls.push_back(c);
    mFilterMode = mode;
}

void RecordingRenderingAPI::SetSrgbMode() {
    RCall c;
    c.type = RCallType::SetSrgbMode;
    mCalls.push_back(c);
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void RecordingRenderingAPI::DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    RCall c;
    c.type = RCallType::DrawTriangles;
    c.vbo.assign(buf_vbo, buf_vbo + buf_vbo_len);
    c.vbo_num_tris = buf_vbo_num_tris;
    c.i[0] = static_cast<int>(buf_vbo_len);
    c.i[1] = static_cast<int>(buf_vbo_num_tris);
    mCalls.push_back(c);
    mFrameTriCount += buf_vbo_num_tris;
}

// ---------------------------------------------------------------------------
// Frame lifecycle
// ---------------------------------------------------------------------------

void RecordingRenderingAPI::Init() {
    RCall c;
    c.type = RCallType::Init;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::OnResize() {
    RCall c;
    c.type = RCallType::OnResize;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::StartFrame() {
    RCall c;
    c.type = RCallType::StartFrame;
    mCalls.push_back(c);
    mFrameTriCount = 0;
}

void RecordingRenderingAPI::EndFrame() {
    RCall c;
    c.type = RCallType::EndFrame;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::FinishRender() {
    RCall c;
    c.type = RCallType::FinishRender;
    mCalls.push_back(c);
}

int RecordingRenderingAPI::CreateFramebuffer() {
    RCall c;
    c.type = RCallType::CreateFramebuffer;
    int id = mNextFbId++;
    c.i[0] = id;
    mCalls.push_back(c);
    return id;
}

void RecordingRenderingAPI::UpdateFramebufferParameters(int fbId, uint32_t w, uint32_t h, uint32_t msaa,
                                                         bool oglInvertY, bool renderTarget, bool hasDepth,
                                                         bool canExtractDepth) {
    RCall c;
    c.type = RCallType::UpdateFramebufferParameters;
    c.i[0] = fbId;
    c.i[1] = static_cast<int>(w);
    c.i[2] = static_cast<int>(h);
    c.i[3] = static_cast<int>(msaa);
    c.i[4] = oglInvertY ? 1 : 0;
    c.i[5] = renderTarget ? 1 : 0;
    c.i[6] = hasDepth ? 1 : 0;
    c.i[7] = canExtractDepth ? 1 : 0;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::StartDrawToFramebuffer(int fbId, float noiseScale) {
    RCall c;
    c.type = RCallType::StartDrawToFramebuffer;
    c.i[0] = fbId;
    c.f[0] = noiseScale;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::CopyFramebuffer(int dst, int src, int sx0, int sy0, int sx1, int sy1, int dx0, int dy0,
                                             int dx1, int dy1) {
    RCall c;
    c.type = RCallType::CopyFramebuffer;
    c.i[0] = dst;
    c.i[1] = src;
    c.i[2] = sx0;
    c.i[3] = sy0;
    c.i[4] = sx1;
    c.i[5] = sy1;
    c.i[6] = dx0;
    c.i[7] = dy0;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::ClearFramebuffer(bool color, bool depth) {
    RCall c;
    c.type = RCallType::ClearFramebuffer;
    c.i[0] = color ? 1 : 0;
    c.i[1] = depth ? 1 : 0;
    mCalls.push_back(c);
}

void RecordingRenderingAPI::ReadFramebufferToCPU(int fbId, uint32_t w, uint32_t h, uint16_t* /*buf*/) {
    RCall c;
    c.type = RCallType::ReadFramebufferToCPU;
    c.i[0] = fbId;
    c.i[1] = static_cast<int>(w);
    c.i[2] = static_cast<int>(h);
    mCalls.push_back(c);
}

void RecordingRenderingAPI::ResolveMSAAColorBuffer(int dst, int src) {
    RCall c;
    c.type = RCallType::ResolveMSAAColorBuffer;
    c.i[0] = dst;
    c.i[1] = src;
    mCalls.push_back(c);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::vector<RCall> RecordingRenderingAPI::GetDrawCalls() const {
    std::vector<RCall> out;
    for (const auto& c : mCalls) {
        if (c.type == RCallType::DrawTriangles) {
            out.push_back(c);
        }
    }
    return out;
}

void RecordingRenderingAPI::Reset() {
    mCalls.clear();
    mFrameTriCount = 0;
    mShaders.clear();
    mCurrentShader = nullptr;
    mNextTexId = 1;
    mNextFbId = 1;
}

} // namespace Fast
