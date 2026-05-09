#include "fast/backends/gfx_vulkan.h"

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "fast/backends/gfx_sdl.h"
#include "fast/interpreter.h"

#include <cstring>
#include <stdexcept>

namespace Fast {

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
}

ShaderProgram* GfxRenderingAPIVulkan::CreateAndLoadNewShader(uint64_t shaderId0, uint32_t shaderId1) {
    auto key = std::make_pair(shaderId0, shaderId1);
    auto [it, _] = mShaderProgramPool.emplace(key, std::make_unique<VulkanShaderProgram>());
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
    return mNextTextureId++;
}

void GfxRenderingAPIVulkan::SelectTexture(int tile, uint32_t textureId) {
}

void GfxRenderingAPIVulkan::UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) {
}

void GfxRenderingAPIVulkan::SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) {
}

void GfxRenderingAPIVulkan::SetDepthTestAndMask(bool depth_test, bool z_upd) {
}

void GfxRenderingAPIVulkan::SetZmodeDecal(bool decal) {
}

void GfxRenderingAPIVulkan::SetViewport(int x, int y, int width, int height) {
}

void GfxRenderingAPIVulkan::SetScissor(int x, int y, int width, int height) {
}

void GfxRenderingAPIVulkan::SetUseAlpha(bool useAlpha) {
}

void GfxRenderingAPIVulkan::DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
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

    CreateFramebuffer();
}

void GfxRenderingAPIVulkan::OnResize() {
}

void GfxRenderingAPIVulkan::StartFrame() {
}

void GfxRenderingAPIVulkan::EndFrame() {
}

void GfxRenderingAPIVulkan::FinishRender() {
}

int GfxRenderingAPIVulkan::CreateFramebuffer() {
    mFramebuffers.push_back(static_cast<uint32_t>(mFramebuffers.size()));
    return static_cast<int>(mFramebuffers.size() - 1);
}

void GfxRenderingAPIVulkan::UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height,
                                                        uint32_t msaa_level, bool opengl_invertY, bool render_target,
                                                        bool has_depth_buffer, bool can_extract_depth) {
}

void GfxRenderingAPIVulkan::StartDrawToFramebuffer(int fbId, float noiseScale) {
}

void GfxRenderingAPIVulkan::CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1,
                                            int dstX0, int dstY0, int dstX1, int dstY1) {
}

void GfxRenderingAPIVulkan::ClearFramebuffer(bool color, bool depth) {
}

void GfxRenderingAPIVulkan::ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) {
    if (rgba16Buf != nullptr) {
        std::memset(rgba16Buf, 0, width * height * sizeof(uint16_t));
    }
}

void GfxRenderingAPIVulkan::ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) {
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
    return nullptr;
}

void GfxRenderingAPIVulkan::SelectTextureFb(int fbId) {
}

void GfxRenderingAPIVulkan::DeleteTexture(uint32_t texId) {
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
    return nullptr;
}

void GfxRenderingAPIVulkan::DestroyVulkanObjects() {
    if (mDevice != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(mDevice);
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
