#include "fast/backends/gfx_vulkan.h"

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "fast/backends/gfx_sdl.h"
#include "fast/interpreter.h"

#include <algorithm>
#include <array>
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
    CreateSwapchain();
    CreateImageViews();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();

    CreateFramebuffer();
}

void GfxRenderingAPIVulkan::OnResize() {
    mFramebufferResized = true;
}

void GfxRenderingAPIVulkan::StartFrame() {
}

void GfxRenderingAPIVulkan::EndFrame() {
}

void GfxRenderingAPIVulkan::FinishRender() {
    if (mSwapchain == VK_NULL_HANDLE) {
        return;
    }

    vkWaitForFences(mDevice, 1, &mInFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult acquireResult =
        vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX, mImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire Vulkan swapchain image");
    }

    vkResetFences(mDevice, 1, &mInFlightFence);
    vkResetCommandBuffer(mCommandBuffers[imageIndex], 0);
    RecordClearCommandBuffer(mCommandBuffers[imageIndex], imageIndex);

    VkCommandBufferSubmitInfo commandBufferInfo = {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferInfo.commandBuffer = mCommandBuffers[imageIndex];

    VkSemaphoreSubmitInfo waitSemaphoreInfo = {};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.semaphore = mImageAvailableSemaphore;
    waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT;

    VkSemaphoreSubmitInfo signalSemaphoreInfo = {};
    signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfo.semaphore = mRenderFinishedSemaphores[imageIndex];
    signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkSubmitInfo2 submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandBufferInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

    if (vkQueueSubmit2(mGraphicsQueue, 1, &submitInfo, mInFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit Vulkan clear command buffer");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[imageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &mSwapchain;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(mPresentQueue, &presentInfo);
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

void GfxRenderingAPIVulkan::CreateCommandBuffers() {
    mCommandBuffers.resize(mSwapchainImages.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(mCommandBuffers.size());

    if (vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate Vulkan command buffers");
    }
}

void GfxRenderingAPIVulkan::CreateSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (mImageAvailableSemaphore == VK_NULL_HANDLE &&
        vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan image-available semaphore");
    }

    if (mInFlightFence == VK_NULL_HANDLE &&
        vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan in-flight fence");
    }

    mRenderFinishedSemaphores.resize(mSwapchainImages.size(), VK_NULL_HANDLE);
    for (auto& renderFinishedSemaphore : mRenderFinishedSemaphores) {
        if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan swapchain synchronization objects");
        }
    }
}

void GfxRenderingAPIVulkan::CleanupSwapchainSyncObjects() {
    for (auto renderFinishedSemaphore : mRenderFinishedSemaphores) {
        if (renderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(mDevice, renderFinishedSemaphore, nullptr);
        }
    }
    mRenderFinishedSemaphores.clear();
}

void GfxRenderingAPIVulkan::RecordClearCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin Vulkan command buffer");
    }

    VkImageMemoryBarrier2 transferBarrier = {};
    transferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    transferBarrier.srcStageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT;
    transferBarrier.srcAccessMask = VK_ACCESS_2_NONE;
    transferBarrier.dstStageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    transferBarrier.oldLayout = mSwapchainImageLayouts[imageIndex];
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarrier.image = mSwapchainImages[imageIndex];
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

    VkClearColorValue clearColor = { { 0.02f, 0.02f, 0.04f, 1.0f } };
    VkImageSubresourceRange clearRange = {};
    clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    clearRange.baseMipLevel = 0;
    clearRange.levelCount = 1;
    clearRange.baseArrayLayer = 0;
    clearRange.layerCount = 1;
    vkCmdClearColorImage(commandBuffer, mSwapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor,
                         1, &clearRange);

    VkImageMemoryBarrier2 presentBarrier = {};
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    presentBarrier.srcStageMask = VK_PIPELINE_STAGE_2_CLEAR_BIT;
    presentBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    presentBarrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
    presentBarrier.dstAccessMask = VK_ACCESS_2_NONE;
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.image = mSwapchainImages[imageIndex];
    presentBarrier.subresourceRange = clearRange;

    VkDependencyInfo presentDependency = {};
    presentDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    presentDependency.imageMemoryBarrierCount = 1;
    presentDependency.pImageMemoryBarriers = &presentBarrier;
    vkCmdPipelineBarrier2(commandBuffer, &presentDependency);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record Vulkan command buffer");
    }

    mSwapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
}

void GfxRenderingAPIVulkan::CleanupSwapchain() {
    if (mDevice == VK_NULL_HANDLE) {
        return;
    }

    CleanupSwapchainSyncObjects();

    if (!mCommandBuffers.empty()) {
        vkFreeCommandBuffers(mDevice, mCommandPool, static_cast<uint32_t>(mCommandBuffers.size()),
                             mCommandBuffers.data());
        mCommandBuffers.clear();
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
}

void GfxRenderingAPIVulkan::RecreateSwapchain() {
    vkDeviceWaitIdle(mDevice);
    CleanupSwapchain();
    CreateSwapchain();
    CreateImageViews();
    CreateCommandBuffers();
    CreateSyncObjects();
}

void GfxRenderingAPIVulkan::DestroyVulkanObjects() {
    // TODO: use destructor queue

    if (mDevice != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(mDevice);
        CleanupSwapchain();
        if (mInFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(mDevice, mInFlightFence, nullptr);
            mInFlightFence = VK_NULL_HANDLE;
        }
        if (mImageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(mDevice, mImageAvailableSemaphore, nullptr);
            mImageAvailableSemaphore = VK_NULL_HANDLE;
        }
        if (mCommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
            mCommandPool = VK_NULL_HANDLE;
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
