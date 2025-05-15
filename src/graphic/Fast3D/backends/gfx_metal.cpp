//
//  gfx_metal.cpp
//  libultraship
//
//  Created by David Chavez on 16.08.22.
//

#ifdef __APPLE__

#include "gfx_metal.h"

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include <time.h>
#include <math.h>
#include <cmath>
#include <stddef.h>

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>

#include <SDL_render.h>
#include <imgui_impl_metal.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#include "gfx_metal_shader.h"

#include "libultraship/libultra/abi.h"
#include "public/bridge/consolevariablebridge.h"

#define ARRAY_COUNT(arr) (s32)(sizeof(arr) / sizeof(arr[0]))

// MARK: - Helpers
namespace Fast {

static MTL::SamplerAddressMode gfx_cm_to_metal(uint32_t val) {
    switch (val) {
        case G_TX_NOMIRROR | G_TX_CLAMP:
            return MTL::SamplerAddressModeClampToEdge;
        case G_TX_MIRROR | G_TX_WRAP:
            return MTL::SamplerAddressModeMirrorRepeat;
        case G_TX_MIRROR | G_TX_CLAMP:
            return MTL::SamplerAddressModeMirrorClampToEdge;
        case G_TX_NOMIRROR | G_TX_WRAP:
            return MTL::SamplerAddressModeRepeat;
    }

    return MTL::SamplerAddressModeClampToEdge;
}

// MARK: - ImGui & SDL Wrappers

bool GfxRenderingAPIMetal::NonUniformThreadGroupSupported() {
#ifdef __IOS__
    // iOS devices with A11 or later support dispatch threads
    return mDevice->supportsFamily(MTL::GPUFamilyApple4);
#else
    // macOS devices with Metal 2 support dispatch threads
    return mDevice->supportsFamily(MTL::GPUFamilyMac2);
#endif
}

bool GfxRenderingAPIMetal::MetalInit(SDL_Renderer* renderer) {
    mRenderer = renderer;
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    mLayer = (CA::MetalLayer*)SDL_RenderGetMetalLayer(renderer);
    mLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

    mDevice = mLayer->device();
    mCommandQueue = mDevice->newCommandQueue();

    for (size_t i = 0; i < kMaxVertexBufferPoolSize; i++) {
        MTL::Buffer* new_buffer = mDevice->newBuffer(256 * 32 * 3 * sizeof(float) * 50, MTL::ResourceStorageModeShared);
        mVertexBufferPool[i] = new_buffer;
    }

    autorelease_pool->release();
    mNonUniformThreadgroupSupported = NonUniformThreadGroupSupported();

    return ImGui_ImplMetal_Init(mDevice);
}

static void SetupScreenFramebuffer(uint32_t width, uint32_t height);

void GfxRenderingAPIMetal::NewFrame() {
    int width, height;
    SDL_GetRendererOutputSize(mRenderer, &width, &height);
    SetupScreenFramebuffer(width, height);

    MTL::RenderPassDescriptor* current_render_pass = mFramebuffers[0].mRenderPassDescriptor;
    ImGui_ImplMetal_NewFrame(current_render_pass);
}

void GfxRenderingAPIMetal::SetupFloatingFrame() {
    // We need the descriptor for the main framebuffer and to clear the existing depth attachment
    // so that we can set ImGui up again for our floating windows. Helps avoid Metal API validation issues.
    MTL::RenderPassDescriptor* current_render_pass = mFramebuffers[0].mRenderPassDescriptor;
    current_render_pass->setDepthAttachment(nullptr);
    ImGui_ImplMetal_NewFrame(current_render_pass);
}

void GfxRenderingAPIMetal::RenderDrawData(ImDrawData* drawData) {
    auto framebuffer = mFramebuffers[0];

    // Workaround for detecting when transitioning to/from full screen mode.
    MTL::Texture* screen_texture = mTextures[framebuffer.mTextureId].texture;
    int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (screen_texture->width() != fb_width || screen_texture->height() != fb_height)
        return;

    ImGui_ImplMetal_RenderDrawData(drawData, framebuffer.mCommandBuffer, framebuffer.mCommandEncoder);
}

// MARK: - Metal Graphics Rendering API

const char* GfxRenderingAPIMetal::GetName() {
    return "Metal";
}

int GfxRenderingAPIMetal::GetMaxTextureSize() {
    return mDevice->supportsFamily(MTL::GPUFamilyApple3) ? 16384 : 8192;
}

void GfxRenderingAPIMetal::Init() {
    // Create the default framebuffer which represents the window
    FramebufferMetal& fb = mFramebuffers[CreateFramebuffer()];
    fb.mMsaaLevel = 1;

    // Check device for supported msaa levels
    for (uint32_t sample_count = 1; sample_count <= METAL_MAX_MULTISAMPLE_SAMPLE_COUNT; sample_count++) {
        if (mDevice->supportsTextureSampleCount(sample_count)) {
            mMsaaNumQualityLevels[sample_count - 1] = 1;
        } else {
            mMsaaNumQualityLevels[sample_count - 1] = 0;
        }
    }

    // Compute shader for retrieving depth values
    const char* depth_shader = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct CoordUniforms {
            uint2 coords[1024];
        };

        kernel void depthKernel(depth2d<float, access::read> depth_texture [[ texture(0) ]],
                                     constant CoordUniforms& query_coords [[ buffer(0) ]],
                                     device float* output_values [[ buffer(1) ]],
                                     ushort2 thread_position [[ thread_position_in_grid ]]) {
            uint2 coord = query_coords.coords[thread_position.x];
            output_values[thread_position.x] = depth_texture.read(coord);
        }

        kernel void convertToRGB5A1(texture2d<half, access::read> inTexture [[ texture(0) ]],
                                    device short* outputBuffer [[ buffer(0) ]],
                                    uint2 gid [[ thread_position_in_grid ]]) {
            uint index = gid.x + (inTexture.get_width() * gid.y);
            half4 pixel = inTexture.read(gid);
            uint r = pixel.r * 0x1F;
            uint g = pixel.g * 0x1F;
            uint b = pixel.b * 0x1F;
            uint a = pixel.a > 0;
            outputBuffer[index] = (r << 11) | (g << 6) | (b << 1) | a;
        }
    )";

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    NS::Error* error = nullptr;
    MTL::Library* library =
        mDevice->newLibrary(NS::String::string(depth_shader, NS::UTF8StringEncoding), nullptr, &error);

    if (error != nullptr)
        SPDLOG_ERROR("Failed to compile shader library: {}",
                     error->localizedDescription()->cString(NS::UTF8StringEncoding));

    mDepthComputeFunction = library->newFunction(NS::String::string("depthKernel", NS::UTF8StringEncoding));
    mConvertToRgb5a1Function = library->newFunction(NS::String::string("convertToRGB5A1", NS::UTF8StringEncoding));

    library->release();
    autorelease_pool->release();
}

struct GfxClipParameters GfxRenderingAPIMetal::GetClipParameters() {
    return { true, false };
}

void GfxRenderingAPIMetal::UnloadShader(struct ShaderProgram* old_prg) {
}

void GfxRenderingAPIMetal::LoadShader(struct ShaderProgram* new_prg) {
    mShaderProgram = (struct ShaderProgramMetal*)new_prg;
}

struct ShaderProgram* GfxRenderingAPIMetal::CreateAndLoadNewShader(uint64_t shader_id0, uint32_t shader_id1) {
    CCFeatures cc_features;
    gfx_cc_get_features(shader_id0, shader_id1, &cc_features);

    size_t numFloats = 0;
    std::string buf;
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    MTL::VertexDescriptor* vertex_descriptor =
        gfx_metal_build_shader(buf, numFloats, cc_features, mCurrentFilterMode == FILTER_THREE_POINT);

    NS::Error* error = nullptr;
    MTL::Library* library =
        mDevice->newLibrary(NS::String::string(buf.data(), NS::UTF8StringEncoding), nullptr, &error);

    if (error != nullptr)
        SPDLOG_ERROR("Failed to compile shader library, error {}",
                     error->localizedDescription()->cString(NS::UTF8StringEncoding));

    MTL::RenderPipelineDescriptor* pipeline_descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    MTL::Function* vertexFunc = library->newFunction(NS::String::string("vertexShader", NS::UTF8StringEncoding));
    MTL::Function* fragmentFunc = library->newFunction(NS::String::string("fragmentShader", NS::UTF8StringEncoding));

    pipeline_descriptor->setVertexFunction(vertexFunc);
    pipeline_descriptor->setFragmentFunction(fragmentFunc);
    pipeline_descriptor->setVertexDescriptor(vertex_descriptor);

    pipeline_descriptor->colorAttachments()->object(0)->setPixelFormat(mSrgbMode ? MTL::PixelFormatBGRA8Unorm_sRGB
                                                                                 : MTL::PixelFormatBGRA8Unorm);
    pipeline_descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    if (cc_features.opt_alpha) {
        pipeline_descriptor->colorAttachments()->object(0)->setBlendingEnabled(true);
        pipeline_descriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
        pipeline_descriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(
            MTL::BlendFactorOneMinusSourceAlpha);
        pipeline_descriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
        pipeline_descriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorZero);
        pipeline_descriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);
        pipeline_descriptor->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
        pipeline_descriptor->colorAttachments()->object(0)->setWriteMask(MTL::ColorWriteMaskAll);
    } else {
        pipeline_descriptor->colorAttachments()->object(0)->setBlendingEnabled(false);
        pipeline_descriptor->colorAttachments()->object(0)->setWriteMask(MTL::ColorWriteMaskAll);
    }

    struct ShaderProgramMetal* prg = &mShaderProgramPool[std::make_pair(shader_id0, shader_id1)];
    prg->shader_id0 = shader_id0;
    prg->shader_id1 = shader_id1;
    prg->usedTextures[0] = cc_features.usedTextures[0];
    prg->usedTextures[1] = cc_features.usedTextures[1];
    prg->usedTextures[2] = cc_features.used_masks[0];
    prg->usedTextures[3] = cc_features.used_masks[1];
    prg->usedTextures[4] = cc_features.used_blend[0];
    prg->usedTextures[5] = cc_features.used_blend[1];
    prg->numInputs = cc_features.numInputs;
    prg->numFloats = numFloats;

    // Prepoluate pipeline state cache with program and available msaa levels
    for (int i = 0; i < ARRAY_COUNT(mMsaaNumQualityLevels); i++) {
        if (mMsaaNumQualityLevels[i] == 1) {
            int msaa_level = i + 1;
            pipeline_descriptor->setSampleCount(msaa_level);
            MTL::RenderPipelineState* pipeline_state = mDevice->newRenderPipelineState(pipeline_descriptor, &error);

            if (!pipeline_state || error != nullptr) {
                // Pipeline State creation could fail if we haven't properly set up our pipeline descriptor.
                // If the Metal API validation is enabled, we can find out more information about what
                // went wrong.  (Metal API validation is enabled by default when a debug build is run
                // from Xcode)
                SPDLOG_ERROR("Failed to create pipeline state, error {}",
                             error->localizedDescription()->cString(NS::UTF8StringEncoding));
            }

            prg->pipeline_state_variants[msaa_level] = pipeline_state;
        }
    }

    LoadShader((struct ShaderProgram*)prg);

    vertexFunc->release();
    fragmentFunc->release();
    library->release();
    pipeline_descriptor->release();
    autorelease_pool->release();

    return (struct ShaderProgram*)prg;
}

struct ShaderProgram* GfxRenderingAPIMetal::LookupShader(uint64_t shader_id0, uint32_t shader_id1) {
    auto it = mShaderProgramPool.find(std::make_pair(shader_id0, shader_id1));
    return it == mShaderProgramPool.end() ? nullptr : (struct ShaderProgram*)&it->second;
}

void GfxRenderingAPIMetal::ShaderGetInfo(struct ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) {
    struct ShaderProgramMetal* p = (struct ShaderProgramMetal*)prg;

    *numInputs = p->numInputs;
    usedTextures[0] = p->usedTextures[0];
    usedTextures[1] = p->usedTextures[1];
}

uint32_t GfxRenderingAPIMetal::NewTexture() {
    mTextures.resize(mTextures.size() + 1);
    return (uint32_t)(mTextures.size() - 1);
}

void GfxRenderingAPIMetal::DeleteTexture(uint32_t texID) {
}

void GfxRenderingAPIMetal::SelectTexture(int tile, uint32_t texture_id) {
    mCurrentTile = tile;
    mCurrentTextureIds[tile] = texture_id;
}

void GfxRenderingAPIMetal::UploadTexture(const uint8_t* rgba32_buf, uint32_t width, uint32_t height) {
    TextureDataMetal* texture_data = &mTextures[mCurrentTextureIds[mCurrentTile]];

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    MTL::TextureDescriptor* texture_descriptor =
        MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm, width, height, true);
    texture_descriptor->setArrayLength(1);
    texture_descriptor->setMipmapLevelCount(1);
    texture_descriptor->setSampleCount(1);
    texture_descriptor->setStorageMode(MTL::StorageModeShared);

    MTL::Region region = MTL::Region::Make2D(0, 0, width, height);

    MTL::Texture* texture = texture_data->texture;
    if (texture_data->texture == nullptr || texture_data->texture->width() != width ||
        texture_data->texture->height() != height) {
        if (texture_data->texture != nullptr)
            texture_data->texture->release();

        texture = mDevice->newTexture(texture_descriptor);
    }

    NS::UInteger bytes_per_pixel = 4;
    NS::UInteger bytes_per_row = bytes_per_pixel * width;
    texture->replaceRegion(region, 0, rgba32_buf, bytes_per_row);
    texture_data->texture = texture;

    autorelease_pool->release();
}

void GfxRenderingAPIMetal::SetSamplerParameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt) {
    TextureDataMetal* texture_data = &mTextures[mCurrentTextureIds[tile]];
    texture_data->linear_filtering = linear_filter;

    // This function is called twice per texture, the first one only to set default values.
    // Maybe that could be skipped? Anyway, make sure to release the first default sampler
    // state before setting the actual one.
    texture_data->sampler->release();

    MTL::SamplerDescriptor* sampler_descriptor = MTL::SamplerDescriptor::alloc()->init();
    MTL::SamplerMinMagFilter filter = linear_filter && mCurrentFilterMode == FILTER_LINEAR
                                          ? MTL::SamplerMinMagFilterLinear
                                          : MTL::SamplerMinMagFilterNearest;
    sampler_descriptor->setMinFilter(filter);
    sampler_descriptor->setMagFilter(filter);
    sampler_descriptor->setSAddressMode(gfx_cm_to_metal(cms));
    sampler_descriptor->setTAddressMode(gfx_cm_to_metal(cmt));
    sampler_descriptor->setRAddressMode(MTL::SamplerAddressModeRepeat);

    texture_data->sampler = mDevice->newSamplerState(sampler_descriptor);
    sampler_descriptor->release();
}

void GfxRenderingAPIMetal::SetDepthTestAndMask(bool depth_test, bool depth_mask) {
    mCurrentDepthTest = depth_test;
    mCurrentDepthMask = depth_mask;
}

void GfxRenderingAPIMetal::SetZmodeDecal(bool zmode_decal) {
    mCurrentZmodeDecal = zmode_decal;
}

void GfxRenderingAPIMetal::SetViewport(int x, int y, int width, int height) {
    FramebufferMetal& fb = mFramebuffers[mCurrentFramebuffer];

    fb.mViewport->originX = x;
    fb.mViewport->originY = mRenderTargetHeight - y - height;
    fb.mViewport->width = width;
    fb.mViewport->height = height;
    fb.mViewport->znear = 0;
    fb.mViewport->zfar = 1;

    fb.mCommandEncoder->setViewport(*fb.mViewport);
}

void GfxRenderingAPIMetal::SetScissor(int x, int y, int width, int height) {
    FramebufferMetal& fb = mFramebuffers[mCurrentFramebuffer];
    TextureDataMetal tex = mTextures[fb.mTextureId];

    // clamp to viewport size as metal does not support larger values than viewport size
    fb.mScissorRect->x = std::max(0, std::min<int>(x, tex.width));
    fb.mScissorRect->y = std::max(0, std::min<int>(mRenderTargetHeight - y - height, tex.height));
    fb.mScissorRect->width = std::max(0, std::min<int>(width, tex.width));
    fb.mScissorRect->height = std::max(0, std::min<int>(height, tex.height));

    fb.mCommandEncoder->setScissorRect(*fb.mScissorRect);
}

void GfxRenderingAPIMetal::SetUseAlpha(bool use_alpha) {
    // Already part of the pipeline state from shader info
}

void GfxRenderingAPIMetal::DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    auto& current_framebuffer = mFramebuffers[mCurrentFramebuffer];

    if (current_framebuffer.mLastDepthTest != mCurrentDepthTest ||
        current_framebuffer.mLastDepthMask != mCurrentDepthMask) {
        current_framebuffer.mLastDepthTest = mCurrentDepthTest;
        current_framebuffer.mLastDepthMask = mCurrentDepthMask;

        MTL::DepthStencilDescriptor* depth_descriptor = MTL::DepthStencilDescriptor::alloc()->init();
        depth_descriptor->setDepthWriteEnabled(mCurrentDepthMask);
        depth_descriptor->setDepthCompareFunction(
            mCurrentDepthTest ? (mCurrentZmodeDecal ? MTL::CompareFunctionLessEqual : MTL::CompareFunctionLess)
                              : MTL::CompareFunctionAlways);

        MTL::DepthStencilState* depth_stencil_state = mDevice->newDepthStencilState(depth_descriptor);
        current_framebuffer.mCommandEncoder->setDepthStencilState(depth_stencil_state);

        depth_descriptor->release();
    }

    if (current_framebuffer.mLastZmodeDecal != mCurrentZmodeDecal) {
        current_framebuffer.mLastZmodeDecal = mCurrentZmodeDecal;

        current_framebuffer.mCommandEncoder->setTriangleFillMode(MTL::TriangleFillModeFill);
        current_framebuffer.mCommandEncoder->setCullMode(MTL::CullModeNone);
        current_framebuffer.mCommandEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);

        // SSDB = SlopeScaledDepthBias 120 leads to -2 at 240p which is the same as N64 mode which has very little
        // fighting
        const int n64modeFactor = 120;
        const int noVanishFactor = 100;
        float SSDB = -2;
        switch (CVarGetInteger(CVAR_Z_FIGHTING_MODE, 0)) {
            case 1: // scaled z-fighting (N64 mode like)
                SSDB = -1.0f * (float)mRenderTargetHeight / n64modeFactor;
                break;
            case 2: // no vanishing paths
                SSDB = -1.0f * (float)mRenderTargetHeight / noVanishFactor;
                break;
            case 0: // disabled
            default:
                SSDB = -2;
        }
        current_framebuffer.mCommandEncoder->setDepthBias(0, mCurrentZmodeDecal ? SSDB : 0, 0);
    }

    MTL::Buffer* vertex_buffer = mVertexBufferPool[mCurrentVertexBufferPoolIndex];
    memcpy((char*)vertex_buffer->contents() + mCurrentVertexBufferOffset, buf_vbo, sizeof(float) * buf_vbo_len);

    if (!current_framebuffer.mHasBoundVertexShader) {
        current_framebuffer.mCommandEncoder->setVertexBuffer(vertex_buffer, 0, 0);
        current_framebuffer.mHasBoundVertexShader = true;
    }

    current_framebuffer.mCommandEncoder->setVertexBufferOffset(mCurrentVertexBufferOffset, 0);

    if (!current_framebuffer.mHasBoundFragShader) {
        current_framebuffer.mCommandEncoder->setFragmentBuffer(mFrameUniformBuffer, 0, 0);
        current_framebuffer.mHasBoundFragShader = true;
    }

    for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
        if (mShaderProgram->usedTextures[i]) {
            if (current_framebuffer.mLastBoundTextures[i] != mTextures[mCurrentTextureIds[i]].texture) {
                current_framebuffer.mLastBoundTextures[i] = mTextures[mCurrentTextureIds[i]].texture;
                current_framebuffer.mCommandEncoder->setFragmentTexture(mTextures[mCurrentTextureIds[i]].texture, i);

                if (current_framebuffer.mLastBoundSamplers[i] != mTextures[mCurrentTextureIds[i]].sampler) {
                    current_framebuffer.mLastBoundSamplers[i] = mTextures[mCurrentTextureIds[i]].sampler;
                    current_framebuffer.mCommandEncoder->setFragmentSamplerState(
                        mTextures[mCurrentTextureIds[i]].sampler, i);
                }
            }
        }
    }

    if (current_framebuffer.mLastShaderProgram != mShaderProgram) {
        current_framebuffer.mLastShaderProgram = mShaderProgram;

        MTL::RenderPipelineState* pipeline_state =
            mShaderProgram->pipeline_state_variants[current_framebuffer.mMsaaLevel];
        current_framebuffer.mCommandEncoder->setRenderPipelineState(pipeline_state);
    }

    current_framebuffer.mCommandEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, 0.f, buf_vbo_num_tris * 3);
    mCurrentVertexBufferOffset += sizeof(float) * buf_vbo_len;

    autorelease_pool->release();
}

void GfxRenderingAPIMetal::OnResize() {
}

void GfxRenderingAPIMetal::StartFrame() {
    mFrameUniforms.frameCount++;
    if (mFrameUniforms.frameCount > 150) {
        // No high values, as noise starts to look ugly
        mFrameUniforms.frameCount = 0;
    }

    if (!mFrameUniformBuffer) {
        mFrameUniformBuffer = mDevice->newBuffer(sizeof(FrameUniforms), MTL::ResourceCPUCacheModeDefaultCache);
    }
    if (!mCoordUniformBuffer) {
        mCoordUniformBuffer = mDevice->newBuffer(sizeof(CoordUniforms), MTL::ResourceCPUCacheModeDefaultCache);
    }

    mCurrentVertexBufferOffset = 0;

    mFrameAutoreleasePool = NS::AutoreleasePool::alloc()->init();
}

void GfxRenderingAPIMetal::EndFrame() {
    std::set<int>::iterator it = mDrawnFramebuffers.begin();
    it++;

    while (it != mDrawnFramebuffers.end()) {
        auto framebuffer = mFramebuffers[*it];

        if (!framebuffer.mHasEndedEncoding)
            framebuffer.mCommandEncoder->endEncoding();

        framebuffer.mCommandBuffer->commit();
        it++;
    }

    auto screen_framebuffer = mFramebuffers[0];
    screen_framebuffer.mCommandEncoder->endEncoding();
    screen_framebuffer.mCommandBuffer->presentDrawable(mCurrentDrawable);
    mCurrentVertexBufferPoolIndex = (mCurrentVertexBufferPoolIndex + 1) % kMaxVertexBufferPoolSize;
    screen_framebuffer.mCommandBuffer->commit();

    mDrawnFramebuffers.clear();

    // Cleanup states
    for (int fb_id = 0; fb_id < (int)mFramebuffers.size(); fb_id++) {
        FramebufferMetal& fb = mFramebuffers[fb_id];

        fb.mLastShaderProgram = nullptr;
        fb.mCommandBuffer = nullptr;
        fb.mCommandEncoder = nullptr;
        fb.mHasEndedEncoding = false;
        fb.mHasBoundVertexShader = false;
        fb.mHasBoundFragShader = false;
        for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
            fb.mLastBoundTextures[i] = nullptr;
            fb.mLastBoundSamplers[i] = nullptr;
        }
        memset(fb.mViewport, 0, sizeof(MTL::Viewport));
        memset(fb.mScissorRect, 0, sizeof(MTL::ScissorRect));
        fb.mLastDepthTest = -1;
        fb.mLastDepthMask = -1;
        fb.mLastZmodeDecal = -1;
    }

    mFrameAutoreleasePool->release();
}

void GfxRenderingAPIMetal::FinishRender() {
}

int GfxRenderingAPIMetal::CreateFramebuffer() {
    uint32_t texture_id = NewTexture();
    TextureDataMetal& t = mTextures[texture_id];

    size_t index = mFramebuffers.size();
    mFramebuffers.resize(mFramebuffers.size() + 1);
    FramebufferMetal& data = mFramebuffers.back();
    data.mScissorRect = new MTL::ScissorRect();
    data.mViewport = new MTL::Viewport();
    data.mTextureId = texture_id;

    uint32_t tile = 0;
    uint32_t saved = mCurrentTextureIds[tile];
    mCurrentTextureIds[tile] = texture_id;
    SetSamplerParameters(0, true, G_TX_WRAP, G_TX_WRAP);
    mCurrentTextureIds[tile] = saved;

    return (int)index;
}

void GfxRenderingAPIMetal::SetupScreenFramebuffer(uint32_t width, uint32_t height) {
    mCurrentDrawable = nullptr;
    mCurrentDrawable = mLayer->nextDrawable();

    bool msaa_enabled = CVarGetInteger("gMSAAValue", 1) > 1;

    FramebufferMetal& fb = mFramebuffers[0];
    TextureDataMetal& tex = mTextures[fb.mTextureId];

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    if (tex.texture != nullptr)
        tex.texture->release();

    tex.texture = mCurrentDrawable->texture();

    MTL::RenderPassDescriptor* render_pass_descriptor = MTL::RenderPassDescriptor::renderPassDescriptor();
    render_pass_descriptor->colorAttachments()->object(0)->setTexture(tex.texture);
    render_pass_descriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
    render_pass_descriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);

    tex.width = width;
    tex.height = height;

    // recreate depth texture only if necessary (size changed)
    if (fb.mDepthTexture == nullptr || (fb.mDepthTexture->width() != width || fb.mDepthTexture->height() != height)) {
        if (fb.mDepthTexture != nullptr)
            fb.mDepthTexture->release();

        // If possible, we eventually we want to disable this when msaa is enabled since we don't need this depth
        // texture However, problem is if the user switches to msaa during game, we need a way to then generate it
        // before drawing.
        MTL::TextureDescriptor* depth_tex_desc =
            MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatDepth32Float, width, height, true);

        depth_tex_desc->setTextureType(MTL::TextureType2D);
        depth_tex_desc->setStorageMode(MTL::StorageModePrivate);
        depth_tex_desc->setSampleCount(1);
        depth_tex_desc->setArrayLength(1);
        depth_tex_desc->setMipmapLevelCount(1);
        depth_tex_desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

        fb.mDepthTexture = mDevice->newTexture(depth_tex_desc);
    }

    render_pass_descriptor->depthAttachment()->setTexture(fb.mDepthTexture);
    render_pass_descriptor->depthAttachment()->setLoadAction(MTL::LoadActionLoad);
    render_pass_descriptor->depthAttachment()->setStoreAction(MTL::StoreActionStore);
    render_pass_descriptor->depthAttachment()->setClearDepth(1);

    if (fb.mRenderPassDescriptor != nullptr)
        fb.mRenderPassDescriptor->release();

    fb.mRenderPassDescriptor = render_pass_descriptor;
    fb.mRenderPassDescriptor->retain();
    fb.mRenderTarget = true;
    fb.mHasDepthBuffer = true;

    autorelease_pool->release();
}

void GfxRenderingAPIMetal::UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                                       bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                                       bool can_extract_depth) {
    // Screen framebuffer is handled separately on a frame by frame basis
    // see `SetupScreenFramebuffer`.
    if (fb_id == 0) {
        int width, height;
        SDL_GetRendererOutputSize(mRenderer, &width, &height);
        mLayer->setDrawableSize({ CGFloat(width), CGFloat(height) });

        return;
    }

    FramebufferMetal& fb = mFramebuffers[fb_id];
    TextureDataMetal& tex = mTextures[fb.mTextureId];

    width = std::max(width, 1U);
    height = std::max(height, 1U);
    while (msaa_level > 1 && mMsaaNumQualityLevels[msaa_level - 1] == 0) {
        --msaa_level;
    }

    bool diff = tex.width != width || tex.height != height || fb.mMsaaLevel != msaa_level;

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    if (diff || (fb.mRenderPassDescriptor != nullptr) != render_target) {
        MTL::TextureDescriptor* tex_descriptor = MTL::TextureDescriptor::alloc()->init();
        tex_descriptor->setTextureType(MTL::TextureType2D);
        tex_descriptor->setWidth(width);
        tex_descriptor->setHeight(height);
        tex_descriptor->setSampleCount(1);
        tex_descriptor->setMipmapLevelCount(1);
        tex_descriptor->setPixelFormat(mSrgbMode ? MTL::PixelFormatBGRA8Unorm_sRGB : MTL::PixelFormatBGRA8Unorm);
        tex_descriptor->setUsage((render_target ? MTL::TextureUsageRenderTarget : 0) | MTL::TextureUsageShaderRead);

        if (tex.texture != nullptr)
            tex.texture->release();

        tex.texture = mDevice->newTexture(tex_descriptor);

        if (msaa_level > 1) {
            tex_descriptor->setTextureType(MTL::TextureType2DMultisample);
            tex_descriptor->setSampleCount(msaa_level);
            tex_descriptor->setStorageMode(MTL::StorageModePrivate);
            tex_descriptor->setUsage(render_target ? MTL::TextureUsageRenderTarget : 0);

            if (tex.msaaTexture != nullptr)
                tex.msaaTexture->release();
            tex.msaaTexture = mDevice->newTexture(tex_descriptor);
        }

        if (render_target) {
            MTL::RenderPassDescriptor* render_pass_descriptor = MTL::RenderPassDescriptor::renderPassDescriptor();

            bool fb_msaa_enabled = (msaa_level > 1);
            bool game_msaa_enabled = CVarGetInteger("gMSAAValue", 1) > 1;

            if (fb_msaa_enabled) {
                render_pass_descriptor->colorAttachments()->object(0)->setTexture(tex.msaaTexture);
                render_pass_descriptor->colorAttachments()->object(0)->setResolveTexture(tex.texture);
                render_pass_descriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
                render_pass_descriptor->colorAttachments()->object(0)->setStoreAction(
                    MTL::StoreActionStoreAndMultisampleResolve);
            } else {
                render_pass_descriptor->colorAttachments()->object(0)->setTexture(tex.texture);
                render_pass_descriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionLoad);
                render_pass_descriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
            }

            if (fb.mRenderPassDescriptor != nullptr)
                fb.mRenderPassDescriptor->release();

            fb.mRenderPassDescriptor = render_pass_descriptor;
            fb.mRenderPassDescriptor->retain();
        }

        tex.width = width;
        tex.height = height;

        tex_descriptor->release();
    }

    if (has_depth_buffer && (diff || !fb.mHasDepthBuffer || (fb.mDepthTexture != nullptr) != can_extract_depth)) {
        MTL::TextureDescriptor* depth_tex_desc =
            MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatDepth32Float, width, height, true);
        depth_tex_desc->setTextureType(MTL::TextureType2D);
        depth_tex_desc->setStorageMode(MTL::StorageModePrivate);
        depth_tex_desc->setSampleCount(1);
        depth_tex_desc->setArrayLength(1);
        depth_tex_desc->setMipmapLevelCount(1);
        depth_tex_desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

        if (fb.mDepthTexture != nullptr)
            fb.mDepthTexture->release();

        fb.mDepthTexture = mDevice->newTexture(depth_tex_desc);

        if (msaa_level > 1) {
            depth_tex_desc->setTextureType(MTL::TextureType2DMultisample);
            depth_tex_desc->setSampleCount(msaa_level);

            if (fb.mMsaaDepthTexture != nullptr)
                fb.mMsaaDepthTexture->release();

            fb.mMsaaDepthTexture = mDevice->newTexture(depth_tex_desc);
        }
    }

    if (has_depth_buffer) {
        if (msaa_level > 1) {
            fb.mRenderPassDescriptor->depthAttachment()->setTexture(fb.mMsaaDepthTexture);
            fb.mRenderPassDescriptor->depthAttachment()->setResolveTexture(fb.mDepthTexture);
            fb.mRenderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionLoad);
            fb.mRenderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionMultisampleResolve);
            fb.mRenderPassDescriptor->depthAttachment()->setClearDepth(1);
        } else {
            fb.mRenderPassDescriptor->depthAttachment()->setTexture(fb.mDepthTexture);
            fb.mRenderPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadActionLoad);
            fb.mRenderPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreActionStore);
            fb.mRenderPassDescriptor->depthAttachment()->setClearDepth(1);
        }
    } else {
        fb.mRenderPassDescriptor->setDepthAttachment(nullptr);
    }

    fb.mRenderTarget = render_target;
    fb.mHasDepthBuffer = has_depth_buffer;
    fb.mMsaaLevel = msaa_level;

    autorelease_pool->release();
}

void GfxRenderingAPIMetal::StartDrawToFramebuffer(int fb_id, float noise_scale) {
    FramebufferMetal& fb = mFramebuffers[fb_id];
    mRenderTargetHeight = mTextures[fb.mTextureId].height;

    mCurrentFramebuffer = fb_id;
    mDrawnFramebuffers.insert(fb_id);

    if (fb.mRenderTarget && fb.mCommandBuffer == nullptr && fb.mCommandEncoder == nullptr) {
        fb.mCommandBuffer = mCommandQueue->commandBuffer();
        std::string fbcb_label = fmt::format("FrameBuffer {} Command Buffer", fb_id);
        fb.mCommandBuffer->setLabel(NS::String::string(fbcb_label.c_str(), NS::UTF8StringEncoding));

        // Queue the command buffers in order of start draw
        fb.mCommandBuffer->enqueue();

        fb.mCommandEncoder = fb.mCommandBuffer->renderCommandEncoder(fb.mRenderPassDescriptor);
        std::string fbce_label = fmt::format("FrameBuffer {} Command Encoder", fb_id);
        fb.mCommandEncoder->setLabel(NS::String::string(fbce_label.c_str(), NS::UTF8StringEncoding));
        fb.mCommandEncoder->setDepthClipMode(MTL::DepthClipModeClamp);
    }

    if (noise_scale != 0.0f) {
        mFrameUniforms.noiseScale = 1.0f / noise_scale;
    }

    memcpy(mFrameUniformBuffer->contents(), &mFrameUniforms, sizeof(FrameUniforms));
}

void GfxRenderingAPIMetal::ClearFramebuffer(bool color, bool depth) {
    if (!color && !depth) {
        return;
    }

    auto& framebuffer = mFramebuffers[mCurrentFramebuffer];

    // End the current render encoder
    framebuffer.mCommandEncoder->endEncoding();

    // Track the original load action and set the next load actions to Load to leverage the blit results
    MTL::RenderPassColorAttachmentDescriptor* srcColorAttachment =
        framebuffer.mRenderPassDescriptor->colorAttachments()->object(0);
    MTL::LoadAction origLoadAction = srcColorAttachment->loadAction();
    if (color) {
        srcColorAttachment->setLoadAction(MTL::LoadActionClear);
    }

    MTL::RenderPassDepthAttachmentDescriptor* srcDepthAttachment = framebuffer.mRenderPassDescriptor->depthAttachment();
    MTL::LoadAction origDepthLoadAction = MTL::LoadActionDontCare;
    if (depth && framebuffer.mHasDepthBuffer) {
        origDepthLoadAction = srcDepthAttachment->loadAction();
        srcDepthAttachment->setLoadAction(MTL::LoadActionClear);
    }

    // Create a new render encoder back onto the framebuffer
    framebuffer.mCommandEncoder = framebuffer.mCommandBuffer->renderCommandEncoder(framebuffer.mRenderPassDescriptor);

    std::string fbce_label = fmt::format("FrameBuffer {} Command Encoder After Clear", mCurrentFramebuffer);
    framebuffer.mCommandEncoder->setLabel(NS::String::string(fbce_label.c_str(), NS::UTF8StringEncoding));
    framebuffer.mCommandEncoder->setDepthClipMode(MTL::DepthClipModeClamp);
    framebuffer.mCommandEncoder->setViewport(*framebuffer.mViewport);
    framebuffer.mCommandEncoder->setScissorRect(*framebuffer.mScissorRect);

    // Now that the command encoder is started, we set the original load actions back for the next frame's use
    srcColorAttachment->setLoadAction(origLoadAction);
    if (depth && framebuffer.mHasDepthBuffer) {
        srcDepthAttachment->setLoadAction(origDepthLoadAction);
    }

    // Reset the framebuffer so the encoder is setup again when rendering triangles
    framebuffer.mHasBoundVertexShader = false;
    framebuffer.mHasBoundFragShader = false;
    framebuffer.mLastShaderProgram = nullptr;
    for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
        framebuffer.mLastBoundTextures[i] = nullptr;
        framebuffer.mLastBoundSamplers[i] = nullptr;
    }
    framebuffer.mLastDepthTest = -1;
    framebuffer.mLastDepthMask = -1;
    framebuffer.mLastZmodeDecal = -1;
}

void GfxRenderingAPIMetal::ResolveMSAAColorBuffer(int fb_id_target, int fb_id_source) {
    int source_texture_id = mFramebuffers[fb_id_source].mTextureId;
    MTL::Texture* source_texture = mTextures[source_texture_id].texture;

    int target_texture_id = mFramebuffers[fb_id_target].mTextureId;
    MTL::Texture* target_texture =
        target_texture_id == 0 ? mCurrentDrawable->texture() : mTextures[target_texture_id].texture;

    // Workaround for detecting when transitioning to/from full screen mode.
    if (source_texture->width() != target_texture->width() || source_texture->height() != target_texture->height()) {
        return;
    }

    // When the target buffer is our main window buffer, we need to perform the blit operation on the target
    // buffer instead of the source buffer
    if (fb_id_target != 0) {
        // Copy over the source framebuffer's texture to the target
        auto& source_framebuffer = mFramebuffers[fb_id_source];
        source_framebuffer.mCommandEncoder->endEncoding();
        source_framebuffer.mHasEndedEncoding = true;

        MTL::BlitCommandEncoder* blit_encoder = source_framebuffer.mCommandBuffer->blitCommandEncoder();
        blit_encoder->setLabel(NS::String::string("MSAA Copy Encoder", NS::UTF8StringEncoding));
        blit_encoder->copyFromTexture(source_texture, target_texture);
        blit_encoder->endEncoding();
    } else {
        // End the current render encoder
        auto& target_framebuffer = mFramebuffers[fb_id_target];
        target_framebuffer.mCommandEncoder->endEncoding();

        // Create a blit encoder
        MTL::BlitCommandEncoder* blit_encoder = target_framebuffer.mCommandBuffer->blitCommandEncoder();
        blit_encoder->setLabel(NS::String::string("MSAA Copy Encoder", NS::UTF8StringEncoding));

        // Copy the texture over using the origins and size
        blit_encoder->copyFromTexture(source_texture, target_texture);
        blit_encoder->endEncoding();

        // Update the load action to Load to leverage the blit results
        // The original load action will be set back on the next frame by SetupScreenFramebuffer
        MTL::RenderPassColorAttachmentDescriptor* targetColorAttachment =
            target_framebuffer.mRenderPassDescriptor->colorAttachments()->object(0);
        targetColorAttachment->setLoadAction(MTL::LoadActionLoad);

        // Create a new render encoder back onto the framebuffer
        target_framebuffer.mCommandEncoder =
            target_framebuffer.mCommandBuffer->renderCommandEncoder(target_framebuffer.mRenderPassDescriptor);

        std::string fbce_label = fmt::format("FrameBuffer {} Command Encoder After MSAA Resolve", fb_id_target);
        target_framebuffer.mCommandEncoder->setLabel(NS::String::string(fbce_label.c_str(), NS::UTF8StringEncoding));
    }
}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
GfxRenderingAPIMetal::GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) {
    auto framebuffer = mFramebuffers[fb_id];

    if (coordinates.size() > mCoordBufferSize) {
        if (mDepthValueOutputBuffer != nullptr)
            mDepthValueOutputBuffer->release();

        mDepthValueOutputBuffer =
            mDevice->newBuffer(sizeof(float) * coordinates.size(), MTL::ResourceOptionCPUCacheModeDefault);
        mDepthValueOutputBuffer->setLabel(NS::String::string("Depth output buffer", NS::UTF8StringEncoding));

        mCoordBufferSize = coordinates.size();
    }

    // zero out the buffer
    memset(mCoordUniformBuffer->contents(), 0, sizeof(CoordUniforms));
    memset(mDepthValueOutputBuffer->contents(), 0, sizeof(float) * coordinates.size());

    // map coordinates to right y axis
    size_t i = 0;
    for (const auto& coord : coordinates) {
        mCoordUniforms.coords[i].x = coord.first;
        mCoordUniforms.coords[i].y = framebuffer.mDepthTexture->height() - 1 - coord.second;
        ++i;
    }

    // set uniform values
    memcpy(mCoordUniformBuffer->contents(), &mCoordUniforms, sizeof(CoordUniforms));

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    auto command_buffer = mCommandQueue->commandBuffer();
    command_buffer->setLabel(NS::String::string("Depth Shader Command Buffer", NS::UTF8StringEncoding));

    NS::Error* error = nullptr;
    MTL::ComputePipelineState* compute_pipeline_state = mDevice->newComputePipelineState(mDepthComputeFunction, &error);

    MTL::ComputeCommandEncoder* compute_encoder = command_buffer->computeCommandEncoder();
    compute_encoder->setComputePipelineState(compute_pipeline_state);
    compute_encoder->setTexture(framebuffer.mDepthTexture, 0);
    compute_encoder->setBuffer(mCoordUniformBuffer, 0, 0);
    compute_encoder->setBuffer(mDepthValueOutputBuffer, 0, 1);

    MTL::Size thread_group_size = MTL::Size::Make(1, 1, 1);
    MTL::Size thread_group_count = MTL::Size::Make(coordinates.size(), 1, 1);

    // We validate if the device supports non-uniform threadgroup sizes
    if (mNonUniformThreadgroupSupported) {
        compute_encoder->dispatchThreads(thread_group_count, thread_group_size);
    } else {
        compute_encoder->dispatchThreadgroups(thread_group_count, thread_group_size);
    }

    compute_encoder->endEncoding();

    command_buffer->commit();
    command_buffer->waitUntilCompleted();

    // Now the depth values can be accessed in the buffer.
    float* depth_values = (float*)mDepthValueOutputBuffer->contents();

    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> res;
    {
        size_t i = 0;
        for (const auto& coord : coordinates) {
            res.emplace(coord, depth_values[i++] * 65532.0f);
        }
    }

    compute_pipeline_state->release();
    autorelease_pool->release();

    return res;
}

void* GfxRenderingAPIMetal::GetFramebufferTextureId(int fb_id) {
    return (void*)mTextures[mFramebuffers[fb_id].mTextureId].texture;
}

void GfxRenderingAPIMetal::SelectTextureFb(int fb_id) {
    int tile = 0;
    SelectTexture(tile, mFramebuffers[fb_id].mTextureId);
}

void GfxRenderingAPIMetal::CopyFramebuffer(int fb_dst_id, int fb_src_id, int srcX0, int srcY0, int srcX1, int srcY1,
                                           int dstX0, int dstY0, int dstX1, int dstY1) {
    if (fb_src_id >= (int)mFramebuffers.size() || fb_dst_id >= (int)mFramebuffers.size()) {
        return;
    }

    FramebufferMetal& source_framebuffer = mFramebuffers[fb_src_id];

    int source_texture_id = source_framebuffer.mTextureId;
    MTL::Texture* source_texture = mTextures[source_texture_id].texture;

    int target_texture_id = mFramebuffers[fb_dst_id].mTextureId;
    MTL::Texture* target_texture = mTextures[target_texture_id].texture;

    // End the current render encoder
    source_framebuffer.mCommandEncoder->endEncoding();

    // Create a blit encoder
    MTL::BlitCommandEncoder* blit_encoder = source_framebuffer.mCommandBuffer->blitCommandEncoder();
    blit_encoder->setLabel(NS::String::string("Copy Framebuffer Encoder", NS::UTF8StringEncoding));

    MTL::Origin source_origin = MTL::Origin(srcX0, srcY0, 0);
    MTL::Origin target_origin = MTL::Origin(dstX0, dstY0, 0);
    MTL::Size source_size = MTL::Size(srcX1 - srcX0, srcY1 - srcY0, 1);

    // Copy the texture over using the origins and size
    blit_encoder->copyFromTexture(source_texture, 0, 0, source_origin, source_size, target_texture, 0, 0,
                                  target_origin);
    blit_encoder->endEncoding();

    // Track the original load action and set the next load actions to Load to leverage the blit results
    MTL::RenderPassColorAttachmentDescriptor* srcColorAttachment =
        source_framebuffer.mRenderPassDescriptor->colorAttachments()->object(0);
    MTL::LoadAction origLoadAction = srcColorAttachment->loadAction();
    srcColorAttachment->setLoadAction(MTL::LoadActionLoad);

    MTL::RenderPassDepthAttachmentDescriptor* srcDepthAttachment =
        source_framebuffer.mRenderPassDescriptor->depthAttachment();
    MTL::LoadAction origDepthLoadAction = MTL::LoadActionDontCare;
    if (source_framebuffer.mHasDepthBuffer) {
        origDepthLoadAction = srcDepthAttachment->loadAction();
        srcDepthAttachment->setLoadAction(MTL::LoadActionLoad);
    }

    // Create a new render encoder back onto the framebuffer
    source_framebuffer.mCommandEncoder =
        source_framebuffer.mCommandBuffer->renderCommandEncoder(source_framebuffer.mRenderPassDescriptor);

    std::string fbce_label = fmt::format("FrameBuffer {} Command Encoder After Copy", fb_src_id);
    source_framebuffer.mCommandEncoder->setLabel(NS::String::string(fbce_label.c_str(), NS::UTF8StringEncoding));
    source_framebuffer.mCommandEncoder->setDepthClipMode(MTL::DepthClipModeClamp);
    source_framebuffer.mCommandEncoder->setViewport(*source_framebuffer.mViewport);
    source_framebuffer.mCommandEncoder->setScissorRect(*source_framebuffer.mScissorRect);

    // Now that the command encoder is started, we set the original load actions back for the next frame's use
    srcColorAttachment->setLoadAction(origLoadAction);
    if (source_framebuffer.mHasDepthBuffer) {
        srcDepthAttachment->setLoadAction(origDepthLoadAction);
    }

    // Reset the framebuffer so the encoder is setup again when rendering triangles
    source_framebuffer.mHasBoundVertexShader = false;
    source_framebuffer.mHasBoundFragShader = false;
    source_framebuffer.mLastShaderProgram = nullptr;
    for (int i = 0; i < SHADER_MAX_TEXTURES; i++) {
        source_framebuffer.mLastBoundTextures[i] = nullptr;
        source_framebuffer.mLastBoundSamplers[i] = nullptr;
    }
    source_framebuffer.mLastDepthTest = -1;
    source_framebuffer.mLastDepthMask = -1;
    source_framebuffer.mLastZmodeDecal = -1;
}

void GfxRenderingAPIMetal::GfxRenderingAPIMetal::ReadFramebufferToCPU(int fb_id, uint32_t width, uint32_t height,
                                                                      uint16_t* rgba16_buf) {
    if (fb_id >= (int)mFramebuffers.size()) {
        return;
    }

    FramebufferMetal& framebuffer = mFramebuffers[fb_id];
    MTL::Texture* texture = mTextures[framebuffer.mTextureId].texture;

    MTL::Buffer* output_buffer =
        mDevice->newBuffer(sizeof(uint16_t) * width * height, MTL::ResourceOptionCPUCacheModeDefault);
    output_buffer->setLabel(NS::String::string("Pixels output buffer", NS::UTF8StringEncoding));

    NS::AutoreleasePool* autorelease_pool = NS::AutoreleasePool::alloc()->init();

    auto command_buffer = mCommandQueue->commandBuffer();
    command_buffer->setLabel(NS::String::string("Read Pixels Shader Command Buffer", NS::UTF8StringEncoding));

    NS::Error* error = nullptr;
    MTL::ComputePipelineState* compute_pipeline_state =
        mDevice->newComputePipelineState(mConvertToRgb5a1Function, &error);

    // Use a compute encoder to convert the pixel data to rgba16 and transfer to a cpu readable buffer
    MTL::ComputeCommandEncoder* compute_encoder = command_buffer->computeCommandEncoder();
    compute_encoder->setComputePipelineState(compute_pipeline_state);
    compute_encoder->setTexture(texture, 0);
    compute_encoder->setBuffer(output_buffer, 0, 0);

    // Use a thread group size and count that covers the whole copy area
    MTL::Size thread_group_size = MTL::Size::Make(1, 1, 1);
    MTL::Size thread_group_count = MTL::Size::Make(width, height, 1);

    // We validate if the device supports non-uniform threadgroup sizes
    if (mNonUniformThreadgroupSupported) {
        compute_encoder->dispatchThreads(thread_group_count, thread_group_size);
    } else {
        compute_encoder->dispatchThreadgroups(thread_group_count, thread_group_size);
    }
    compute_encoder->endEncoding();

    // Use a completion handler to wait for the GPU to be done without blocking the thread
    command_buffer->addCompletedHandler([=](MTL::CommandBuffer* cmd_buffer) {
        // Now the converted pixel values can be copied from the buffer
        uint16_t* values = (uint16_t*)output_buffer->contents();
        memcpy(rgba16_buf, values, sizeof(uint16_t) * width * height);

        output_buffer->release();
    });

    command_buffer->commit();

    compute_pipeline_state->release();
    autorelease_pool->release();
}

void GfxRenderingAPIMetal::SetTextureFilter(FilteringMode mode) {
    mCurrentFilterMode = mode;
    gfx_texture_cache_clear();
}

FilteringMode GfxRenderingAPIMetal::GetTextureFilter() {
    return mCurrentFilterMode;
}

ImTextureID GfxRenderingAPIMetal::GetTextureById(int fb_id) {
    return (void*)mTextures[fb_id].texture;
}

void GfxRenderingAPIMetal::SetSrgbMode() {
    mSrgbMode = true;
}
} // namespace Fast

bool Metal_IsSupported() {
#ifdef __IOS__
    // iOS always supports Metal and MTLCopyAllDevices is not available
    return true;
#else
    NS::Array* devices = MTLCopyAllDevices();
    NS::UInteger count = devices->count();

    devices->release();

    return count > 0;
#endif
}

#endif
