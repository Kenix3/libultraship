#pragma once

#include <map>
#include <vector>
#include <string>
#include <functional>

#include "GuiWindow.h"
#include <ImGui/imgui.h>
#include <spdlog/spdlog.h>

namespace LUS {
#define CMD_SUCCESS true
#define CMD_FAILED false
#define NULLSTR "None"

class ConsoleWindow;
typedef std::function<bool(std::shared_ptr<ConsoleWindow> console, std::vector<std::string> args)> CommandHandler;

enum class ArgumentType { TEXT, NUMBER, PLAYER_POS, PLAYER_ROT };

struct CommandArgument {
    std::string info;
    ArgumentType type = ArgumentType::NUMBER;
    bool optional = false;
};

struct CommandEntry {
    CommandHandler handler;
    std::string description;
    std::vector<CommandArgument> arguments;
};

struct ConsoleLine {
    std::string text;
    spdlog::level::level_enum priority = spdlog::level::info;
    std::string channel = "Console";
};

class ConsoleWindow : public GuiWindow, public std::enable_shared_from_this<ConsoleWindow> {
  private:
    static int CallbackStub(ImGuiInputTextCallbackData* data);
    static bool ClearCommand(std::shared_ptr<ConsoleWindow> console, const std::vector<std::string>& args);
    static bool HelpCommand(std::shared_ptr<ConsoleWindow> console, const std::vector<std::string>& args);
    static bool BindCommand(std::shared_ptr<ConsoleWindow> console, const std::vector<std::string>& args);
    static bool BindToggleCommand(std::shared_ptr<ConsoleWindow> console, const std::vector<std::string>& args);

    using GuiWindow::GuiWindow;

    int mSelectedId = -1;
    int mHistoryIndex = -1;
    std::vector<int> mSelectedEntries;
    std::string mFilter;
    std::string mCurrentChannel = "Console";
    bool mOpenAutocomplete = false;
    char* mInputBuffer = nullptr;
    char* mFilterBuffer = nullptr;
    std::string mCmdHint = NULLSTR;
    spdlog::level::level_enum mLevelFilter = spdlog::level::trace;

    std::vector<std::string> mHistory;
    std::vector<std::string> mAutoComplete;
    std::map<ImGuiKey, std::string> mBindings;
    std::map<ImGuiKey, std::string> mBindingToggle;
    std::map<std::string, CommandEntry> mCommands;
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

  protected:
    void Append(const std::string& channel, spdlog::level::level_enum priority, const char* fmt, va_list args);

  public:
    void ClearLogs(std::string channel);
    void ClearLogs();
    void Init() override;
    void Update() override;
    void Draw() override;
    void Dispatch(const std::string& line);
    void SendInfoMessage(const char* fmt, ...);
    void SendErrorMessage(const char* fmt, ...);
    void SendInfoMessage(const std::string& str);
    void SendErrorMessage(const std::string& str);
    void Append(const std::string& channel, spdlog::level::level_enum priority, const char* fmt, ...);
    bool HasCommand(const std::string& command);
    void AddCommand(const std::string& command, CommandEntry entry);
    std::string GetCurrentChannel();
};
} // namespace LUS