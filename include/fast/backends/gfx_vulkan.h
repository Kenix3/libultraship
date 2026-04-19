#ifdef ENABLE_VULKAN
#pragma once

#include "gfx_rendering_api.h"
#include "../interpreter.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <optional>
#include <vector>
#include <map>

struct ImDrawData;

namespace Fast {

struct ShaderProgramVulkan {
    uint64_t shaderId0;
    uint64_t shaderId1;
    uint8_t numInputs;
    bool usedTextures[SHADER_MAX_TEXTURES];
};

struct FramebufferVK {
    uint32_t width;
    uint32_t height;
    bool hasDepthBuffer;
    uint32_t msaaLevel;
    bool invertY;
};

/**
 * @brief Vulkan-backed GfxRenderingAPI (first phase — architecture bring-up).
 *
 * Implements the GfxRenderingAPI contract using Vulkan for window integration
 * and ImGui rendering.  Game-scene rendering methods (DrawTriangles, textures,
 * shader programs) are scaffolded stubs that will be connected to ParallelRDP
 * in subsequent phases.
 *
 * Initialization sequence called by Fast3dWindow / Interpreter:
 *   1. Constructor — stores SDL window pointer.
 *   2. VulkanInit(sdlWindow) — creates Vk instance/device/swapchain, inits
 *      ImGui Vulkan backend.  Called from Gui::ImGuiBackendInit().
 *   3. Init() — creates framebuffers and PRDP CommandProcessor.
 *   4. Per-frame: StartFrame() → game commands → EndFrame() → FinishRender().
 */
class GfxRenderingAPIVulkan final : public GfxRenderingAPI {
  public:
    GfxRenderingAPIVulkan() = default;
    ~GfxRenderingAPIVulkan() override;

    // ---- GfxRenderingAPI interface ------------------------------------------
    const char* GetName() override { return "Vulkan"; }
    int GetMaxTextureSize() override { return 8192; }
    GfxClipParameters GetClipParameters() override { return { true, false }; }

    void UnloadShader(ShaderProgram* oldPrg) override;
    void LoadShader(ShaderProgram* newPrg) override;
    void ClearShaderCache() override;
    ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) override;
    ShaderProgram* LookupShader(uint64_t shaderId0, uint64_t shaderId1) override;
    void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;

    uint32_t NewTexture() override;
    void SelectTexture(int tile, uint32_t textureId) override;
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) override;
    void SetSamplerParameters(int sampler, bool linearFilter, uint32_t cms, uint32_t cmt) override;

    void SetDepthTestAndMask(bool depthTest, bool zUpd) override;
    void SetZmodeDecal(bool decal) override;
    void SetViewport(int x, int y, int width, int height) override;
    void SetScissor(int x, int y, int width, int height) override;
    void SetUseAlpha(bool useAlpha) override;

    void DrawTriangles(float bufVbo[], size_t bufVboLen, size_t bufVboNumTris) override;

    void Init() override;
    void OnResize() override;
    void StartFrame() override;
    void EndFrame() override;
    void FinishRender() override;

    int CreateFramebuffer() override;
    void UpdateFramebufferParameters(int fbId, uint32_t width, uint32_t height, uint32_t msaaLevel,
                                     bool oglInvertY, bool renderTarget, bool hasDepthBuffer,
                                     bool canExtractDepth) override;
    void StartDrawToFramebuffer(int fbId, float noiseScale) override;
    void CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0,
                         int dstY0, int dstX1, int dstY1) override;
    void ClearFramebuffer(bool color, bool depth) override;
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) override;
    void ResolveMSAAColorBuffer(int fbIdTarget, int fbIdSrc) override;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fbId, const std::set<std::pair<float, float>>& coordinates) override;
    void* GetFramebufferTextureId(int fbId) override;
    void SelectTextureFb(int fbId) override;
    void DeleteTexture(uint32_t texId) override;

    void SetTextureFilter(FilteringMode mode) override;
    FilteringMode GetTextureFilter() override;
    void SetSrgbMode() override;
    ImTextureID GetTextureById(int id) override;

    // ---- Vulkan-specific helpers called from Gui ----------------------------

    /** Called from Gui::ImGuiBackendInit() to bootstrap Vulkan and ImGui. */
    void VulkanInit(SDL_Window* window);
    /** Called from Gui::ImGuiBackendNewFrame(). */
    void NewFrame();
    /** Called from Gui::ImGuiRenderDrawData(). */
    void RenderDrawData(ImDrawData* data);

  private:
    // ---- Internal Vulkan helpers --------------------------------------------
    void CreateInstance(SDL_Window* window);
    void CreateSurface(SDL_Window* window);
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapchain(uint32_t width, uint32_t height);
    void CreateSwapchainImageViews();
    void CreateRenderPass();
    void CreateFramebuffersForSwapchain();
    void CreateCommandPoolAndBuffers();
    void CreateSyncObjects();
    void InitImGuiVulkanBackend();
    void CleanupSwapchain();
    void RecreateSwapchain();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    // ---- Vulkan handles -----------------------------------------------------
    VkInstance mInstance = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkQueue mGraphicsQueue = VK_NULL_HANDLE;
    VkQueue mPresentQueue = VK_NULL_HANDLE;
    uint32_t mGraphicsFamily = 0;
    uint32_t mPresentFamily = 0;

    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
    VkFormat mSwapchainFormat{};
    VkExtent2D mSwapchainExtent{};
    std::vector<VkImage> mSwapchainImages;
    std::vector<VkImageView> mSwapchainImageViews;
    std::vector<VkFramebuffer> mSwapchainFramebuffers;

    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    VkCommandPool mCommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> mCommandBuffers;

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    VkSemaphore mImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT] = {};
    VkSemaphore mRenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT] = {};
    VkFence mInFlightFences[MAX_FRAMES_IN_FLIGHT] = {};
    std::vector<VkFence> mImagesInFlight;

    VkDescriptorPool mImGuiDescriptorPool = VK_NULL_HANDLE;

    uint32_t mCurrentFrame = 0;
    uint32_t mCurrentImageIndex = 0;
    bool mFrameStarted = false;
    bool mNeedsSwapchainRecreation = false;

    // ---- Game-scene state (stubs — will connect to PRDP) --------------------
    std::map<std::pair<uint64_t, uint64_t>, ShaderProgramVulkan> mShaderPool;
    ShaderProgramVulkan* mCurrentShader = nullptr;
    std::vector<FramebufferVK> mFramebuffers;
    int mCurrentFbId = 0;
    uint32_t mNextTextureId = 1;
    FilteringMode mFilterMode = FILTER_THREE_POINT;

    SDL_Window* mSdlWindow = nullptr;
};

} // namespace Fast
#endif // ENABLE_VULKAN
