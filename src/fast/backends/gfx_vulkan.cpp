#include "fast/backends/gfx_vulkan.h"

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "fast/backends/gfx_sdl.h"
#include "fast/interpreter.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

namespace Fast {
namespace {

constexpr uint32_t ImGuiDescriptorPoolSize = 64;
constexpr uint32_t PreferredBindlessTextureCount = 4096;

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

} // namespace

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
    if (rgba32Buf == nullptr || width == 0 || height == 0) {
        throw std::runtime_error("Cannot upload empty Vulkan texture");
    }

    uint32_t textureId = mCurrentTextureIds[mCurrentTile];
    VulkanTexture& texture = GetTexture(textureId);
    UploadTextureToGpu(texture, rgba32Buf, width, height);
    WriteBindlessTextureDescriptor(textureId);
}

void GfxRenderingAPIVulkan::SetSamplerParameters(int sampler, bool linearFilter, uint32_t cms, uint32_t cmt) {
    if (sampler < 0 || sampler >= SHADER_MAX_TEXTURES) {
        throw std::runtime_error("Invalid Vulkan sampler tile index");
    }

    VulkanTexture& texture = GetTexture(mCurrentTextureIds[sampler]);
    texture.linearFiltering = linearFilter;
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
    CreateAllocator();
    CreateTextureDescriptorResources();
    CreateSwapchain();
    CreateImageViews();
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
    if (mSwapchain == VK_NULL_HANDLE || mFrameActive) {
        return;
    }

    mCurrentFrame = &mFrames[mCurrentFrameIndex];
    if (mCurrentFrame->renderFinishedTimelineValue > 0) {
        VkSemaphoreWaitInfo waitInfo = {};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &mCurrentFrame->renderFinishedTimelineSemaphore;
        waitInfo.pValues = &mCurrentFrame->renderFinishedTimelineValue;
        CheckVk(vkWaitSemaphores(mDevice, &waitInfo, UINT64_MAX), "Failed to wait for Vulkan frame timeline");
    }

    VkResult acquireResult = vkAcquireNextImageKHR(
        mDevice, mSwapchain, UINT64_MAX, mCurrentFrame->imageAvailableSemaphore, VK_NULL_HANDLE, &mCurrentImageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        mCurrentFrame = nullptr;
        RecreateSwapchain();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire Vulkan swapchain image");
    }

    mCurrentCommandBuffer = mCurrentFrame->commandBuffer;
    vkResetCommandBuffer(mCurrentCommandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(mCurrentCommandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin Vulkan command buffer");
    }

    VkImageMemoryBarrier2 colorAttachmentBarrier = {};
    colorAttachmentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    colorAttachmentBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorAttachmentBarrier.srcAccessMask = VK_ACCESS_2_NONE;
    colorAttachmentBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorAttachmentBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    colorAttachmentBarrier.oldLayout = mSwapchainImageLayouts[mCurrentImageIndex];
    colorAttachmentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentBarrier.image = mSwapchainImages[mCurrentImageIndex];
    colorAttachmentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorAttachmentBarrier.subresourceRange.baseMipLevel = 0;
    colorAttachmentBarrier.subresourceRange.levelCount = 1;
    colorAttachmentBarrier.subresourceRange.baseArrayLayer = 0;
    colorAttachmentBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo colorAttachmentDependency = {};
    colorAttachmentDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    colorAttachmentDependency.imageMemoryBarrierCount = 1;
    colorAttachmentDependency.pImageMemoryBarriers = &colorAttachmentBarrier;
    vkCmdPipelineBarrier2(mCurrentCommandBuffer, &colorAttachmentDependency);

    VkClearValue clearValue = {};
    clearValue.color = { { 0.02f, 0.02f, 0.04f, 1.0f } };

    VkRenderingAttachmentInfo colorAttachment = {};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = mSwapchainImageViews[mCurrentImageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearValue;

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = { 0, 0 };
    renderingInfo.renderArea.extent = mSwapchainExtent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    vkCmdBeginRendering(mCurrentCommandBuffer, &renderingInfo);

    mSwapchainImageLayouts[mCurrentImageIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    mFrameActive = true;
}

void GfxRenderingAPIVulkan::EndFrame() {
}

void GfxRenderingAPIVulkan::FinishRender() {
    if (mSwapchain == VK_NULL_HANDLE || !mFrameActive) {
        return;
    }

    vkCmdEndRendering(mCurrentCommandBuffer);

    VkImageMemoryBarrier2 presentBarrier = {};
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    presentBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    presentBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    presentBarrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
    presentBarrier.dstAccessMask = VK_ACCESS_2_NONE;
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.image = mSwapchainImages[mCurrentImageIndex];
    presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    presentBarrier.subresourceRange.baseMipLevel = 0;
    presentBarrier.subresourceRange.levelCount = 1;
    presentBarrier.subresourceRange.baseArrayLayer = 0;
    presentBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo presentDependency = {};
    presentDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    presentDependency.imageMemoryBarrierCount = 1;
    presentDependency.pImageMemoryBarriers = &presentBarrier;
    vkCmdPipelineBarrier2(mCurrentCommandBuffer, &presentDependency);

    if (vkEndCommandBuffer(mCurrentCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record Vulkan command buffer");
    }
    mSwapchainImageLayouts[mCurrentImageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    mFrameActive = false;

    VkCommandBufferSubmitInfo commandBufferInfo = {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferInfo.commandBuffer = mCurrentCommandBuffer;

    VkSemaphoreSubmitInfo waitSemaphoreInfo = {};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.semaphore = mCurrentFrame->imageAvailableSemaphore;
    waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    mCurrentFrame->renderFinishedTimelineValue++;

    std::array<VkSemaphoreSubmitInfo, 2> signalSemaphoreInfos = {};
    signalSemaphoreInfos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfos[0].semaphore = mCurrentFrame->renderFinishedSemaphore;
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

    if (vkQueueSubmit2(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit Vulkan command buffer");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &mCurrentFrame->renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &mSwapchain;
    presentInfo.pImageIndices = &mCurrentImageIndex;

    VkResult presentResult = vkQueuePresentKHR(mPresentQueue, &presentInfo);
    mCurrentCommandBuffer = VK_NULL_HANDLE;
    mCurrentFrame = nullptr;
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % FRAMES_IN_FLIGHT;
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
    mFramebuffers.push_back(static_cast<uint32_t>(mFramebuffers.size()));
    return static_cast<int>(mFramebuffers.size() - 1);
}

void GfxRenderingAPIVulkan::UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                                        bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                                        bool can_extract_depth) {
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
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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
        if (frame.renderFinishedSemaphore == VK_NULL_HANDLE &&
            vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan render-finished semaphore");
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
        if (frame.renderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(mDevice, frame.renderFinishedSemaphore, nullptr);
            frame.renderFinishedSemaphore = VK_NULL_HANDLE;
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

VkCommandBuffer GfxRenderingAPIVulkan::BeginImmediateCommands() {
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

    CheckVk(vkQueueSubmit2(mGraphicsQueue, 1, &submitInfo, fence), "Failed to submit Vulkan immediate command buffer");
    CheckVk(vkWaitForFences(mDevice, 1, &fence, VK_TRUE, UINT64_MAX), "Failed to wait for Vulkan immediate fence");
    vkDestroyFence(mDevice, fence, nullptr);
    vkFreeCommandBuffers(mDevice, mUploadCommandPool, 1, &commandBuffer);
}

VulkanTexture& GfxRenderingAPIVulkan::GetTexture(uint32_t textureId) {
    if (textureId >= mTextures.size()) {
        throw std::runtime_error("Vulkan texture id does not exist");
    }
    return mTextures[textureId];
}

void GfxRenderingAPIVulkan::UploadTextureToGpu(VulkanTexture& texture, const uint8_t* rgba32Buf, uint32_t width,
                                               uint32_t height) {
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
    CheckVk(vmaCreateBuffer(mAllocator, &bufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation,
                            &stagingInfo),
            "Failed to create Vulkan texture staging buffer");
    std::memcpy(stagingInfo.pMappedData, rgba32Buf, static_cast<size_t>(uploadSize));

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
    CheckVk(vmaCreateImage(mAllocator, &imageInfo, &imageAllocInfo, &texture.image, &texture.allocation, nullptr),
            "Failed to create Vulkan texture image");

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
    vmaDestroyBuffer(mAllocator, stagingBuffer, stagingAllocation);

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

void GfxRenderingAPIVulkan::CleanupSwapchain() {
    if (mDevice == VK_NULL_HANDLE) {
        return;
    }

    for (auto imageView : mSwapchainImageViews) {
        vkDestroyImageView(mDevice, imageView, nullptr);
    }
    mSwapchainImageViews.clear();

    if (mSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
        mSwapchain = VK_NULL_HANDLE;
    }

    mSwapchainImages.clear();
    mSwapchainImageLayouts.clear();
    mSwapchainImageFormat = VK_FORMAT_UNDEFINED;
    mSwapchainExtent = {};
    mCurrentCommandBuffer = VK_NULL_HANDLE;
    mCurrentFrame = nullptr;
    mFrameActive = false;
}

void GfxRenderingAPIVulkan::RecreateSwapchain() {
    vkDeviceWaitIdle(mDevice);

    bool restoreImGui = mImGuiInitialized;
    ShutdownImGui();
    CleanupSwapchain();
    CreateSwapchain();
    CreateImageViews();
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
        DestroyTextures();
        DestroyTextureDescriptorResources();
        DestroyFrameResources();
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
