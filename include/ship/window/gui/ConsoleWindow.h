#pragma once

#include <map>
#include <vector>
#include <string>
#include <functional>

#include "ship/window/gui/GuiWindow.h"
#include "ship/debug/Console.h"
#include <imgui.h>
#include <spdlog/spdlog.h>

namespace Ship {

/**
 * @brief An ImGui window that provides an in-game developer console.
 *
 * ConsoleWindow renders a scrollable log view with a command-input field and
 * delegates command execution to the Console subsystem. It supports:
 * - Multiple named log channels (Console / Logs).
 * - Key bindings (bind/unbind commands).
 * - Log-level filtering.
 * - Command history and auto-complete.
 *
 * The window integrates with spdlog through a custom sink that routes log
 * output to the "Logs" channel automatically.
 *
 * Obtain the instance from Gui::GetGuiWindow("Console").
 */
class ConsoleWindow : public GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    virtual ~ConsoleWindow();

    /**
     * @brief Clears all log entries in the given channel.
     * @param channel Channel name to clear (e.g. "Console" or "Logs").
     */
    void ClearLogs(std::string channel);

    /** @brief Clears all log entries across all channels. */
    void ClearLogs();

    /**
     * @brief Parses and executes a command string via the Console subsystem.
     *
     * Equivalent to the user pressing Enter in the input field.
     * @param line Raw command string.
     */
    void Dispatch(const std::string& line);

    /**
     * @brief Appends a formatted informational message to the active console channel.
     * @param fmt printf-style format string.
     */
    void SendInfoMessage(const char* fmt, ...);

    /**
     * @brief Appends a formatted error message to the active console channel.
     * @param fmt printf-style format string.
     */
    void SendErrorMessage(const char* fmt, ...);

    /**
     * @brief Appends a plain-string informational message to the active console channel.
     * @param str Message string.
     */
    void SendInfoMessage(const std::string& str);

    /**
     * @brief Appends a plain-string error message to the active console channel.
     * @param str Message string.
     */
    void SendErrorMessage(const std::string& str);

    /**
     * @brief Appends a formatted log entry to the given channel with the given priority.
     * @param channel  Log channel to write to (e.g. "Console").
     * @param priority spdlog log level used to colour the entry.
     * @param fmt      printf-style format string.
     */
    void Append(const std::string& channel, spdlog::level::level_enum priority, const char* fmt, ...);

    /**
     * @brief Returns the name of the currently active log channel.
     * @return Channel name (e.g. "Console" or "Logs").
     */
    std::string GetCurrentChannel();

    /**
     * @brief Removes all key-to-command bindings registered via the "bind" command.
     */
    void ClearBindings();

    /** @brief Renders the console window contents (log, input field, filters). */
    void DrawElement() override;

  protected:
    /**
     * @brief Appends a log entry using a pre-parsed va_list (called by the public overloads).
     * @param channel  Log channel.
     * @param priority spdlog log level.
     * @param fmt      printf-style format string.
     * @param args     Argument list.
     */
    void Append(const std::string& channel, spdlog::level::level_enum priority, const char* fmt, va_list args);

    /** @brief Registers built-in console commands (clear, help, bind, set, get, …). */
    void InitElement() override;

    /** @brief Processes key bindings and clears expired auto-complete state. */
    void UpdateElement() override;

  private:
    /** @brief An individual log entry stored in a channel's history. */
    struct ConsoleLine {
        std::string Text;                                        ///< Rendered text.
        spdlog::level::level_enum Priority = spdlog::level::info; ///< Severity (controls colour).
        std::string Channel = "Console";                         ///< Owning channel name.
    };

    static int CallbackStub(ImGuiInputTextCallbackData* data);
    static int32_t ClearCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args,
                                std::string* output);
    static int32_t HelpCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args,
                               std::string* output);
    static int32_t UnbindCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args,
                                 std::string* output);
    static int32_t BindCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args,
                               std::string* output);
    static int32_t BindToggleCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args,
                                     std::string* output);
    static int32_t SetCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args,
                              std::string* output);
    static int32_t GetCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args,
                              std::string* output);
    static int32_t CheckVarType(const std::string& input);

    int32_t mSelectedId = -1;
    int32_t mHistoryIndex = -1;
    std::vector<int> mSelectedEntries;
    std::string mFilter;
    std::string mCurrentChannel = "Console";
    bool mOpenAutocomplete = false;
    char* mInputBuffer = nullptr;
    char* mFilterBuffer = nullptr;
    std::string mCmdHint = "None";
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
} // namespace Ship
