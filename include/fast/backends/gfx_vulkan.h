#pragma once

#include "gfx_rendering_api.h"
#include "../interpreter.h"
#include "gfx_vulkan_utils.h"

#include <array>
#include <deque>
#include <map>
#include <memory>
#include <utility>
#include <vector>

struct ImDrawData;
struct VmaAllocation_T;
struct VmaAllocator_T;

namespace Fast {

class GfxWindowBackendSDL2;

constexpr uint32_t FRAMES_IN_FLIGHT = 3;

enum class VulkanImageUsage {
    ColorAttachment,
    DepthAttachment,
    ShaderRead,
    TransferSrc,
    TransferDst,
    Present,
};

struct VulkanShaderProgram {
    uint8_t numInputs = 0;
    bool usedTextures[SHADER_MAX_TEXTURES] = {};
    size_t numFloats = 0;
    VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

struct VulkanTexture {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation_T* allocation = nullptr;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorSet imguiDescriptorSet = VK_NULL_HANDLE;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t width = 0;
    uint32_t height = 0;
    bool uploaded = false;
    bool linearFiltering = false;
    uint32_t filtering = FILTER_THREE_POINT;
    uint32_t cms = 0;
    uint32_t cmt = 0;
};

struct VulkanFrame {
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedTimelineSemaphore = VK_NULL_HANDLE;
    uint64_t renderFinishedTimelineValue = 0;
};

struct VulkanFramebuffer {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t msaaLevel = 1;
    bool openglInvertY = false;
    bool renderTarget = false;
    bool hasDepthBuffer = false;
    bool canExtractDepth = false;
    uint32_t colorTextureId = UINT32_MAX;
    VkImage colorImage = VK_NULL_HANDLE;
    VmaAllocation_T* colorAllocation = nullptr;
    VkImageView colorImageView = VK_NULL_HANDLE;
    VkImageLayout colorLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags2 colorStageMask = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 colorAccessMask = VK_ACCESS_2_NONE;
    VkImage depthImage = VK_NULL_HANDLE;
    VmaAllocation_T* depthAllocation = nullptr;
    VkImageView depthImageView = VK_NULL_HANDLE;
    VkImageLayout depthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags2 depthStageMask = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 depthAccessMask = VK_ACCESS_2_NONE;
};

class VulkanVertexRingBuffer {
  public:
    struct Allocation {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceSize offset = 0;
        void* mapped = nullptr;
    };

    void Init(VkDevice device, VmaAllocator_T* allocator, VkDeviceSize initialSize);
    void Destroy();
    void BeginFrame();
    void EndFrame(VkSemaphore timelineSemaphore, uint64_t timelineValue);
    Allocation Allocate(VkDeviceSize size, VkDeviceSize alignment);

  private:
    struct Buffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation_T* allocation = nullptr;
        void* mapped = nullptr;
        VkDeviceSize size = 0;
        VkDeviceSize head = 0;
    };

    struct SubmittedRange {
        VkSemaphore timelineSemaphore = VK_NULL_HANDLE;
        uint64_t timelineValue = 0;
    };

    static VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment);
    Buffer CreateBuffer(VkDeviceSize size);
    void DestroyBuffer(Buffer& buffer);
    void Grow(VkDeviceSize requiredSize);
    bool CurrentBufferIsInUse() const;
    void CollectCompletedRanges();

    VkDevice mDevice = VK_NULL_HANDLE;
    VmaAllocator_T* mAllocator = nullptr;
    Buffer mCurrent;
    std::deque<Buffer> mRetiredBuffers;
    std::vector<SubmittedRange> mCurrentRanges;
    bool mFrameAllocated = false;
};

class GfxRenderingAPIVulkan final : public GfxRenderingAPI {
  public:
    explicit GfxRenderingAPIVulkan(GfxWindowBackendSDL2* windowBackend);
    ~GfxRenderingAPIVulkan() override;

    const char* GetName() override;
    int GetMaxTextureSize() override;
    GfxClipParameters GetClipParameters() override;
    void UnloadShader(ShaderProgram* oldPrg) override;
    void LoadShader(ShaderProgram* newPrg) override;
    ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint32_t shaderId1) override;
    ShaderProgram* LookupShader(uint64_t shaderId0, uint32_t shaderId1) override;
    void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;
    uint32_t NewTexture() override;
    void SelectTexture(int tile, uint32_t textureId) override;
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) override;
    void SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) override;
    void SetDepthTestAndMask(bool depth_test, bool z_upd) override;
    void SetZmodeDecal(bool decal) override;
    void SetViewport(int x, int y, int width, int height) override;
    void SetScissor(int x, int y, int width, int height) override;
    void SetUseAlpha(bool useAlpha) override;
    void DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) override;
    void Init() override;
    void OnResize() override;
    void StartFrame() override;
    void EndFrame() override;
    void FinishRender() override;
    int CreateFramebuffer() override;
    void UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                     bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                     bool can_extract_depth) override;
    void StartDrawToFramebuffer(int fbId, float noiseScale) override;
    void CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0,
                         int dstX1, int dstY1) override;
    void ClearFramebuffer(bool color, bool depth) override;
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) override;
    void ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) override;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) override;
    void* GetFramebufferTextureId(int fbId) override;
    void SelectTextureFb(int fbId) override;
    void DeleteTexture(uint32_t texId) override;
    void SetTextureFilter(FilteringMode mode) override;
    FilteringMode GetTextureFilter() override;
    void SetSrgbMode() override;
    ImTextureID GetTextureById(int id) override;
    bool InitImGui();
    void ShutdownImGui();
    void NewFrame();
    void RenderDrawData(ImDrawData* drawData);

  private:
    uint32_t GetMinImageCount() const;
    void CreateSwapchain();
    void CleanupSwapchain();
    void RecreateSwapchain();
    void CreateImageViews();
    void CreateDepthResources();
    void DestroyDepthResources();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void DestroyFrameResources();
    void CreateAllocator();
    void CreateUploadCommandPool();
    VkPipelineLayout CreatePipelineLayout();
    VkPipeline CreateGraphicsPipeline(VulkanShaderProgram& program, const CCFeatures& ccFeatures, bool useAlpha);
    void CreateTextureDescriptorResources();
    void DestroyTextureDescriptorResources();
    void DestroyShaderProgram(VulkanShaderProgram& program);
    void DestroyShaderPrograms();
    void DestroyTexture(VulkanTexture& texture);
    void DestroyTextures();
    void DestroyFramebufferResources(VulkanFramebuffer& framebuffer);
    void DestroyFramebuffers();
    void CreateFramebufferColorResources(VulkanFramebuffer& framebuffer);
    void CreateFramebufferDepthResources(VulkanFramebuffer& framebuffer);
    void TransitionImageUsage(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout& layout,
                              VkPipelineStageFlags2& stageMask, VkAccessFlags2& accessMask, VulkanImageUsage newUsage);
    void EndCurrentRendering();
    void BeginRenderingToCurrentFramebuffer();
    VulkanFramebuffer* GetFramebuffer(int fbId);
    VkImage GetFramebufferColorImage(int fbId);
    VkImageView GetFramebufferColorImageView(int fbId);
    VkImageLayout& GetFramebufferColorLayout(int fbId);
    VkPipelineStageFlags2& GetFramebufferColorStageMask(int fbId);
    VkAccessFlags2& GetFramebufferColorAccessMask(int fbId);
    VkExtent2D GetFramebufferExtent(int fbId) const;
    void UploadTextureToGpu(VulkanTexture& texture, const uint8_t* rgba32Buf, uint32_t width, uint32_t height);
    void WriteBindlessTextureDescriptor(uint32_t textureId);
    void EnsureImGuiTextureDescriptor(uint32_t textureId);
    VkCommandBuffer BeginImmediateCommands();
    void EndImmediateCommands(VkCommandBuffer commandBuffer);
    VulkanTexture& GetTexture(uint32_t textureId);
    void DestroyVulkanObjects();

    GfxWindowBackendSDL2* mWindowBackend = nullptr;
    VkInstance mInstance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    VmaAllocator_T* mAllocator = nullptr;
    VkQueue mGraphicsQueue = VK_NULL_HANDLE;
    VkQueue mPresentQueue = VK_NULL_HANDLE;
    Vulkan::QueueFamilyIndices mQueueFamilies;
    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
    VkFormat mSwapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D mSwapchainExtent = {};
    std::vector<VkImage> mSwapchainImages;
    std::vector<VkImageView> mSwapchainImageViews;
    std::vector<VkSemaphore> mSwapchainRenderFinishedSemaphores;
    std::vector<VkImageLayout> mSwapchainImageLayouts;
    std::vector<VkPipelineStageFlags2> mSwapchainImageStageMasks;
    std::vector<VkAccessFlags2> mSwapchainImageAccessMasks;
    VkFormat mDepthFormat = VK_FORMAT_D32_SFLOAT;
    VkImage mDepthImage = VK_NULL_HANDLE;
    VmaAllocation_T* mDepthAllocation = nullptr;
    VkImageView mDepthImageView = VK_NULL_HANDLE;
    VkImageLayout mDepthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlags2 mDepthImageStageMask = VK_PIPELINE_STAGE_2_NONE;
    VkAccessFlags2 mDepthImageAccessMask = VK_ACCESS_2_NONE;
    VkExtent2D mDepthExtent = {};
    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    VkCommandPool mUploadCommandPool = VK_NULL_HANDLE;
    std::array<VulkanFrame, FRAMES_IN_FLIGHT> mFrames;
    uint32_t mCurrentFrameIndex = 0;
    VulkanFrame* mCurrentFrame = nullptr;
    uint32_t mCurrentImageIndex = 0;
    VkCommandBuffer mCurrentCommandBuffer = VK_NULL_HANDLE;
    bool mFrameActive = false;
    bool mRenderingActive = false;
    bool mImGuiInitialized = false;
    bool mFramebufferResized = false;

    std::map<std::pair<uint64_t, uint32_t>, std::unique_ptr<VulkanShaderProgram>> mShaderProgramPool;
    VulkanShaderProgram* mShaderProgram = nullptr;
    std::vector<VulkanFramebuffer> mFramebuffers;
    uint32_t mCurrentFramebuffer = 0;
    uint32_t mCurrentRenderTargetWidth = 0;
    uint32_t mCurrentRenderTargetHeight = 0;
    std::vector<VulkanTexture> mTextures;
    uint32_t mCurrentTextureIds[SHADER_MAX_TEXTURES] = {};
    int mCurrentTile = 0;
    uint32_t mMaxBindlessTextures = 0;
    VkDescriptorSetLayout mTextureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool mTextureDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet mTextureDescriptorSet = VK_NULL_HANDLE;
    VulkanVertexRingBuffer mVertexRingBuffer;
    uint32_t mFrameCount = 0;
    float mCurrentNoiseScale = 1.0f;
    FilteringMode mCurrentFilterMode = FILTER_THREE_POINT;
};

} // namespace Fast
