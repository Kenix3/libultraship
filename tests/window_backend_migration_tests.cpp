#include <gtest/gtest.h>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "ship/config/Config.h"
#include "ship/window/Window.h"

// ============================================================
// GetSavedWindowBackend config migration
//
// Backend IDs were renumbered once already (DX11 0→1, OpenGL 1→2,
// Metal 2→3), which silently moved existing configs onto a different
// renderer. These tests cover the recovery path: the saved backend
// name identifies the renderer even when the saved ID has gone stale.
//
// Uses a minimal Window subclass with a fake name/ID mapping that
// mirrors the production Fast3D numbering, so the base-class logic is
// exercised without any graphics or windowing work.
// ============================================================

namespace {

class FakeWindow final : public Ship::Window {
  public:
    explicit FakeWindow(std::shared_ptr<Ship::Config> config) : Ship::Window(std::move(config)) {
    }

    // Surface the protected members under test.
    using Ship::Window::AddAvailableWindowBackend;
    using Ship::Window::GetSavedWindowBackend;
    using Ship::Window::SetWindowBackend;

    // Same names and IDs as Fast3dWindow so the tests read like the
    // production incident they guard against.
    std::string GetWindowBackendName() override {
        switch (GetWindowBackend()) {
            case 1:
                return "DirectX 11";
            case 2:
                return "OpenGL";
            case 3:
                return "Metal";
            default:
                return "";
        }
    }

    int32_t GetWindowBackendIdByName(const std::string& name) override {
        if (name == "DirectX 11") {
            return 1;
        }
        if (name == "OpenGL") {
            return 2;
        }
        if (name == "Metal") {
            return 3;
        }
        return -1;
    }

    // Pure-virtual stubs; none of these run in the tests below.
    void Close() override {
    }
    void RunGuiOnly() override {
    }
    void StartFrame() override {
    }
    void EndFrame() override {
    }
    bool IsFrameReady() override {
        return false;
    }
    void HandleEvents() override {
    }
    void SetCursorVisibility(bool visible) override {
    }
    uint32_t GetWidth() override {
        return 0;
    }
    uint32_t GetHeight() override {
        return 0;
    }
    float GetAspectRatio() override {
        return 0.0f;
    }
    int32_t GetPosX() override {
        return 0;
    }
    int32_t GetPosY() override {
        return 0;
    }
    void SetMousePos(Ship::Coords pos) override {
    }
    Ship::Coords GetMousePos() override {
        return { 0, 0 };
    }
    Ship::Coords GetMouseDelta() override {
        return { 0, 0 };
    }
    Ship::CoordsF GetMouseWheel() override {
        return { 0.0f, 0.0f };
    }
    bool GetMouseState(Ship::MouseBtn btn) override {
        return false;
    }
    void SetMouseCapture(bool capture) override {
    }
    bool IsMouseCaptured() override {
        return false;
    }
    uint32_t GetCurrentRefreshRate() override {
        return 0;
    }
    bool SupportsWindowedFullscreen() override {
        return false;
    }
    bool CanDisableVerticalSync() override {
        return false;
    }
    void SetResolutionMultiplier(float multiplier) override {
    }
    void SetMsaaLevel(uint32_t value) override {
    }
    void SetFullscreen(bool isFullscreen) override {
    }
    bool IsFullscreen() override {
        return false;
    }
    bool IsRunning() override {
        return false;
    }
    const char* GetKeyName(int32_t scancode) override {
        return "";
    }
    uintptr_t GetGfxFrameBuffer() override {
        return 0;
    }
    void SetCurrentDimensions(uint32_t width, uint32_t height) override {
    }
    void SetCurrentDimensions(uint32_t width, uint32_t height, int32_t posX, int32_t posY) override {
    }
    void SetCurrentDimensions(bool isFullscreen, uint32_t width, uint32_t height) override {
    }
    void SetCurrentDimensions(bool isFullscreen, uint32_t width, uint32_t height, int32_t posX,
                              int32_t posY) override {
    }
    Ship::WindowRect GetPrimaryMonitorRect() override {
        return {};
    }
};

class WindowBackendMigrationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mTempDir = std::filesystem::temp_directory_path() / "lus_backend_migration_test";
        std::filesystem::create_directories(mTempDir);
        mConfig = std::make_shared<Ship::Config>((mTempDir / "backend_migration.cfg.json").string());
        mConfig->Init();
        mConfig->Erase("Window.Backend.Id");
        mConfig->Erase("Window.Backend.Name");

        mWindow = std::make_unique<FakeWindow>(mConfig);
        // Register the two backends "this platform" offers. DX11 (1) is
        // deliberately absent so an unavailable-backend path exists.
        mWindow->AddAvailableWindowBackend(2); // OpenGL
        mWindow->AddAvailableWindowBackend(3); // Metal
    }

    void TearDown() override {
        mWindow.reset();
        mConfig.reset();
        std::filesystem::remove_all(mTempDir);
    }

    std::filesystem::path mTempDir;
    std::shared_ptr<Ship::Config> mConfig;
    std::unique_ptr<FakeWindow> mWindow;
};

} // anonymous namespace

// A stale ID with a valid saved name must resolve by name, and the stored
// ID must be healed to match.
TEST_F(WindowBackendMigrationTest, SavedNameRecoversStaleId) {
    // Pre-renumbering OpenGL config: id 1 now names DX11 (unavailable here).
    mConfig->SetInt("Window.Backend.Id", 1);
    mConfig->SetString("Window.Backend.Name", "OpenGL");

    EXPECT_EQ(mWindow->GetSavedWindowBackend(), 2);
    EXPECT_EQ(mConfig->GetInt("Window.Backend.Id", -1), 2);
}

TEST_F(WindowBackendMigrationTest, MatchingNameAndIdResolveUnchanged) {
    mConfig->SetInt("Window.Backend.Id", 2);
    mConfig->SetString("Window.Backend.Name", "OpenGL");

    EXPECT_EQ(mWindow->GetSavedWindowBackend(), 2);
    EXPECT_EQ(mConfig->GetInt("Window.Backend.Id", -1), 2);
}

// An unrecognized name must not disturb the existing ID validation path.
TEST_F(WindowBackendMigrationTest, UnknownNameFallsThroughToSavedId) {
    mConfig->SetInt("Window.Backend.Id", 2);
    mConfig->SetString("Window.Backend.Name", "Glide64");

    EXPECT_EQ(mWindow->GetSavedWindowBackend(), 2);
}

// The case from the original report: a pre-renumbering Metal config
// (id 2) would re-resolve as OpenGL, silently switching macOS users off
// Metal. The saved id is valid AND available, but names a different
// backend than the user chose; the name must win.
TEST_F(WindowBackendMigrationTest, PreRenumberingMetalConfigStaysOnMetal) {
    mConfig->SetInt("Window.Backend.Id", 2); // pre-renumbering Metal; now OpenGL's ID
    mConfig->SetString("Window.Backend.Name", "Metal");

    EXPECT_EQ(mWindow->GetSavedWindowBackend(), 3);
    EXPECT_EQ(mConfig->GetInt("Window.Backend.Id", -1), 3);
}

// With no saved name at all, the legacy ID path must behave as before.
TEST_F(WindowBackendMigrationTest, MissingNameUsesSavedIdWhenAvailable) {
    mConfig->SetInt("Window.Backend.Id", 3);

    EXPECT_EQ(mWindow->GetSavedWindowBackend(), 3);
}

// Startup resolution must never rewrite the saved choice; only an explicit
// (persisting) backend change may. A resolve-and-rewrite on launch is what
// let the renumbering bug permanently overwrite saved configs.
TEST_F(WindowBackendMigrationTest, NonPersistingSetLeavesConfigUntouched) {
    mWindow->SetWindowBackend(2, false);
    EXPECT_EQ(mConfig->GetInt("Window.Backend.Id", -1), -1);
    EXPECT_EQ(mConfig->GetString("Window.Backend.Name", ""), "");

    mWindow->SetWindowBackend(2);
    EXPECT_EQ(mConfig->GetInt("Window.Backend.Id", -1), 2);
    EXPECT_EQ(mConfig->GetString("Window.Backend.Name", ""), "OpenGL");
}
