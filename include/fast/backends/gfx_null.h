#pragma once
// Null / recording Fast3D backends — used for headless testing.
//
// NullWindowBackend   : a no-op GfxWindowBackend that never touches the OS.
// RecordingRenderingAPI : a GfxRenderingAPI that records every call so tests
//   can inspect the exact sequence of operations the interpreter emits for a
//   given display-list.  It is also used as the "ground truth" fixture when
//   comparing Fast3D → PRDP output.

#include "fast/backends/gfx_rendering_api.h"
#include "fast/backends/gfx_window_manager_api.h"

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace Fast {

// ---------------------------------------------------------------------------
// NullWindowBackend
// ---------------------------------------------------------------------------
// All methods are no-ops; GetDimensions returns the dimensions passed to Init.
class NullWindowBackend final : public GfxWindowBackend {
  public:
    void Init(const char* /*gameName*/, const char* /*apiName*/, bool /*startFullScreen*/, uint32_t width,
              uint32_t height, int32_t /*posX*/, int32_t /*posY*/) override {
        mWidth = width;
        mHeight = height;
    }
    void Close() override {}
    void SetKeyboardCallbacks(bool (*onKeyDown)(int), bool (*onKeyUp)(int), void (*onAllKeysUp)()) override {
        mOnKeyDown = onKeyDown;
        mOnKeyUp = onKeyUp;
    }
    void SetMouseCallbacks(bool (*onMouseButtonDown)(int), bool (*onMouseButtonUp)(int)) override {
        mOnMouseButtonDown = onMouseButtonDown;
        mOnMouseButtonUp = onMouseButtonUp;
    }
    void SetFullscreenChangedCallback(void (*cb)(bool)) override {
        mOnFullscreenChanged = cb;
    }
    void SetFullscreen(bool /*fs*/) override {}
    void GetActiveWindowRefreshRate(uint32_t* rate) override {
        if (rate)
            *rate = 60;
    }
    void SetCursorVisibility(bool /*v*/) override {}
    void SetMousePos(int32_t, int32_t) override {}
    void GetMousePos(int32_t* x, int32_t* y) override {
        if (x)
            *x = 0;
        if (y)
            *y = 0;
    }
    void GetMouseDelta(int32_t* x, int32_t* y) override {
        if (x)
            *x = 0;
        if (y)
            *y = 0;
    }
    void GetMouseWheel(float* x, float* y) override {
        if (x)
            *x = 0;
        if (y)
            *y = 0;
    }
    bool GetMouseState(uint32_t) override {
        return false;
    }
    void SetMouseCapture(bool) override {}
    bool IsMouseCaptured() override {
        return false;
    }
    void GetDimensions(uint32_t* w, uint32_t* h, int32_t* px, int32_t* py) override {
        if (w)
            *w = mWidth;
        if (h)
            *h = mHeight;
        if (px)
            *px = 0;
        if (py)
            *py = 0;
    }
    void HandleEvents() override {}
    bool IsFrameReady() override {
        return true;
    }
    void SwapBuffersBegin() override {}
    void SwapBuffersEnd() override {}
    double GetTime() override {
        return 0.0;
    }
    int GetTargetFps() override {
        return 60;
    }
    void SetTargetFps(int fps) override {
        mTargetFps = fps;
    }
    void SetMaxFrameLatency(int) override {}
    const char* GetKeyName(int) override {
        return "";
    }
    bool CanDisableVsync() override {
        return false;
    }
    bool IsRunning() override {
        return true;
    }
    void Destroy() override {}
    bool IsFullscreen() override {
        return false;
    }

  private:
    uint32_t mWidth = 320;
    uint32_t mHeight = 240;
    void (*mOnAllKeysUp)() = nullptr;
};

// ---------------------------------------------------------------------------
// RecordingRenderingAPI — records every call the interpreter makes
// ---------------------------------------------------------------------------

// Types of API calls the interpreter can emit.
enum class RCallType {
    Init,
    OnResize,
    StartFrame,
    EndFrame,
    FinishRender,
    CreateFramebuffer,
    UpdateFramebufferParameters,
    StartDrawToFramebuffer,
    ClearFramebuffer,
    CopyFramebuffer,
    ReadFramebufferToCPU,
    ResolveMSAAColorBuffer,
    // Shader
    CreateAndLoadNewShader,
    LookupShader,
    LoadShader,
    UnloadShader,
    ClearShaderCache,
    ShaderGetInfo,
    // Texture
    NewTexture,
    SelectTexture,
    UploadTexture,
    SetSamplerParameters,
    DeleteTexture,
    SelectTextureFb,
    // State
    SetDepthTestAndMask,
    SetZmodeDecal,
    SetViewport,
    SetScissor,
    SetUseAlpha,
    SetTextureFilter,
    SetSrgbMode,
    // Draw
    DrawTriangles,
};

// A recorded invocation.
struct RCall {
    RCallType type;

    // Generic integer / float parameters (stored in order of significance).
    int i[8] = {};
    float f[8] = {};

    // Vertex buffer snapshot (DrawTriangles only).
    std::vector<float> vbo;
    size_t vbo_num_tris = 0;
};

// Shader program record kept by RecordingRenderingAPI.
struct RecordedShader {
    uint64_t id0 = 0;
    uint64_t id1 = 0;
    uint8_t numInputs = 0;
    bool usedTextures[2] = {};
};

class RecordingRenderingAPI final : public GfxRenderingAPI {
  public:
    // ---- GfxRenderingAPI interface ----------------------------------------
    const char* GetName() override {
        return "Recording";
    }
    int GetMaxTextureSize() override {
        return 4096;
    }
    GfxClipParameters GetClipParameters() override {
        return { false, false };
    }

    void UnloadShader(ShaderProgram* /*prg*/) override;
    void LoadShader(ShaderProgram* prg) override;
    void ClearShaderCache() override;
    ShaderProgram* CreateAndLoadNewShader(uint64_t id0, uint64_t id1) override;
    ShaderProgram* LookupShader(uint64_t id0, uint64_t id1) override;
    void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) override;

    uint32_t NewTexture() override;
    void SelectTexture(int tile, uint32_t texId) override;
    void UploadTexture(const uint8_t* rgba32Buf, uint32_t w, uint32_t h) override;
    void SetSamplerParameters(int sampler, bool linear, uint32_t cms, uint32_t cmt) override;
    void DeleteTexture(uint32_t texId) override;
    void SelectTextureFb(int fbId) override;

    void SetDepthTestAndMask(bool depthTest, bool zUpd) override;
    void SetZmodeDecal(bool decal) override;
    void SetViewport(int x, int y, int w, int h) override;
    void SetScissor(int x, int y, int w, int h) override;
    void SetUseAlpha(bool useAlpha) override;
    void SetTextureFilter(FilteringMode mode) override;
    FilteringMode GetTextureFilter() override {
        return mFilterMode;
    }
    void SetSrgbMode() override;
    ImTextureID GetTextureById(int /*id*/) override {
        return nullptr;
    }

    void DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) override;

    void Init() override;
    void OnResize() override;
    void StartFrame() override;
    void EndFrame() override;
    void FinishRender() override;
    int CreateFramebuffer() override;
    void UpdateFramebufferParameters(int fbId, uint32_t w, uint32_t h, uint32_t msaa, bool oglInvertY,
                                     bool renderTarget, bool hasDepth, bool canExtractDepth) override;
    void StartDrawToFramebuffer(int fbId, float noiseScale) override;
    void CopyFramebuffer(int dst, int src, int sx0, int sy0, int sx1, int sy1, int dx0, int dy0, int dx1,
                         int dy1) override;
    void ClearFramebuffer(bool color, bool depth) override;
    void ReadFramebufferToCPU(int fbId, uint32_t w, uint32_t h, uint16_t* buf) override;
    void ResolveMSAAColorBuffer(int dst, int src) override;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int /*fbId*/, const std::set<std::pair<float, float>>& /*coords*/) override {
        return {};
    }
    void* GetFramebufferTextureId(int /*fbId*/) override {
        return nullptr;
    }

    // ---- Test-inspection helpers ------------------------------------------

    /** All recorded API calls in chronological order. */
    const std::vector<RCall>& GetCalls() const {
        return mCalls;
    }

    /** Only the DrawTriangles calls. */
    std::vector<RCall> GetDrawCalls() const;

    /** Number of triangles emitted in the most recent frame. */
    size_t FrameTriCount() const {
        return mFrameTriCount;
    }

    /** Reset the recording (clears calls and counters). */
    void Reset();

    /** Currently-active shader (may be nullptr if none has been loaded). */
    RecordedShader* CurrentShader() {
        return mCurrentShader;
    }

  private:
    std::vector<RCall> mCalls;
    size_t mFrameTriCount = 0;
    int mNextTexId = 1;
    int mNextFbId = 1;
    FilteringMode mFilterMode = FILTER_THREE_POINT;

    // Shader pool: keyed by (id0, id1), values stored in mShaders vector so
    // pointers remain stable (no invalidation across push_back because we
    // index via stable pointer-to-element trick; the vector must not reallocate
    // after creation — use reserve or a list in the implementation).
    struct ShaderEntry {
        RecordedShader info;
        // The address of this struct is used as the opaque ShaderProgram* cookie.
    };
    std::vector<ShaderEntry> mShaders;
    RecordedShader* mCurrentShader = nullptr;

    RecordedShader* FindShader(uint64_t id0, uint64_t id1);

    // Helpers to convert between ShaderProgram* (opaque cookie) and ShaderEntry*.
    static ShaderProgram* EntryToSP(ShaderEntry* e);
    static ShaderEntry* SPToEntry(ShaderProgram* prg);
};

} // namespace Fast
