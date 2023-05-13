#pragma once

#include <map>
#include <vector>
#include <string>
#include <functional>

#include "gui/GuiWindow.h"
#include "core/Console.h"
#include <ImGui/imgui.h>
#include <spdlog/spdlog.h>

namespace LUS {

struct ConsoleLine {
    std::string text;
    spdlog::level::level_enum priority = spdlog::level::info;
    std::string channel = "Console";
};

class ConsoleWindow : public GuiWindow {
  public:
    void ClearLogs(std::string channel);
    void ClearLogs();
    void Dispatch(const std::string& line);
    void SendInfoMessage(const char* fmt, ...);
    void SendErrorMessage(const char* fmt, ...);
    void SendInfoMessage(const std::string& str);
    void SendErrorMessage(const std::string& str);
    void Append(const std::string& channel, spdlog::level::level_enum priority, const char* fmt, ...);
    std::string GetCurrentChannel();

  protected:
    void Append(const std::string& channel, spdlog::level::level_enum priority, const char* fmt, va_list args);
    void InitElement() override;
    void UpdateElement() override;
    void DrawElement() override;

  private:
    static int CallbackStub(ImGuiInputTextCallbackData* data);
    static bool ClearCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args,
                             std::string* output);
    static bool HelpCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args,
                            std::string* output);
    static bool BindCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args,
                            std::string* output);
    static bool BindToggleCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args,
                                  std::string* output);
    static bool SetCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args, std::string* output);
    static bool GetCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args, std::string* output);
    static int32_t CheckVarType(const std::string& input);

    using GuiWindow::GuiWindow;

    int32_t mSelectedId = -1;
    int32_t mHistoryIndex = -1;
    std::vector<int> mSelectedEntries;
    std::string mFilter;
    std::string mCurrentChannel = "Console";
    bool mOpenAutocomplete = false;
    char* mInputBuffer = nullptr;
    char* mFilterBuffer = nullptr;
    std::string mCmdHint = "Null";
    spdlog::level::level_enum mLevelFilter = spdlog::level::trace;
    std::map<ImGuiKey, std::string> mBindings;
    std::map<ImGuiKey, std::string> mBindingToggle;
    std::vector<std::string> mHistory;
    std::vector<std::string> mAutoComplete;
    std::map<std::string, std::vector<ConsoleLine>> mLog;
    const std::vector<std::string> mLogChannels = { "Console", "Logs" };
    const std::vector<spdlog::level::level_enum> mPriorityFilters = { spdlog::level::off,  spdlog::level::critical,
                                                                      spdlog::level::err,  spdlog::level::warn,
                                                                      spdlog::level::info, spdlog::level::debug,
                                                                      spdlog::level::trace };
    const std::vector<ImVec4> mPriorityColours = {
        ImVec4(0.8f, 0.8f, 0.8f, 1.0f),     // TRACE
        ImVec4(0.9f, 0.9f, 0.9f, 1.0f),     // DEBUG
        ImVec4(1.0f, 1.0f, 1.0f, 1.0f),     // INFO
        ImVec4(1.0f, 0.875f, 0.125f, 1.0f), // WARN
        ImVec4(0.65f, 0.18f, 0.25, 1.0f),   // ERROR
        ImVec4(0.95f, 0.11f, 0.25, 1.0f),   // CRITICAL
        ImVec4(0.0f, 0.0f, 0.0f, 0.0f)      // OFF
    };
    static constexpr size_t gMaxBufferSize = 255;
};
} // namespace LUS