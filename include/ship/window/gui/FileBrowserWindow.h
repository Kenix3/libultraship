#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "ship/window/gui/GuiWindow.h"

namespace Ship {

/**
 * @brief A label and glob set for one entry in the browser's filter combo.
 *
 * Patterns match by extension: "*.png" keeps names ending in ".png" (case
 * insensitive) and "*" matches everything.
 */
struct FileFilter {
    std::string Label;                 ///< Human-readable name shown in the filter combo (e.g. "Images").
    std::vector<std::string> Patterns; ///< Globs, e.g. { "*.png", "*.jpg" }; "*" matches everything.
};

/**
 * @brief One file-pick request passed to FileBrowserWindow::Open / OpenBlocking.
 *
 * The browser knows nothing about what is being picked. The caller fills in the
 * title, start directory and filters, and gets the chosen path back through
 * OnResult (or std::nullopt if the user cancels).
 */
struct FileBrowserRequest {
    std::string Title = "Select a file"; ///< Modal title.
    std::filesystem::path StartDir;      ///< Directory to open in; empty -> current working directory.
    std::vector<FileFilter> Filters;     ///< Selectable filters; empty -> show all files.
    bool Save = false;                   ///< Save mode: adds a filename field and returns directory/filename.
    std::string DefaultName;             ///< Save-mode default filename.
    /** @brief Fires on the render thread with the chosen path, or std::nullopt if cancelled. */
    std::function<void(std::optional<std::filesystem::path>)> OnResult;
};

/**
 * @brief An ImGui file browser used in place of a native OS file dialog.
 *
 * Draws a modal that walks the filesystem and picks (or, in save mode, names) a
 * file using only ImGui, so it works with no display server or native dialog on
 * any backend. Works with mouse, keyboard or controller. Gui registers it once;
 * it stays invisible until a request comes in and is driven through the static
 * API below.
 *
 * The browser stays domain-agnostic; the calling port owns the rest of the
 * contract:
 * - Describe the pick. Set the title, filters and (for saves) the default name
 *   on the request; the browser only knows what you put there.
 * - Handle the result in OnResult. It runs on the render thread, a later frame
 *   than the Open() call, and fires exactly once: the chosen path on confirm,
 *   or std::nullopt on cancel. Keep it short, or hand heavy work to a worker.
 * - Mind lifetime. Because OnResult can resolve frames later, anything it
 *   captures (a `this`, a buffer) must outlive the request. Capture a
 *   shared_ptr when you can't otherwise guarantee that.
 * - Drive your own flow with IsOpen() while a pick is in flight, e.g. to hold a
 *   boot step or keep the render loop running.
 */
class FileBrowserWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    virtual ~FileBrowserWindow();

    /**
     * @brief Queue a request and return immediately. OnResult fires later on the render thread.
     * @param request The pick to perform. Safe to call from any thread.
     */
    static void Open(FileBrowserRequest request);

    /**
     * @brief Queue a request and block until the render loop resolves it.
     *
     * Only safe to call off the render thread (e.g. a worker); it waits for the
     * render loop to draw the browser, so calling it on the render thread deadlocks.
     * @param request The pick to perform (its OnResult is overwritten).
     * @return The chosen path, or std::nullopt if cancelled.
     */
    static std::optional<std::filesystem::path> OpenBlocking(FileBrowserRequest request);

    /** @brief True while a request is active or queued. */
    static bool IsOpen();

  protected:
    /** @brief Popup host: renders the modal directly (no host window) when a request is active. */
    void Draw() override;

    void OnInit(const nlohmann::json& initArgs = nlohmann::json::object()) override;
    void DrawElement() override;
    void UpdateElement() override;

  private:
    /** @brief One row in the current directory listing. */
    struct Entry {
        std::string Name;
        std::filesystem::path Path; ///< For the ".." row this is the parent directory.
        bool IsDir;
    };

    /** @brief One sidebar shortcut: an icon, a label and the directory it opens. */
    struct Place {
        const char* Icon;
        std::string Label;
        std::filesystem::path Path;
    };

    /** @brief A collapsible group of sidebar shortcuts, e.g. "Places" or "Drives". */
    struct PlaceGroup {
        std::string Name;
        std::vector<Place> Places;
    };

    void BeginRequest();
    void SetCwd(const std::filesystem::path& path);
    void BuildPlaces();
    void Refresh();
    void Finish(std::optional<std::filesystem::path> result);
    void DrawBody();
    bool PassesFilter(const std::string& name) const;

    FileBrowserRequest mRequest;
    bool mActive = false;
    bool mOpenPopup = false;
    bool mNeedRefresh = false;
    bool mFocusList = false;
    int mFilterIndex = 0;
    std::filesystem::path mCwd;
    std::vector<Entry> mEntries;
    std::vector<PlaceGroup> mPlaces;
    char mPathBuffer[1024] = {};
    std::string mFilename;
    char mFilenameBuffer[260] = {};
};

} // namespace Ship
