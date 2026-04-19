#ifdef ENABLE_VULKAN

#include "fast/backends/gfx_vulkan.h"

#include <spdlog/spdlog.h>

// ImGui Vulkan backend
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>

namespace Fast {

// ---------------------------------------------------------------------------
// Helper: check VkResult and abort on error
// ---------------------------------------------------------------------------
static void VK_CHECK(VkResult result, const char* msg) {
    if (result != VK_SUCCESS) {
        SPDLOG_CRITICAL("Vulkan error ({}): result={}", msg, static_cast<int>(result));
        std::abort();
    }
}

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------
GfxRenderingAPIVulkan::~GfxRenderingAPIVulkan() {
    if (mDevice == VK_NULL_HANDLE) {
        return;
    }
    vkDeviceWaitIdle(mDevice);

    ImGui_ImplVulkan_Shutdown();

    CleanupSwapchain();

    if (mImGuiDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(mDevice, mImGuiDescriptorPool, nullptr);
    }
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(mDevice, mRenderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(mDevice, mImageAvailableSemaphores[i], nullptr);
        vkDestroyFence(mDevice, mInFlightFences[i], nullptr);
    }
    if (mCommandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
    }
    if (mRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
    }
    vkDestroyDevice(mDevice, nullptr);
    if (mSurface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    }
    vkDestroyInstance(mInstance, nullptr);
}

// ---------------------------------------------------------------------------
// VulkanInit — called from Gui::ImGuiBackendInit() with the SDL_Window*
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::VulkanInit(SDL_Window* window) {
    mSdlWindow = window;

    CreateInstance(window);
    CreateSurface(window);
    PickPhysicalDevice();
    CreateLogicalDevice();

    int w = 0;
    int h = 0;
    SDL_Vulkan_GetDrawableSize(window, &w, &h);
    CreateSwapchain(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    CreateSwapchainImageViews();
    CreateRenderPass();
    CreateFramebuffersForSwapchain();
    CreateCommandPoolAndBuffers();
    CreateSyncObjects();
    InitImGuiVulkanBackend();
}

// ---------------------------------------------------------------------------
// CreateInstance
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::CreateInstance(SDL_Window* window) {
    // Gather extensions required by SDL Vulkan
    uint32_t extCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr);
    std::vector<const char*> extensions(extCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extCount, extensions.data());

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "libultraship";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Fast3D";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &mInstance), "vkCreateInstance");
}

// ---------------------------------------------------------------------------
// CreateSurface
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::CreateSurface(SDL_Window* window) {
    if (SDL_Vulkan_CreateSurface(window, mInstance, &mSurface) != SDL_TRUE) {
        SPDLOG_CRITICAL("SDL_Vulkan_CreateSurface failed: {}", SDL_GetError());
        std::abort();
    }
}

// ---------------------------------------------------------------------------
// PickPhysicalDevice
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::PickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(mInstance, &count, nullptr);
    if (count == 0) {
        SPDLOG_CRITICAL("No Vulkan-capable GPU found");
        std::abort();
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(mInstance, &count, devices.data());

    // Prefer a discrete GPU; fall back to the first available device
    for (auto dev : devices) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(dev, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            mPhysicalDevice = dev;
            return;
        }
    }
    mPhysicalDevice = devices[0];
}

// ---------------------------------------------------------------------------
// CreateLogicalDevice
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::CreateLogicalDevice() {
    // Find graphics and present queue families
    uint32_t qfCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &qfCount, nullptr);
    std::vector<VkQueueFamilyProperties> qfProps(qfCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &qfCount, qfProps.data());

    std::optional<uint32_t> gfx, present;
    for (uint32_t i = 0; i < qfCount; i++) {
        if (qfProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            gfx = i;
        }
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, mSurface, &presentSupport);
        if (presentSupport) {
            present = i;
        }
        if (gfx.has_value() && present.has_value()) {
            break;
        }
    }

    if (!gfx.has_value() || !present.has_value()) {
        SPDLOG_CRITICAL("Could not find suitable Vulkan queue families");
        std::abort();
    }
    mGraphicsFamily = gfx.value();
    mPresentFamily = present.value();

    std::set<uint32_t> uniqueFamilies = { mGraphicsFamily, mPresentFamily };
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float priority = 1.0f;
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo qi{};
        qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = family;
        qi.queueCount = 1;
        qi.pQueuePriorities = &priority;
        queueCreateInfos.push_back(qi);
    }

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    VK_CHECK(vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice), "vkCreateDevice");
    vkGetDeviceQueue(mDevice, mGraphicsFamily, 0, &mGraphicsQueue);
    vkGetDeviceQueue(mDevice, mPresentFamily, 0, &mPresentQueue);
}

// ---------------------------------------------------------------------------
// CreateSwapchain
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::CreateSwapchain(uint32_t width, uint32_t height) {
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &caps);

    // Choose surface format (prefer BGRA8 SRGB)
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, formats.data());

    VkSurfaceFormatKHR chosenFormat = formats[0];
    for (auto& fmt : formats) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
            fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFormat = fmt;
            break;
        }
    }
    mSwapchainFormat = chosenFormat.format;

    // Choose present mode (prefer mailbox, fall back to FIFO)
    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &modeCount, nullptr);
    std::vector<VkPresentModeKHR> modes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &modeCount, modes.data());

    VkPresentModeKHR chosenMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto m : modes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
            chosenMode = m;
            break;
        }
    }

    // Choose extent
    if (caps.currentExtent.width != UINT32_MAX) {
        mSwapchainExtent = caps.currentExtent;
    } else {
        mSwapchainExtent.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
        mSwapchainExtent.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0) {
        imageCount = std::min(imageCount, caps.maxImageCount);
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = mSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = chosenFormat.format;
    createInfo.imageColorSpace = chosenFormat.colorSpace;
    createInfo.imageExtent = mSwapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t families[2] = { mGraphicsFamily, mPresentFamily };
    if (mGraphicsFamily != mPresentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = families;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = chosenMode;
    createInfo.clipped = VK_TRUE;

    VK_CHECK(vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapchain), "vkCreateSwapchainKHR");

    uint32_t cnt = 0;
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &cnt, nullptr);
    mSwapchainImages.resize(cnt);
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &cnt, mSwapchainImages.data());

    mImagesInFlight.assign(cnt, VK_NULL_HANDLE);
}

// ---------------------------------------------------------------------------
// CreateSwapchainImageViews
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::CreateSwapchainImageViews() {
    mSwapchainImageViews.resize(mSwapchainImages.size());
    for (size_t i = 0; i < mSwapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = mSwapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = mSwapchainFormat;
        createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                  VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(mDevice, &createInfo, nullptr, &mSwapchainImageViews[i]),
                 "vkCreateImageView");
    }
}

// ---------------------------------------------------------------------------
// CreateRenderPass
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::CreateRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = mSwapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dep;

    VK_CHECK(vkCreateRenderPass(mDevice, &createInfo, nullptr, &mRenderPass), "vkCreateRenderPass");
}

// ---------------------------------------------------------------------------
// CreateFramebuffersForSwapchain
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::CreateFramebuffersForSwapchain() {
    mSwapchainFramebuffers.resize(mSwapchainImageViews.size());
    for (size_t i = 0; i < mSwapchainImageViews.size(); i++) {
        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = mRenderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &mSwapchainImageViews[i];
        createInfo.width = mSwapchainExtent.width;
        createInfo.height = mSwapchainExtent.height;
        createInfo.layers = 1;
        VK_CHECK(vkCreateFramebuffer(mDevice, &createInfo, nullptr, &mSwapchainFramebuffers[i]),
                 "vkCreateFramebuffer");
    }
}

// ---------------------------------------------------------------------------
// CreateCommandPoolAndBuffers
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::CreateCommandPoolAndBuffers() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = mGraphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool), "vkCreateCommandPool");

    mCommandBuffers.resize(mSwapchainImages.size());
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(mCommandBuffers.size());
    VK_CHECK(vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()),
             "vkAllocateCommandBuffers");
}

// ---------------------------------------------------------------------------
// CreateSyncObjects
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::CreateSyncObjects() {
    VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkCreateSemaphore(mDevice, &semInfo, nullptr, &mImageAvailableSemaphores[i]),
                 "vkCreateSemaphore imageAvailable");
        VK_CHECK(vkCreateSemaphore(mDevice, &semInfo, nullptr, &mRenderFinishedSemaphores[i]),
                 "vkCreateSemaphore renderFinished");
        VK_CHECK(vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFences[i]), "vkCreateFence");
    }
}

// ---------------------------------------------------------------------------
// InitImGuiVulkanBackend
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::InitImGuiVulkanBackend() {
    // Descriptor pool large enough for ImGui textures
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 100 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 },
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;
    VK_CHECK(vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mImGuiDescriptorPool),
             "vkCreateDescriptorPool for ImGui");

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = mInstance;
    initInfo.PhysicalDevice = mPhysicalDevice;
    initInfo.Device = mDevice;
    initInfo.QueueFamily = mGraphicsFamily;
    initInfo.Queue = mGraphicsQueue;
    initInfo.DescriptorPool = mImGuiDescriptorPool;
    initInfo.RenderPass = mRenderPass;
    initInfo.MinImageCount = static_cast<uint32_t>(mSwapchainImages.size());
    initInfo.ImageCount = static_cast<uint32_t>(mSwapchainImages.size());
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo);

    // Upload ImGui fonts to the GPU
    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(mCommandBuffers[0], &beginInfo);
    ImGui_ImplVulkan_CreateFontsTexture();
    vkEndCommandBuffer(mCommandBuffers[0]);

    VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffers[0];
    vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(mGraphicsQueue);
}

// ---------------------------------------------------------------------------
// CleanupSwapchain / RecreateSwapchain
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::CleanupSwapchain() {
    for (auto fb : mSwapchainFramebuffers) {
        vkDestroyFramebuffer(mDevice, fb, nullptr);
    }
    mSwapchainFramebuffers.clear();
    for (auto iv : mSwapchainImageViews) {
        vkDestroyImageView(mDevice, iv, nullptr);
    }
    mSwapchainImageViews.clear();
    if (mSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
        mSwapchain = VK_NULL_HANDLE;
    }
}

void GfxRenderingAPIVulkan::RecreateSwapchain() {
    vkDeviceWaitIdle(mDevice);

    int w = 0;
    int h = 0;
    SDL_Vulkan_GetDrawableSize(mSdlWindow, &w, &h);
    if (w == 0 || h == 0) {
        return; // minimised
    }

    CleanupSwapchain();
    CreateSwapchain(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    CreateSwapchainImageViews();
    CreateFramebuffersForSwapchain();
    mImagesInFlight.assign(mSwapchainImages.size(), VK_NULL_HANDLE);

    ImGui_ImplVulkan_SetMinImageCount(static_cast<uint32_t>(mSwapchainImages.size()));
    mNeedsSwapchainRecreation = false;
}

// ---------------------------------------------------------------------------
// GfxRenderingAPI::Init (called after VulkanInit from Interpreter::Init)
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::Init() {
    // Framebuffer 0 = the screen / swapchain target.  Reserve it.
    mFramebuffers.push_back({});
    mRdpWords.reserve(4096); // pre-allocate for typical frame RDP command count
}

void GfxRenderingAPIVulkan::OnResize() {
    mNeedsSwapchainRecreation = true;
}

// ---------------------------------------------------------------------------
// Frame lifecycle
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::StartFrame() {
    // Clear the RDP command staging buffer at the start of each frame.
    mRdpWords.clear();

    if (mNeedsSwapchainRecreation) {
        RecreateSwapchain();
    }

    vkWaitForFences(mDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(mDevice, mSwapchain, UINT64_MAX,
                                            mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE,
                                            &mCurrentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    }

    if (mImagesInFlight[mCurrentImageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(mDevice, 1, &mImagesInFlight[mCurrentImageIndex], VK_TRUE, UINT64_MAX);
    }
    mImagesInFlight[mCurrentImageIndex] = mInFlightFences[mCurrentFrame];
    vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrame]);

    VkCommandBuffer cmd = mCommandBuffers[mCurrentImageIndex];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo), "vkBeginCommandBuffer StartFrame");

    VkClearValue clearVal{};
    clearVal.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = mRenderPass;
    rpInfo.framebuffer = mSwapchainFramebuffers[mCurrentImageIndex];
    rpInfo.renderArea.extent = mSwapchainExtent;
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearVal;
    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    mFrameStarted = true;
}

void GfxRenderingAPIVulkan::EndFrame() {
    if (!mFrameStarted) {
        return;
    }
    VkCommandBuffer cmd = mCommandBuffers[mCurrentImageIndex];
    vkCmdEndRenderPass(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd), "vkEndCommandBuffer EndFrame");
    mFrameStarted = false;
}

void GfxRenderingAPIVulkan::FinishRender() {
    VkCommandBuffer cmd = mCommandBuffers[mCurrentImageIndex];
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &mImageAvailableSemaphores[mCurrentFrame];
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &mRenderFinishedSemaphores[mCurrentFrame];
    VK_CHECK(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrame]),
             "vkQueueSubmit FinishRender");

    VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[mCurrentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &mSwapchain;
    presentInfo.pImageIndices = &mCurrentImageIndex;

    VkResult result = vkQueuePresentKHR(mPresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        mNeedsSwapchainRecreation = true;
    }

    mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// ---------------------------------------------------------------------------
// ImGui helpers (called from Gui)
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::NewFrame() {
    ImGui_ImplVulkan_NewFrame();
}

void GfxRenderingAPIVulkan::RenderDrawData(ImDrawData* data) {
    VkCommandBuffer cmd = mCommandBuffers[mCurrentImageIndex];
    ImGui_ImplVulkan_RenderDrawData(data, cmd);
}

// ---------------------------------------------------------------------------
// Framebuffer management (stubs — game-scene FBs for PRDP, phase 2)
// ---------------------------------------------------------------------------
int GfxRenderingAPIVulkan::CreateFramebuffer() {
    int id = static_cast<int>(mFramebuffers.size());
    mFramebuffers.push_back({});
    return id;
}

void GfxRenderingAPIVulkan::UpdateFramebufferParameters(int /*fbId*/, uint32_t /*width*/, uint32_t /*height*/,
                                                        uint32_t /*msaaLevel*/, bool /*oglInvertY*/,
                                                        bool /*renderTarget*/, bool /*hasDepthBuffer*/,
                                                        bool /*canExtractDepth*/) {
    // TODO (PRDP phase 2): allocate or resize off-screen Vulkan images.
}

void GfxRenderingAPIVulkan::StartDrawToFramebuffer(int fbId, float /*noiseScale*/) {
    mCurrentFbId = fbId;
    // TODO (PRDP phase 2): bind off-screen framebuffer.
}

void GfxRenderingAPIVulkan::ClearFramebuffer(bool /*color*/, bool /*depth*/) {
    // TODO (PRDP phase 2): issue vkCmdClearAttachments.
}

void GfxRenderingAPIVulkan::CopyFramebuffer(int /*fbDstId*/, int /*fbSrcId*/, int, int, int, int, int, int,
                                            int, int) {
    // TODO (PRDP phase 2): vkCmdBlitImage.
}

void GfxRenderingAPIVulkan::ReadFramebufferToCPU(int /*fbId*/, uint32_t /*width*/, uint32_t /*height*/,
                                                  uint16_t* /*rgba16Buf*/) {
    // TODO (PRDP phase 2): vkCmdCopyImageToBuffer + readback.
}

void GfxRenderingAPIVulkan::ResolveMSAAColorBuffer(int /*fbIdTarget*/, int /*fbIdSrc*/) {
    // TODO (PRDP phase 2).
}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
GfxRenderingAPIVulkan::GetPixelDepth(int /*fbId*/, const std::set<std::pair<float, float>>& /*coordinates*/) {
    // TODO (PRDP phase 2): return depth values from the depth buffer.
    return {};
}

void* GfxRenderingAPIVulkan::GetFramebufferTextureId(int /*fbId*/) {
    // TODO (PRDP phase 2): return ImTextureID for the resolved colour image.
    return nullptr;
}

void GfxRenderingAPIVulkan::SelectTextureFb(int fbId) {
    mCurrentFbId = fbId;
}

// ---------------------------------------------------------------------------
// Shader program management (stubs — PRDP uses its own pipeline)
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::UnloadShader(ShaderProgram* /*oldPrg*/) {}
void GfxRenderingAPIVulkan::LoadShader(ShaderProgram* newPrg) {
    mCurrentShader = reinterpret_cast<ShaderProgramVulkan*>(newPrg);
}
void GfxRenderingAPIVulkan::ClearShaderCache() {
    mShaderPool.clear();
    mCurrentShader = nullptr;
}

ShaderProgram* GfxRenderingAPIVulkan::CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) {
    ShaderProgramVulkan prg{};
    prg.shaderId0 = shaderId0;
    prg.shaderId1 = shaderId1;
    prg.numInputs = 0;
    mShaderPool[{ shaderId0, shaderId1 }] = prg;
    mCurrentShader = &mShaderPool[{ shaderId0, shaderId1 }];
    return reinterpret_cast<ShaderProgram*>(mCurrentShader);
}

ShaderProgram* GfxRenderingAPIVulkan::LookupShader(uint64_t shaderId0, uint64_t shaderId1) {
    auto it = mShaderPool.find({ shaderId0, shaderId1 });
    return (it != mShaderPool.end()) ? reinterpret_cast<ShaderProgram*>(&it->second) : nullptr;
}

void GfxRenderingAPIVulkan::ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) {
    if (prg) {
        auto* vkPrg = reinterpret_cast<ShaderProgramVulkan*>(prg);
        *numInputs = vkPrg->numInputs;
        usedTextures[0] = vkPrg->usedTextures[0];
        usedTextures[1] = vkPrg->usedTextures[1];
    }
}

// ---------------------------------------------------------------------------
// Texture management (stubs — PRDP phase 2)
// ---------------------------------------------------------------------------
uint32_t GfxRenderingAPIVulkan::NewTexture() {
    return mNextTextureId++;
}

void GfxRenderingAPIVulkan::SelectTexture(int /*tile*/, uint32_t /*textureId*/) {}
void GfxRenderingAPIVulkan::UploadTexture(const uint8_t* /*rgba32Buf*/, uint32_t /*width*/,
                                           uint32_t /*height*/) {
    // TODO (PRDP phase 2): upload to VkImage.
}
void GfxRenderingAPIVulkan::SetSamplerParameters(int /*sampler*/, bool /*linearFilter*/,
                                                  uint32_t /*cms*/, uint32_t /*cmt*/) {}
void GfxRenderingAPIVulkan::DeleteTexture(uint32_t /*texId*/) {}

// ---------------------------------------------------------------------------
// Render state — track values so DrawTriangles can build PRDP commands
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::SetDepthTestAndMask(bool depthTest, bool zUpd) {
    mDepthTest = depthTest;
    mDepthMask = zUpd;
}
void GfxRenderingAPIVulkan::SetZmodeDecal(bool decal) {
    mDecal = decal;
}
void GfxRenderingAPIVulkan::SetViewport(int x, int y, int width, int height) {
    mVpX = x;
    mVpY = y;
    mVpW = width;
    mVpH = height;
}
void GfxRenderingAPIVulkan::SetScissor(int x, int y, int width, int height) {
    mScX = x;
    mScY = y;
    mScW = width;
    mScH = height;
}
void GfxRenderingAPIVulkan::SetUseAlpha(bool useAlpha) {
    mUseAlpha = useAlpha;
}

// ---------------------------------------------------------------------------
// EncodeShadeTriangle — convert one post-RSP triangle into N64 RDP
// SHADE_TRIANGLE (opcode 0xC8) command words and append to mRdpWords.
//
// The VBO layout (per vertex, floatsPerVertex floats total):
//   [0..3]  x, y, z, w   (clip space; w≠0 so NDC = x/w, y/w)
//   [colourOffset..+3]    r, g, b, a   (if useColour, in [0,1])
//
// Screen-space conversion (N64 convention, y-up in NDC, y-down on screen):
//   sx = ndcX * (vpW/2) + (vpX + vpW/2)
//   sy = -ndcY * (vpH/2) + (vpY + vpH/2)      ← flip Y
//
// Returns number of uint64_t words appended (8 for a SHADE_TRIANGLE).
// ---------------------------------------------------------------------------
int GfxRenderingAPIVulkan::EncodeShadeTriangle(const float* v0, const float* v1, const float* v2,
                                                int /*floatsPerVertex*/, bool useColour, int colourOffset) {
    // Convert clip → NDC → screen (pixel, y-down).
    auto toScreen = [&](const float* v, float& sx, float& sy) {
        float winv = (fabsf(v[3]) > 1e-6f) ? (1.0f / v[3]) : 0.0f;
        float ndcX = v[0] * winv;
        float ndcY = v[1] * winv;
        sx = ndcX * (mVpW * 0.5f) + (mVpX + mVpW * 0.5f);
        sy = -ndcY * (mVpH * 0.5f) + (mVpY + mVpH * 0.5f);
    };

    float sx0, sy0, sx1, sy1, sx2, sy2;
    toScreen(v0, sx0, sy0);
    toScreen(v1, sx1, sy1);
    toScreen(v2, sx2, sy2);

    // Default shade = white (fully opaque) when no colour inputs present.
    float r0 = 1, g0 = 1, b0 = 1, a0 = 1;
    float r1 = 1, g1 = 1, b1 = 1, a1 = 1;
    float r2 = 1, g2 = 1, b2 = 1, a2 = 1;
    if (useColour) {
        r0 = v0[colourOffset + 0]; g0 = v0[colourOffset + 1];
        b0 = v0[colourOffset + 2]; a0 = v0[colourOffset + 3];
        r1 = v1[colourOffset + 0]; g1 = v1[colourOffset + 1];
        b1 = v1[colourOffset + 2]; a1 = v1[colourOffset + 3];
        r2 = v2[colourOffset + 0]; g2 = v2[colourOffset + 1];
        b2 = v2[colourOffset + 2]; a2 = v2[colourOffset + 3];
    }

    // Sort vertices by Y (ascending = top-to-bottom on screen).
    struct V { float x, y, r, g, b, a; };
    V verts[3] = {
        { sx0, sy0, r0, g0, b0, a0 },
        { sx1, sy1, r1, g1, b1, a1 },
        { sx2, sy2, r2, g2, b2, a2 },
    };
    if (verts[0].y > verts[1].y) std::swap(verts[0], verts[1]);
    if (verts[1].y > verts[2].y) std::swap(verts[1], verts[2]);
    if (verts[0].y > verts[1].y) std::swap(verts[0], verts[1]);

    // 10.2 fixed-point Y values.
    auto toY10_2  = [](float y) { return static_cast<int16_t>(y * 4.0f); };
    auto toX16_16 = [](float x) { return static_cast<int32_t>(x * 65536.0f); };

    int16_t YH = toY10_2(verts[0].y);  // top
    int16_t YM = toY10_2(verts[1].y);  // middle
    int16_t YL = toY10_2(verts[2].y);  // bottom

    float dy_hm = verts[1].y - verts[0].y;
    float dy_lh = verts[2].y - verts[0].y;
    float dy_ml = verts[2].y - verts[1].y;

    float DXHDy = (dy_lh > 0.0f) ? (verts[2].x - verts[0].x) / dy_lh : 0.0f;
    float DXMDy = (dy_hm > 0.0f) ? (verts[1].x - verts[0].x) / dy_hm : 0.0f;
    float DXLDy = (dy_ml > 0.0f) ? (verts[2].x - verts[1].x) / dy_ml : 0.0f;

    float xH_at_YM = verts[0].x + DXHDy * dy_hm;
    bool right_major = (xH_at_YM > verts[1].x);

    // Shade derivatives via 2-D linear solve over the triangle.
    float ex = verts[1].x - verts[0].x, ey = dy_hm;
    float fx = verts[2].x - verts[0].x, fy = dy_lh;
    float det = ex * fy - ey * fx;

    auto deriv = [&](float c0, float c1, float c2, float& dcdx, float& dcdy) {
        float dc_e = c1 - c0, dc_f = c2 - c0;
        if (fabsf(det) > 1e-6f) {
            dcdx = (dc_e * fy - dc_f * ey) / det;
            dcdy = (dc_f * ex - dc_e * fx) / det;
        } else {
            dcdx = dcdy = 0.0f;
        }
    };

    float dRdx, dRdy, dGdx, dGdy, dBdx, dBdy, dAdx, dAdy;
    deriv(verts[0].r, verts[1].r, verts[2].r, dRdx, dRdy);
    deriv(verts[0].g, verts[1].g, verts[2].g, dGdx, dGdy);
    deriv(verts[0].b, verts[1].b, verts[2].b, dBdx, dBdy);
    deriv(verts[0].a, verts[1].a, verts[2].a, dAdx, dAdy);

    // 8.16 fixed-point shade (N64 range 0-255 → multiply by 256 × 65536).
    constexpr float kSF = 256.0f * 65536.0f;
    auto toShade = [kSF](float c) { return static_cast<int32_t>(c * kSF); };

    // Word 0: opcode 0xC8 | right_major | level(0) | tile(0) | YL | YM | YH
    uint64_t w0 = (uint64_t(0xC8) << 56)
                | (uint64_t(right_major ? 1 : 0) << 55)
                | (uint64_t(uint16_t(YL)) << 32)
                | (uint64_t(uint16_t(YM)) << 16)
                | uint64_t(uint16_t(YH));

    // Words 1-3: edge coefficients XL/DXLDy, XH/DXHDy, XM/DXMDy.
    uint64_t w1 = (uint64_t(uint32_t(toX16_16(verts[1].x))) << 32)
                | uint64_t(uint32_t(toX16_16(DXLDy)));
    uint64_t w2 = (uint64_t(uint32_t(toX16_16(verts[0].x))) << 32)
                | uint64_t(uint32_t(toX16_16(DXHDy)));
    uint64_t w3 = (uint64_t(uint32_t(toX16_16(verts[0].x))) << 32)
                | uint64_t(uint32_t(toX16_16(DXMDy)));

    // Words 4-7: RGBA at top vertex + x-derivatives.
    uint64_t w4 = (uint64_t(uint32_t(toShade(verts[0].r))) << 32) | uint64_t(uint32_t(toShade(verts[0].g)));
    uint64_t w5 = (uint64_t(uint32_t(toShade(verts[0].b))) << 32) | uint64_t(uint32_t(toShade(verts[0].a)));
    uint64_t w6 = (uint64_t(uint32_t(toShade(dRdx))) << 32) | uint64_t(uint32_t(toShade(dGdx)));
    uint64_t w7 = (uint64_t(uint32_t(toShade(dBdx))) << 32) | uint64_t(uint32_t(toShade(dAdx)));

    mRdpWords.push_back(w0);
    mRdpWords.push_back(w1);
    mRdpWords.push_back(w2);
    mRdpWords.push_back(w3);
    mRdpWords.push_back(w4);
    mRdpWords.push_back(w5);
    mRdpWords.push_back(w6);
    mRdpWords.push_back(w7);
    return 8;
}

// ---------------------------------------------------------------------------
// DrawTriangles — translate the interpreter's VBO batch into RDP commands
// ---------------------------------------------------------------------------
// VBO layout (from GfxSpTri1 / Flush):
//   Each vertex occupies a variable number of floats:
//     [0..3]    x, y, z, w    (clip-space position; always present)
//     [4..5]    u0, v0        (texture 0 UVs; only when usedTextures[0])
//     optional: clamp max_u0, max_v0
//     [...]     u1, v1        (texture 1 UVs; only when usedTextures[1])
//     [...]     fog_r,g,b,factor  (if FOG shader opt)
//     [...]     grayscale r,g,b,a  (if GRAYSCALE shader opt)
//     [...]     r,g,b,a per shader input (numInputs × colour channels)
//
// For phase 2 the full texture/fog/grayscale decoding will be added; this
// implementation handles the common "position + flat/gouraud shade" path.
void GfxRenderingAPIVulkan::DrawTriangles(float* bufVbo, size_t bufVboLen, size_t bufVboNumTris) {
    if (bufVboNumTris == 0 || bufVboLen == 0) {
        return;
    }

    // Determine per-vertex stride from the current shader.
    int floatsPerVertex = 4; // x,y,z,w always
    bool useColour = false;
    int colourOffset = 4;
    uint8_t numInputs = 0;
    bool usedTex[2] = {};

    if (mCurrentShader) {
        numInputs = mCurrentShader->numInputs;
        usedTex[0] = mCurrentShader->usedTextures[0];
        usedTex[1] = mCurrentShader->usedTextures[1];

        int stride = 4; // position
        for (int t = 0; t < 2; t++) {
            if (usedTex[t]) {
                stride += 2; // u, v
                // NOTE: clamp values (max_u, max_v) are added by the interpreter
                // based on the texture tile flags — we approximate as not present
                // for the basic path.
            }
        }
        // Colour inputs follow textures.
        colourOffset = stride;
        if (numInputs > 0) {
            // Each input is 1 RGBA (4 floats).  The number of active channels
            // is 1 (no alpha) or 2 (alpha), but for shade we use the first input.
            useColour = true;
            stride += numInputs * 4;
        }
        floatsPerVertex = stride;
    }

    if (floatsPerVertex == 0) {
        return;
    }

    // Emit one SHADE_TRIANGLE per triangle.
    size_t vboIdx = 0;
    for (size_t tri = 0; tri < bufVboNumTris; tri++) {
        const float* v0 = bufVbo + vboIdx;
        const float* v1 = bufVbo + vboIdx + floatsPerVertex;
        const float* v2 = bufVbo + vboIdx + floatsPerVertex * 2;

        if (vboIdx + static_cast<size_t>(floatsPerVertex) * 3 > bufVboLen) {
            break; // guard against malformed buffer
        }

        EncodeShadeTriangle(v0, v1, v2, floatsPerVertex, useColour, colourOffset);

#ifdef ENABLE_PARALLEL_RDP
        // TODO (PRDP phase 2): submit mRdpWords to the CommandProcessor here.
        // Example (once CommandProcessor is instantiated):
        //   mPrdpCommandProcessor->enqueue_command(mRdpWords.size(), mRdpWords.data());
        //   mRdpWords.clear();
#endif
        vboIdx += static_cast<size_t>(floatsPerVertex) * 3;
    }
}

// ---------------------------------------------------------------------------
// Filter / SRGB
// ---------------------------------------------------------------------------
void GfxRenderingAPIVulkan::SetTextureFilter(FilteringMode mode) {
    mFilterMode = mode;
}
FilteringMode GfxRenderingAPIVulkan::GetTextureFilter() {
    return mFilterMode;
}
void GfxRenderingAPIVulkan::SetSrgbMode() {}

ImTextureID GfxRenderingAPIVulkan::GetTextureById(int /*id*/) {
    return nullptr;
}

} // namespace Fast
#endif // ENABLE_VULKAN
