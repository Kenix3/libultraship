#include "ship/window/gui/FileBrowserWindow.h"
#include "ship/window/gui/IconsFontAwesome4.h"

#include <imgui.h>
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <future>
#include <mutex>
#include <set>

namespace fs = std::filesystem;

namespace Ship {

namespace {

std::mutex sMutex;
std::deque<FileBrowserRequest> sQueue;
bool sActiveOrQueued = false; // lets IsOpen() answer without reaching into the window instance.
fs::path sLastDir;            // last directory browsed this session; resumes there on the next pick.

bool CaseLess(const std::string& a, const std::string& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), [](char x, char y) {
        return std::tolower((unsigned char)x) < std::tolower((unsigned char)y);
    });
}

bool EndsWithCI(const std::string& s, const std::string& suffix) {
    if (suffix.size() > s.size()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), s.rbegin(),
                      [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); });
}

bool MatchPattern(const std::string& name, const std::string& pattern) {
    if (pattern == "*" || pattern.empty()) {
        return true;
    }
    if (pattern.size() >= 2 && pattern[0] == '*' && pattern[1] == '.') {
        return EndsWithCI(name, pattern.substr(1));
    }
    return EndsWithCI(name, pattern);
}

// Ellipsize a sidebar label that's wider than the rail.
std::string FitLabel(const std::string& text, float maxWidth) {
    if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth) {
        return text;
    }
    std::string head = text;
    const std::string ellipsis = "...";
    while (!head.empty() && ImGui::CalcTextSize((head + ellipsis).c_str()).x > maxWidth) {
        head.pop_back();
    }
    return head + ellipsis;
}

} // namespace

FileBrowserWindow::~FileBrowserWindow() {
    SPDLOG_TRACE("destruct file browser window");
}

void FileBrowserWindow::InitElement() {
}

void FileBrowserWindow::UpdateElement() {
}

void FileBrowserWindow::Draw() {
    if (!IsVisible()) {
        return;
    }
    DrawElement();
    SyncVisibilityConsoleVariable();
}

void FileBrowserWindow::DrawElement() {
    if (!mActive) {
        std::lock_guard<std::mutex> lock(sMutex);
        if (sQueue.empty()) {
            return;
        }
        mRequest = std::move(sQueue.front());
        sQueue.pop_front();
        BeginRequest();
    }
    if (mNeedRefresh) {
        Refresh();
        mNeedRefresh = false;
    }

    const std::string popupId = mRequest.Title + "###ShipFileBrowser";
    if (mOpenPopup) {
        ImVec2 center = ImGui::GetMainViewport()->GetWorkCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::OpenPopup(popupId.c_str());
        mOpenPopup = false;
    }
    const ImVec2 viewport = ImGui::GetMainViewport()->WorkSize;
    ImGui::SetNextWindowSize(ImVec2(viewport.x * 0.8f, viewport.y * 0.8f), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal(popupId.c_str(), nullptr,
                               ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse)) {
        DrawBody();
        ImGui::EndPopup();
    }
}

void FileBrowserWindow::BeginRequest() {
    mActive = true;
    mOpenPopup = true;
    mNeedRefresh = true;
    mFocusList = true;
    mFilterIndex = 0;

    std::error_code ec;
    // Caller's StartDir wins; otherwise resume where the last pick left off this session, falling
    // back to the working directory.
    fs::path start = mRequest.StartDir;
    if (start.empty()) {
        start = sLastDir.empty() ? fs::current_path(ec) : sLastDir;
    }
    if (ec || !fs::exists(start, ec) || !fs::is_directory(start, ec)) {
        start = fs::current_path(ec);
    }
    SetCwd(start);
    BuildPlaces();

    mFilename = mRequest.DefaultName;
    std::snprintf(mFilenameBuffer, sizeof(mFilenameBuffer), "%s", mFilename.c_str());
}

// Gather the sidebar shortcuts once per request: the user's folders, then the root and mounted
// media. Touches the disk, so it runs on open rather than every frame.
void FileBrowserWindow::BuildPlaces() {
    mPlaces.clear();

    auto isDir = [](const fs::path& p) {
        std::error_code ec;
        return fs::is_directory(p, ec);
    };

    PlaceGroup places;
    places.Name = "Places";
    auto add = [&](const char* icon, const std::string& label, const fs::path& path) {
        if (isDir(path)) {
            places.Places.push_back({ icon, label, path });
        }
    };
    if (const char* homeEnv = std::getenv("HOME")) {
        const fs::path home = homeEnv;
        add(ICON_FA_HOME, "Home", home);
        add(ICON_FA_DESKTOP, "Desktop", home / "Desktop");
        add(ICON_FA_FILE_TEXT_O, "Documents", home / "Documents");
        add(ICON_FA_DOWNLOAD, "Downloads", home / "Downloads");
        add(ICON_FA_PICTURE_O, "Pictures", home / "Pictures");
        add(ICON_FA_MUSIC, "Music", home / "Music");
        add(ICON_FA_FILM, "Videos", home / "Videos");
    }
    if (!places.Places.empty()) {
        mPlaces.push_back(std::move(places));
    }

    PlaceGroup drives;
    drives.Name = "Devices";
    drives.Places.push_back({ ICON_FA_HDD_O, "File System", fs::path("/") });

    std::vector<fs::path> mountRoots = { "/mnt", "/media", "/Volumes" };
    if (const char* user = std::getenv("USER")) {
        mountRoots.push_back(fs::path("/media") / user);
        mountRoots.push_back(fs::path("/run/media") / user);
    }
    std::set<std::string> seen;
    for (const fs::path& mountRoot : mountRoots) {
        if (!isDir(mountRoot)) {
            continue;
        }
        std::error_code ec;
        fs::directory_iterator it(mountRoot, fs::directory_options::skip_permission_denied, ec);
        for (fs::directory_iterator end; !ec && it != end; it.increment(ec)) {
            const fs::path& path = it->path();
            if (isDir(path) && seen.insert(path.string()).second) {
                drives.Places.push_back({ ICON_FA_HDD_O, path.filename().string(), path });
            }
        }
    }
    if (!drives.Places.empty()) {
        mPlaces.push_back(std::move(drives));
    }
}

void FileBrowserWindow::SetCwd(const fs::path& path) {
    mCwd = path.lexically_normal();
    sLastDir = mCwd; // remember for the next pick this session
    std::snprintf(mPathBuffer, sizeof(mPathBuffer), "%s", mCwd.string().c_str());
    mNeedRefresh = true;
}

bool FileBrowserWindow::PassesFilter(const std::string& name) const {
    if (mRequest.Filters.empty()) {
        return true;
    }
    const FileFilter& filter = mRequest.Filters[mFilterIndex];
    for (const std::string& pattern : filter.Patterns) {
        if (MatchPattern(name, pattern)) {
            return true;
        }
    }
    return false;
}

void FileBrowserWindow::Refresh() {
    mEntries.clear();
    mFocusList = true;

    fs::path parent = mCwd.parent_path();
    if (!parent.empty() && parent != mCwd) {
        mEntries.push_back({ "..", parent, true });
    }

    std::vector<Entry> dirs;
    std::vector<Entry> files;
    std::error_code ec;
    fs::directory_iterator it(mCwd, fs::directory_options::skip_permission_denied, ec);
    if (!ec) {
        for (fs::directory_iterator end; it != end; it.increment(ec)) {
            if (ec) {
                break;
            }
            std::error_code ec2;
            const fs::directory_entry& de = *it;
            std::string name = de.path().filename().string();
            if (name.empty() || name[0] == '.') {
                continue;
            }
            if (de.is_directory(ec2)) {
                dirs.push_back({ name, de.path(), true });
            } else if (PassesFilter(name)) {
                files.push_back({ name, de.path(), false });
            }
        }
    }
    auto byName = [](const Entry& a, const Entry& b) { return CaseLess(a.Name, b.Name); };
    std::sort(dirs.begin(), dirs.end(), byName);
    std::sort(files.begin(), files.end(), byName);
    for (Entry& e : dirs) {
        mEntries.push_back(std::move(e));
    }
    for (Entry& e : files) {
        mEntries.push_back(std::move(e));
    }
}

void FileBrowserWindow::Finish(std::optional<fs::path> result) {
    auto callback = std::move(mRequest.OnResult);
    mActive = false;
    {
        std::lock_guard<std::mutex> lock(sMutex);
        sActiveOrQueued = !sQueue.empty();
    }
    ImGui::CloseCurrentPopup();
    if (callback) {
        callback(std::move(result));
    }
}

void FileBrowserWindow::DrawBody() {
    // Escape cancels for keyboard users; gamepads use the Cancel button (B is taken by ImGui nav).
    if (!ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        Finish(std::nullopt);
        return;
    }

    // Set by the address bar, rail or list below, then applied after the list draw so we don't
    // mutate mEntries while iterating it.
    fs::path enterDir;
    std::optional<fs::path> picked;
    std::string fillName;

    if (ImGui::Button(ICON_FA_LEVEL_UP "###up")) {
        fs::path parent = mCwd.parent_path();
        if (!parent.empty() && parent != mCwd) {
            enterDir = parent;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Up one level");
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##address", mPathBuffer, sizeof(mPathBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::error_code ec;
        fs::path typed(mPathBuffer);
        if (fs::is_directory(typed, ec)) {
            enterDir = typed;
        } else if (!mRequest.Save && fs::is_regular_file(typed, ec)) {
            picked = typed;
        } else if (mRequest.Save && !typed.empty()) {
            // Treat a typed file path as a save target: open its directory, pre-fill the name.
            fs::path parent = typed.parent_path();
            if (parent.empty() || fs::is_directory(parent, ec)) {
                if (!parent.empty()) {
                    enterDir = parent;
                }
                fillName = typed.filename().string();
            }
        }
    }
    ImGui::Separator();

    // Leave room below the rail/list for the footer rows: buttons, plus filter/filename when shown.
    float footer = ImGui::GetFrameHeightWithSpacing();
    if (!mRequest.Filters.empty()) {
        footer += ImGui::GetFrameHeightWithSpacing();
    }
    if (mRequest.Save) {
        footer += ImGui::GetFrameHeightWithSpacing();
    }

    ImGui::BeginChild("##rail", ImVec2(200.0f, -footer), true);
    for (const PlaceGroup& group : mPlaces) {
        if (ImGui::CollapsingHeader(group.Name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const Place& place : group.Places) {
                std::string label =
                    FitLabel(std::string(place.Icon) + "  " + place.Label, ImGui::GetContentRegionAvail().x);
                ImGui::PushID(place.Path.string().c_str());
                if (ImGui::Selectable(label.c_str())) {
                    enterDir = place.Path;
                }
                ImGui::PopID();
            }
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("##list", ImVec2(0.0f, -footer), true);
    ImGuiListClipper clipper;
    clipper.Begin((int)mEntries.size());
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
            const Entry& e = mEntries[i];
            std::string label = std::string(e.IsDir ? ICON_FA_FOLDER : ICON_FA_FILE_O) + "  " + e.Name;
            ImGui::PushID(i);
            if (ImGui::Selectable(label.c_str())) {
                if (e.IsDir) {
                    enterDir = e.Path;
                } else if (mRequest.Save) {
                    fillName = e.Name;
                } else {
                    picked = e.Path;
                }
            }
            if (mFocusList && i == 0) {
                ImGui::SetItemDefaultFocus();
            }
            ImGui::PopID();
        }
    }
    clipper.End();
    mFocusList = false;
    ImGui::EndChild();

    if (!mRequest.Filters.empty()) {
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##filter", mRequest.Filters[mFilterIndex].Label.c_str())) {
            for (int i = 0; i < (int)mRequest.Filters.size(); ++i) {
                bool selected = (i == mFilterIndex);
                if (ImGui::Selectable(mRequest.Filters[i].Label.c_str(), selected)) {
                    mFilterIndex = i;
                    mNeedRefresh = true;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    bool confirm = false;
    if (mRequest.Save) {
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputText("##filename", mFilenameBuffer, sizeof(mFilenameBuffer),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            confirm = true;
        }
        mFilename = mFilenameBuffer;
    }

    if (mRequest.Save) {
        if (ImGui::Button("Save")) {
            confirm = true;
        }
        ImGui::SameLine();
    }
    if (ImGui::Button("Cancel")) {
        Finish(std::nullopt);
        return;
    }

    if (!fillName.empty()) {
        mFilename = fillName;
        std::snprintf(mFilenameBuffer, sizeof(mFilenameBuffer), "%s", mFilename.c_str());
    }
    if (!enterDir.empty()) {
        SetCwd(enterDir);
    } else if (picked) {
        Finish(picked);
    } else if (confirm && mRequest.Save && !mFilename.empty()) {
        Finish((mCwd / mFilename).lexically_normal());
    }
}

void FileBrowserWindow::Open(FileBrowserRequest request) {
    std::lock_guard<std::mutex> lock(sMutex);
    sQueue.push_back(std::move(request));
    sActiveOrQueued = true;
}

std::optional<fs::path> FileBrowserWindow::OpenBlocking(FileBrowserRequest request) {
    std::promise<std::optional<fs::path>> promise;
    std::future<std::optional<fs::path>> future = promise.get_future();
    request.OnResult = [&promise](std::optional<fs::path> result) { promise.set_value(std::move(result)); };
    Open(std::move(request));
    return future.get();
}

bool FileBrowserWindow::IsOpen() {
    std::lock_guard<std::mutex> lock(sMutex);
    return sActiveOrQueued;
}

} // namespace Ship
