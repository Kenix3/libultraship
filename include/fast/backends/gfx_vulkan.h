//
//  gfx_vulkan.h
//  libultraship
//
//  Vulkan rendering backend for fast3d. Mirrors the Metal backend's deferred
//  per-framebuffer command buffer architecture: every framebuffer drawn during
//  a frame records into its own primary command buffer, and EndFrame submits
//  them in framebuffer-id order with the screen (fb 0, the swapchain image)
//  last, followed by the present.
//
#pragma once
#ifdef ENABLE_VULKAN

#include "gfx_rendering_api.h"
#include "../interpreter.h"

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>
#include <map>

struct SDL_Window;
struct ImDrawData;

namespace Ship {
class ConsoleVariable;
class ResourceManager;
} // namespace Ship

namespace Fast {

constexpr uint32_t VK_FRAMES_IN_FLIGHT = 3;
constexpr uint32_t VK_MAX_MSAA = 8;

// cantor pairing for the shader pool key (same scheme as Metal)
static inline size_t vk_cantor(uint64_t a, uint64_t b) {
    return (a + b) * (a + b + 1) / 2 + b;
}

struct vk_hash_pair_shader_ids {
    size_t operator()(const std::pair<uint64_t, uint64_t>& p) const {
        return vk_cantor(p.first, p.second);
    }
};

// std140 mirror of the per-frame constants (template block FrameUniforms)
struct VulkanFrameUniforms {
    int32_t frameCount;
    float noiseScale;
    float pad[2];
};

// std140 mirror of the per-draw constants (template block DrawUniforms)
struct VulkanDrawUniforms {
    int32_t textureFiltering[8]; // 2x ivec4, slots 0..6 used
    float misc[4];               // x = prim_depth, y = lod_max
    float inputs[6][4];
    float fog_color[4];
    float grayscale_color[4];
    float uv_transform[2][4];
    float texture_clamp[2][4];
    float fog_params[4];
    float palette_params[2][4];
    float lod_params[4];
    // Game-bindable register file; must stay in lockstep with the DrawUniforms
    // block in port/shaders/vulkan/default.shader.glsl
    float custom[GFX_NUM_CUSTOM_UNIFORMS][4];
    // HD-replacement debug tint (rgb + mix amount); lockstep with the shader block.
    float debug_tint[4];
};

struct ShaderProgramVK {
    uint64_t shader_id0;
    uint64_t shader_id1;

    uint8_t numInputs;
    uint8_t numFloats;
    bool usedTextures[SHADER_MAX_TEXTURES];
    bool usedLighting = false;
    bool useAlphaBlend = false;
    bool markedForDeletion = false;

    VkShaderModule vs = VK_NULL_HANDLE;
    VkShaderModule fs = VK_NULL_HANDLE;

    uint32_t attribCount = 0;
    VkVertexInputAttributeDescription attribs[5];

    // Pipelines keyed by packed render state (msaa level, depth test/write,
    // compare op, cull mode, render pass family), created lazily on first use.
    std::map<uint32_t, VkPipeline> pipelines;
};

struct TextureDataVK {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE; // owned by the sampler cache
    VkDescriptorSet imguiSet = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mip_levels = 1;
    uint32_t filtering = 0;
    bool linear_filtering = false;
    bool isSwapchainAlias = false; // fb 0: view belongs to the swapchain
    bool auto_mipmaps = false;
};

struct FramebufferVK {
    uint32_t mTextureId = 0;

    VkImage mDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory mDepthMemory = VK_NULL_HANDLE;
    VkImageView mDepthView = VK_NULL_HANDLE;

    // MSAA attachments (resolve target is the main texture)
    VkImage mMsaaImage = VK_NULL_HANDLE;
    VkDeviceMemory mMsaaMemory = VK_NULL_HANDLE;
    VkImageView mMsaaView = VK_NULL_HANDLE;
    VkImage mMsaaDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory mMsaaDepthMemory = VK_NULL_HANDLE;
    VkImageView mMsaaDepthView = VK_NULL_HANDLE;

    VkFramebuffer mFramebuffer = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE; // canonical, owned by the pass cache

    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    uint32_t mMsaaLevel = 1;
    bool mHasDepthBuffer = false;
    bool mRenderTarget = false;

    // Per-frame recording state
    VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
    bool mPassActive = false;
    bool mHasEnded = false;

    ShaderProgramVK* mLastShaderProgram = nullptr;
    VkPipeline mLastPipeline = VK_NULL_HANDLE;
    VkImageView mLastBoundViews[SHADER_MAX_TEXTURES] = {};
    VkSampler mLastBoundSamplers[SHADER_MAX_TEXTURES] = {};
    uint32_t mLastSet0Offsets[4] = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };
    VkViewport mViewport = {};
    VkRect2D mScissor = {};
    float mLastDepthBias = 0.0f;

    int8_t mLastDepthTest = -1;
    int8_t mLastDepthMask = -1;
    int8_t mLastZmodeDecal = -1;
    int8_t mLastStrictDecal = -1;
};

class GfxRenderingAPIVK final : public GfxRenderingAPI {
  public:
    GfxRenderingAPIVK(std::shared_ptr<Ship::ConsoleVariable> consoleVariable = nullptr,
                      std::shared_ptr<Ship::ResourceManager> resourceManager = nullptr);
    ~GfxRenderingAPIVK() override = default;
    const char* GetName() override;
    int GetMaxTextureSize() override;
    GfxClipParameters GetClipParameters() override;
    void UnloadShader(ShaderProgram* oldPrg) override;
    void LoadShader(ShaderProgram* newPrg) override;
    ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint64_t shaderId1) override;
    ShaderProgram* LookupShader(uint64_t shaderId0, uint64_t shaderId1) override;
    void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;
    void ClearShaderCache() override;
    uint32_t NewTexture() override;
    void SelectTexture(int tile, uint32_t textureId) override;
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) override;
    void UploadTextureMip(const uint8_t* rgba32Buf, uint32_t width, uint32_t height, uint32_t level,
                          uint32_t totalLevels) override;
    void SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) override;
    void SetDepthTestAndMask(bool depth_test, bool z_upd) override;
    void SetCurrentPrimDepth(float depth) override;
    void SetCurrentMaxLod(float maxLod) override;
    void SetZmodeDecal(bool decal) override;
    void SetStrictDecal(bool on) override;
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
    void ClearDepthRegion(int x, int y, int w, int h) override;
    void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) override;
    void ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) override;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) override;
    void* GetFramebufferTextureId(int fbId) override;
    void SelectTextureFb(int fbId) override;
    void DeleteTexture(uint32_t texId) override;
    void SetTextureFilter(FilteringMode mode) override;
    FilteringMode GetTextureFilter() override;
    ImTextureID GetTextureById(int id) override;

    // Called from Fast3dGui (mirrors the Metal backend's custom ImGui hooks)
    bool VulkanInit(SDL_Window* window);
    void NewFrame();
    void RenderDrawData(ImDrawData* drawData);
    void ShutdownImGui();

  private:
    struct FrameSlot {
        VkFence fence = VK_NULL_HANDLE;
        bool fenceInFlight = false;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkSemaphore renderFinished = VK_NULL_HANDLE;

        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        uint8_t* vertexMapped = nullptr;

        VkBuffer uniformBuffer = VK_NULL_HANDLE;
        VkDeviceMemory uniformMemory = VK_NULL_HANDLE;
        uint8_t* uniformMapped = nullptr;

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
        uint8_t* stagingMapped = nullptr;
        size_t stagingCapacity = 0;

        VkDescriptorSet set0 = VK_NULL_HANDLE; // ring UBOs, static descriptor

        std::vector<std::pair<VkImage, VkDeviceMemory>> garbageImages;
        std::vector<VkImageView> garbageViews;
        std::vector<VkBuffer> garbageBuffers;
        std::vector<VkDeviceMemory> garbageMemory;
    };

    uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags props);
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer& buffer,
                      VkDeviceMemory& memory, uint8_t** mapped);
    void CreateImage(uint32_t width, uint32_t height, uint32_t mips, VkSampleCountFlagBits samples, VkFormat format,
                     VkImageUsageFlags usage, VkImage& image, VkDeviceMemory& memory);
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mips);
    void TransitionToGeneral(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect, uint32_t mips);
    void FullBarrier(VkCommandBuffer cmd);
    VkCommandBuffer BeginOneShot();
    void EndOneShot(VkCommandBuffer cmd);
    VkRenderPass GetRenderPass(uint32_t msaaLevel, bool hasDepth);
    VkSampler GetSampler(bool linear, uint32_t cms, uint32_t cmt, bool autoMipmap = false);
    void EnsureUploadCmd();
    void FlushUploads();
    // Grow a frame slot's staging buffer to fit an oversized upload (e.g. a 4K HD
    // replacement = 64 MB > the 32 MB default). Caller must ensure the queue is idle.
    void GrowStaging(FrameSlot& slot, size_t needed);
    void CreateSwapchain(uint32_t width, uint32_t height);
    void DestroySwapchainViews();
    void RebuildScreenFramebuffer();
    void EnsurePassActive(int fbId);
    void EndPass(FramebufferVK& fb);
    void RestartPass(FramebufferVK& fb);
    VkPipeline GetPipelineForState(ShaderProgramVK* prg, FramebufferVK& fb);
    void WriteDrawUniformsIfDirty();
    void SubmitFramebufferAndWait(int fbId);
    void CollectGarbage(FrameSlot& slot);
    void DestroyTextureData(TextureDataVK& tex, bool deferred);

    SDL_Window* mWindow = nullptr;

    std::shared_ptr<Ship::ConsoleVariable> mConsoleVariable;
    VkInstance mInstance = VK_NULL_HANDLE;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    uint32_t mQueueFamily = 0;
    VkQueue mQueue = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties mDeviceProps = {};
    bool mHasMirrorClampToEdge = false;
    bool mHasDepthClamp = false;
    bool mHasAnisotropy = false;

    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
    VkFormat mSwapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D mSwapchainExtent = {};
    std::vector<VkImage> mSwapchainImages;
    std::vector<VkImageView> mSwapchainViews;
    std::vector<VkFramebuffer> mSwapchainFramebuffers;
    std::vector<bool> mSwapchainImageUsed; // first-use layout tracking
    uint32_t mSwapchainImageIndex = 0;
    bool mSwapchainNeedsRecreate = false;
    bool mFrameActive = false;

    FrameSlot mFrames[VK_FRAMES_IN_FLIGHT];
    uint32_t mFrameSlot = 0;

    size_t mVertexRingOffset = 0;
    size_t mVertexRingCapacity = 0;
    size_t mUniformRingOffset = 0;
    size_t mUniformRingCapacity = 0;
    size_t mStagingOffset = 0;

    VkCommandBuffer mUploadCmd = VK_NULL_HANDLE;

    VkDescriptorSetLayout mSet0Layout = VK_NULL_HANDLE; // 4 dynamic UBOs
    VkDescriptorSetLayout mSet1Layout = VK_NULL_HANDLE; // 7 combined image samplers
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;

    std::map<uint32_t, VkRenderPass> mRenderPassCache; // key: msaa | (hasDepth << 8)
    std::map<uint32_t, VkSampler> mSamplerCache;       // key: linear | cms<<1 | cmt<<9

    std::unordered_map<std::pair<uint64_t, uint64_t>, ShaderProgramVK, vk_hash_pair_shader_ids> mShaderProgramPool;

    std::vector<TextureDataVK> mTextures;
    std::vector<FramebufferVK> mFramebuffers;
    std::vector<int> mDrawnFramebuffers;

    TextureDataVK mDummyTexture; // bound to unused sampler slots

    VulkanFrameUniforms mFrameUniforms = {};
    VulkanDrawUniforms mDrawUniforms = {};
    bool mFrameUniformsDirty = true;

    // Current dynamic offsets into the uniform ring (UINT32_MAX = unwritten)
    uint32_t mFrameUniformOffset = UINT32_MAX;
    uint32_t mDrawUniformOffset = UINT32_MAX;
    uint32_t mLightingUniformOffset = UINT32_MAX;
    uint32_t mTransformUniformOffset = UINT32_MAX;

    ShaderProgramVK* mShaderProgram = nullptr;

    int mCurrentTile = 0;
    uint32_t mCurrentTextureIds[SHADER_MAX_TEXTURES] = {};

    int32_t mRenderTargetHeight = 0;
    int mCurrentFramebuffer = 0;
    FilteringMode mCurrentFilterMode = FILTER_THREE_POINT;
    bool mLodMaxDirty = true;

    // Deferred screen readback (mirrors Metal)
    VkBuffer mScreenReadbackBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mScreenReadbackMemory = VK_NULL_HANDLE;
    uint8_t* mScreenReadbackMapped = nullptr;
    size_t mScreenReadbackCapacity = 0;
    uint32_t mScreenReadbackWidth = 0;
    uint32_t mScreenReadbackHeight = 0;
    bool mScreenReadbackRequested = false;
    bool mScreenReadbackDataReady = false;

    bool mImGuiInitialized = false;
};

} // namespace Fast

bool Vulkan_IsSupported();

#endif
