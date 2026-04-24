#include "ship/window/Window.h"
#ifdef ENABLE_VULKAN_PRDP

#include "fast/backends/gfx_vulkan.h"

// ParallelRDP / Granite headers
#include "rdp_device.hpp"
#include "context.hpp"

#include <algorithm>
#include <cstring>

// volk provides the Vulkan loader used by parallel-rdp
#include "volk.h"

namespace Fast {

// ============================================================
// Construction / Destruction
// ============================================================

GfxRenderingAPIVulkan::GfxRenderingAPIVulkan() = default;

GfxRenderingAPIVulkan::~GfxRenderingAPIVulkan() {
    // CommandProcessor must be destroyed before Device/Context.
    mProcessor.reset();
    mDevice.reset();
    mContext.reset();
}

// ============================================================
// RdpCommandBackend
// ============================================================

void GfxRenderingAPIVulkan::SubmitCommand(size_t numWords, const uint32_t* words) {
    if (!mAvailable || !mProcessor || numWords == 0 || words == nullptr)
        return;

    // If no frame context is active, start one.  This handles the common case
    // where the interpreter emits RDP commands outside of an explicit
    // StartFrame/EndFrame bracket (e.g. during initialisation).
    if (!mFrameActive) {
        mProcessor->begin_frame_context();
        mFrameActive = true;
    }

    mProcessor->enqueue_command(numWords, words);
}

// ============================================================
// GfxRenderingAPI — identity / capability queries
// ============================================================

const char* GfxRenderingAPIVulkan::GetName() {
    return "Vulkan (ParallelRDP)";
}

int GfxRenderingAPIVulkan::GetMaxTextureSize() {
    // ParallelRDP handles textures internally via TMEM, so this is not
    // directly applicable.  Return the N64's maximum texture dimension.
    return 1024;
}

GfxClipParameters GfxRenderingAPIVulkan::GetClipParameters() {
    // ParallelRDP does its own clipping; these values are only consulted by
    // the interpreter's triangle setup.  Use Vulkan-style Z [0,1].
    return { true, false };
}

// ============================================================
// Shader management — no-ops (ParallelRDP handles combiners)
// ============================================================

void GfxRenderingAPIVulkan::UnloadShader(ShaderProgram*) {}
void GfxRenderingAPIVulkan::LoadShader(ShaderProgram*) {}
void GfxRenderingAPIVulkan::ClearShaderCache() {}

ShaderProgram* GfxRenderingAPIVulkan::CreateAndLoadNewShader(uint64_t, uint64_t) {
    // Return a pointer to the dummy program so the interpreter has a
    // non-null ShaderProgram* to work with.
    mDummyShader.numInputs = 0;
    std::fill(std::begin(mDummyShader.usedTextures),
              std::end(mDummyShader.usedTextures), false);
    return reinterpret_cast<ShaderProgram*>(&mDummyShader);
}

ShaderProgram* GfxRenderingAPIVulkan::LookupShader(uint64_t, uint64_t) {
    return reinterpret_cast<ShaderProgram*>(&mDummyShader);
}

void GfxRenderingAPIVulkan::ShaderGetInfo(ShaderProgram*, uint8_t* numInputs,
                                           bool usedTextures[2]) {
    *numInputs = 0;
    usedTextures[0] = false;
    usedTextures[1] = false;
}

// ============================================================
// Texture management — no-ops (textures are loaded via RDP
// commands into RDRAM/TMEM by ParallelRDP)
// ============================================================

uint32_t GfxRenderingAPIVulkan::NewTexture() { return 0; }
void GfxRenderingAPIVulkan::SelectTexture(int, uint32_t) {}
void GfxRenderingAPIVulkan::UploadTexture(const uint8_t*, uint32_t, uint32_t) {}
void GfxRenderingAPIVulkan::SetSamplerParameters(int, bool, uint32_t, uint32_t) {}
void GfxRenderingAPIVulkan::DeleteTexture(uint32_t) {}

// ============================================================
// Render state — no-ops (RDP other-modes control all state)
// ============================================================

void GfxRenderingAPIVulkan::SetDepthTestAndMask(bool, bool) {}
void GfxRenderingAPIVulkan::SetZmodeDecal(bool) {}
void GfxRenderingAPIVulkan::SetViewport(int, int, int, int) {}
void GfxRenderingAPIVulkan::SetScissor(int, int, int, int) {}
void GfxRenderingAPIVulkan::SetUseAlpha(bool) {}

// ============================================================
// Draw calls — no-op (ParallelRDP rasterises from RDP commands)
// ============================================================

void GfxRenderingAPIVulkan::DrawTriangles(float[], size_t, size_t) {
    // Intentionally empty.  All geometry reaches ParallelRDP through the
    // raw RDP triangle commands emitted by the interpreter's EmitRdpCommand
    // path, not through the GfxRenderingAPI DrawTriangles VBO interface.
}

// ============================================================
// Lifecycle
// ============================================================

void GfxRenderingAPIVulkan::Init() {
    if (volkInitialize() != VK_SUCCESS) {
        mAvailable = false;
        return;
    }
    if (!Vulkan::Context::init_loader(nullptr)) {
        mAvailable = false;
        return;
    }

    mContext = std::make_unique<Vulkan::Context>();
    if (!mContext->init_instance_and_device(
            nullptr, 0, nullptr, 0,
            Vulkan::CONTEXT_CREATION_ENABLE_ADVANCED_WSI_BIT)) {
        // Retry without advanced WSI (software Vulkan drivers may lack it).
        mContext = std::make_unique<Vulkan::Context>();
        if (!mContext->init_instance_and_device(nullptr, 0, nullptr, 0, 0)) {
            mAvailable = false;
            return;
        }
    }

    mDevice = std::make_unique<Vulkan::Device>();
    mDevice->set_context(*mContext);

    mRdram.resize(RDRAM_SIZE, 0);

    mProcessor = std::make_unique<RDP::CommandProcessor>(
        *mDevice, mRdram.data(), 0, RDRAM_SIZE, HIDDEN_RDRAM_SIZE,
        RDP::COMMAND_PROCESSOR_FLAG_HOST_VISIBLE_HIDDEN_RDRAM_BIT);

    if (!mProcessor->device_is_supported()) {
        mProcessor.reset();
        mAvailable = false;
        return;
    }

    mAvailable = true;
}

void GfxRenderingAPIVulkan::OnResize() {
    // ParallelRDP renders to RDRAM, not a window surface.
}

void GfxRenderingAPIVulkan::StartFrame() {
    if (!mAvailable || !mProcessor)
        return;
    mProcessor->begin_frame_context();
    mFrameActive = true;
}

void GfxRenderingAPIVulkan::EndFrame() {
    if (!mAvailable || !mProcessor || !mFrameActive)
        return;
    mProcessor->wait_for_timeline(mProcessor->signal_timeline());
    mFrameActive = false;
}

void GfxRenderingAPIVulkan::FinishRender() {
    EndFrame();
}

// ============================================================
// Framebuffer management
// ============================================================

int GfxRenderingAPIVulkan::CreateFramebuffer() {
    // ParallelRDP uses RDRAM as its framebuffer; the interpreter's
    // framebuffer tracking is not needed.
    return 0;
}

void GfxRenderingAPIVulkan::UpdateFramebufferParameters(int, uint32_t, uint32_t,
                                                         uint32_t, bool, bool,
                                                         bool, bool) {}

void GfxRenderingAPIVulkan::StartDrawToFramebuffer(int, float) {}

void GfxRenderingAPIVulkan::CopyFramebuffer(int, int, int, int, int, int,
                                             int, int, int, int) {}

void GfxRenderingAPIVulkan::ClearFramebuffer(bool, bool) {}

void GfxRenderingAPIVulkan::ReadFramebufferToCPU(int, uint32_t width,
                                                  uint32_t height,
                                                  uint16_t* rgba16Buf) {
    if (!mAvailable || !rgba16Buf)
        return;

    // Read from the default N64 framebuffer address (0x00100000).
    // The caller should have set up SET_COLOR_IMAGE to point here.
    constexpr uint32_t kDefaultFbAddr = 0x00100000;
    auto fb = ReadFramebufferFromRDRAM(kDefaultFbAddr, width, height);
    std::memcpy(rgba16Buf, fb.data(), fb.size() * sizeof(uint16_t));
}

void GfxRenderingAPIVulkan::ResolveMSAAColorBuffer(int, int) {}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
GfxRenderingAPIVulkan::GetPixelDepth(
    int, const std::set<std::pair<float, float>>&) {
    return {};
}

void* GfxRenderingAPIVulkan::GetFramebufferTextureId(int) {
    return nullptr;
}

void GfxRenderingAPIVulkan::SelectTextureFb(int) {}

void GfxRenderingAPIVulkan::SetTextureFilter(FilteringMode) {}

FilteringMode GfxRenderingAPIVulkan::GetTextureFilter() {
    return FILTER_NONE;
}

void GfxRenderingAPIVulkan::SetSrgbMode() {}

ImTextureID GfxRenderingAPIVulkan::GetTextureById(int) {
    return nullptr;
}

// ============================================================
// RDRAM access
// ============================================================

uint8_t* GfxRenderingAPIVulkan::GetRDRAM() {
    return mRdram.data();
}

size_t GfxRenderingAPIVulkan::GetRDRAMSize() const {
    return mRdram.size();
}

std::vector<uint16_t> GfxRenderingAPIVulkan::ReadFramebufferFromRDRAM(
    uint32_t addr, uint32_t width, uint32_t height) const {
    std::vector<uint16_t> fb(width * height, 0);
    if (!mAvailable || addr + width * height * 2 > mRdram.size())
        return fb;

    // ParallelRDP's store_vram_color writes each RGBA5551 pixel to
    // vram16[index ^ 1], swapping adjacent uint16_t pairs within every
    // 32-bit RDRAM word.  Applying the same XOR when reading compensates
    // and restores the correct pixel order.
    const uint16_t* vram16 = reinterpret_cast<const uint16_t*>(
        mRdram.data() + addr);
    for (uint32_t i = 0; i < width * height; i++)
        fb[i] = vram16[i ^ 1];
    return fb;
}

} // namespace Fast

#endif // ENABLE_VULKAN_PRDP
