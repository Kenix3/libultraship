#include "menu/ConsoleWindow.h"

#include "core/bridge/consolevariablebridge.h"
#include "menu/Gui.h"
#include <Utils/StringHelper.h>
#include "misc/Utils.h"
#include <sstream>

namespace LUS {
std::string BuildUsage(const CommandEntry& entry) {
    std::string usage;
    for (const auto& arg : entry.arguments) {
        usage += StringHelper::Sprintf(arg.optional ? "[%s] " : "<%s> ", arg.info.c_str());
    }
    return usage;
}

bool ConsoleWindow::HelpCommand(std::shared_ptr<ConsoleWindow> console, const std::vector<std::string>& args) {
    console->SendInfoMessage("Commands:");
    for (const auto& cmd : console->mCommands) {
        console->SendInfoMessage(" - " + cmd.first);
    }
    return CMD_SUCCESS;
}

bool ConsoleWindow::ClearCommand(std::shared_ptr<ConsoleWindow> console, const std::vector<std::string>& args) {
    console->ClearLogs(console->GetCurrentChannel());
    return CMD_SUCCESS;
}

bool ConsoleWindow::BindCommand(std::shared_ptr<ConsoleWindow> console, const std::vector<std::string>& args) {
    if (args.size() > 2) {
        const ImGuiIO* io = &ImGui::GetIO();
        ;
        for (size_t k = 0; k < std::size(io->KeysData); k++) {
            std::string key(ImGui::GetKeyName((ImGuiKey)k));

            if (toLowerCase(args[1]) == toLowerCase(key)) {
                std::vector<std::string> tmp;
                const char* const delim = " ";
                std::ostringstream imploded;
                std::copy(args.begin() + 2, args.end(), std::ostream_iterator<std::string>(imploded, delim));
                console->mBindings[(ImGuiKey)k] = imploded.str();
                console->SendInfoMessage("Binding '%s' to %s", args[1].c_str(),
                                         console->mBindings[(ImGuiKey)k].c_str());
                break;
            }
        }
    }
    return CMD_SUCCESS;
}

bool ConsoleWindow::BindToggleCommand(std::shared_ptr<ConsoleWindow> console, const std::vector<std::string>& args) {
    if (args.size() > 2) {
        const ImGuiIO* io = &ImGui::GetIO();
        ;
        for (size_t k = 0; k < std::size(io->KeysData); k++) {
            std::string key(ImGui::GetKeyName((ImGuiKey)k));

            if (toLowerCase(args[1]) == toLowerCase(key)) {
                console->mBindingToggle[(ImGuiKey)k] = args[2];
                console->SendInfoMessage("Binding toggle '%s' to %s", args[1].c_str(),
                                         console->mBindingToggle[(ImGuiKey)k].c_str());
                break;
            }
        }
    }
    return CMD_SUCCESS;
}

void ConsoleWindow::Init() {
    mIsOpen = CVarGetInteger("gConsoleEnabled", 0);
    mInputBuffer = new char[gMaxBufferSize];
    strcpy(mInputBuffer, "");
    mFilterBuffer = new char[gMaxBufferSize];
    strcpy(mFilterBuffer, "");
    AddCommand("help", { HelpCommand, "Shows all the commands" });
    AddCommand("clear", { ClearCommand, "Clear the console history" });
    AddCommand("bind", { BindCommand, "Binds key to commands" });
    AddCommand("bind-toggle", { BindToggleCommand, "Bind key as a bool toggle" });
}

void ConsoleWindow::Update() {
    for (auto [key, cmd] : mBindings) {
        if (ImGui::IsKeyPressed(key)) {
            Dispatch(cmd);
        }
    }
    for (auto [key, var] : mBindingToggle) {
        if (ImGui::IsKeyPressed(key)) {
            Dispatch("set " + var + " " + std::to_string(!static_cast<bool>(CVarGetInteger(var.c_str(), 0))));
        }
    }
}

void ConsoleWindow::Draw() {
    if (!IsOpen()) {
        if (CVarGetInteger("gConsoleEnabled", 0)) {
            CVarClear("gConsoleEnabled");
        }
        return;
    }

    bool inputFocus = false;

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Console", &mIsOpen, ImGuiWindowFlags_NoFocusOnAppearing);
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();

    // Renders autocomplete window
    if (mOpenAutocomplete) {
        ImGui::SetNextWindowSize(ImVec2(350, std::min(static_cast<int>(mAutoComplete.size()), 3) * 20.f),
                                 ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(pos.x + 8, pos.y + size.y - 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));
        ImGui::Begin("##WndAutocomplete", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
        ImGui::BeginChild("AC_Child", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.3f, .3f, .3f, 1.0f));
        if (ImGui::BeginTable("AC_History", 1)) {
            for (const auto& cmd : mAutoComplete) {
                std::string usage = BuildUsage(mCommands[cmd]);
                std::string preview = cmd + " - " + mCommands[cmd].description;
                std::string autoComplete = (usage == NULLSTR ? cmd : usage);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (ImGui::Selectable(preview.c_str())) {
                    memset(mInputBuffer, 0, gMaxBufferSize);
                    memcpy(mInputBuffer, autoComplete.c_str(), sizeof(char) * autoComplete.size());
                    mOpenAutocomplete = false;
                    inputFocus = true;
                }
            }
            ImGui::EndTable();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            mOpenAutocomplete = false;
        }
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::End();
        ImGui::PopStyleVar();
    }

    if (ImGui::BeginPopupContextWindow("Context Menu")) {
        if (ImGui::MenuItem("Copy Text")) {
            ImGui::SetClipboardText(mLog[mCurrentChannel][mSelectedId].text.c_str());
            mSelectedId = -1;
        }
        ImGui::EndPopup();
    }
    if (mSelectedId != -1 && ImGui::IsMouseClicked(1)) {
        ImGui::OpenPopup("##WndAutocomplete");
    }

    // Renders top bar filters
    if (ImGui::Button("Clear")) {
        mLog[mCurrentChannel].clear();
    }

    if (CVarGetInteger("gSinkEnabled", 0)) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo("##channel", mCurrentChannel.c_str())) {
            for (const auto& channel : mLogChannels) {
                const bool isSelected = (channel == std::string(mCurrentChannel));
                if (ImGui::Selectable(channel.c_str(), isSelected)) {
                    mCurrentChannel = channel;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    } else {
        mCurrentChannel = "Console";
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);

    if (mCurrentChannel != "Console") {
        if (ImGui::BeginCombo("##level", spdlog::level::to_string_view(mLevelFilter).data())) {
            for (const auto& priorityFilter : mPriorityFilters) {
                const bool isSelected = priorityFilter == mLevelFilter;
                if (ImGui::Selectable(spdlog::level::to_string_view(priorityFilter).data(), isSelected)) {
                    mLevelFilter = priorityFilter;
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }
            ImGui::EndCombo();
        }
    } else {
        mLevelFilter = spdlog::level::trace;
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if (ImGui::InputTextWithHint("##input", "Filter", mFilterBuffer, gMaxBufferSize)) {
        mFilter = std::string(mFilterBuffer);
    }
    ImGui::PopItemWidth();

    // Renders console history
    const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.3f, .3f, .3f, 1.0f));
    if (ImGui::BeginTable("History", 1)) {

        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
            if (mSelectedId < (int)mLog.size() - 1) {
                ++mSelectedId;
            }
        }
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
            if (mSelectedId > 0) {
                --mSelectedId;
            }
        }

        const std::vector<ConsoleLine> channel = mLog[mCurrentChannel];
        for (int i = 0; i < static_cast<int>(channel.size()); i++) {
            ConsoleLine line = channel[i];
            if (!mFilter.empty() && line.text.find(mFilter) == std::string::npos) {
                continue;
            }
            if (mLevelFilter > line.priority) {
                continue;
            }
            std::string id = line.text + "##" + std::to_string(i);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const bool isSelected = (mSelectedId == i) ||
                                    std::find(mSelectedEntries.begin(), mSelectedEntries.end(), i) !=
                                        mSelectedEntries.end();
            ImGui::PushStyleColor(ImGuiCol_Text, mPriorityColours[line.priority]);
            if (ImGui::Selectable(id.c_str(), isSelected)) {
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftCtrl)) && !isSelected) {
                    mSelectedEntries.push_back(i);

                } else {
                    mSelectedEntries.clear();
                }
                mSelectedId = isSelected ? -1 : i;
            }
            ImGui::PopStyleColor();
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleColor();
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    if (mCurrentChannel == "Console") {
        // Renders input textfield
        constexpr ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackEdit |
                                              ImGuiInputTextFlags_CallbackCompletion |
                                              ImGuiInputTextFlags_CallbackHistory;
#ifdef __WIIU__
        ImGui::PushItemWidth(-53.0f * 2.0f);
#else
        ImGui::PushItemWidth(-53.0f);
#endif
        if (ImGui::InputTextWithHint("##CMDInput", ">", mInputBuffer, gMaxBufferSize, flags,
                                     &ConsoleWindow::CallbackStub, this)) {
            inputFocus = true;
            if (mInputBuffer[0] != '\0' && mInputBuffer[0] != ' ') {
                Dispatch(std::string(mInputBuffer));
            }
            memset(mInputBuffer, 0, gMaxBufferSize);
        }

        if (mCmdHint != NULLSTR) {
            if (ImGui::IsItemFocused()) {
                ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + size.y));
                ImGui::SameLine();
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(mCmdHint.c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }

        ImGui::SameLine();
#ifdef __WIIU__
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 50 * 2.0f);
#else
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 50);
#endif
        if (ImGui::Button("Submit") && !inputFocus && mInputBuffer[0] != '\0' && mInputBuffer[0] != ' ') {
            Dispatch(std::string(mInputBuffer));
            memset(mInputBuffer, 0, gMaxBufferSize);
        }

        ImGui::SetItemDefaultFocus();
        if (inputFocus) {
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::PopItemWidth();
    }
    ImGui::End();
}

void ConsoleWindow::Dispatch(const std::string& line) {
    mCmdHint = NULLSTR;
    mHistory.push_back(line);
    SendInfoMessage("> " + line);
    const std::vector<std::string> cmdArgs = StringHelper::Split(line, " ");
    if (mCommands.contains(cmdArgs[0])) {
        const CommandEntry entry = mCommands[cmdArgs[0]];
        if (!entry.handler(shared_from_this(), cmdArgs) && !entry.arguments.empty()) {
            SendErrorMessage("[LUS] Usage: " + cmdArgs[0] + " " + BuildUsage(entry));
        }

        return;
    }
    SendErrorMessage("[LUS] Command not found");
}

int ConsoleWindow::CallbackStub(ImGuiInputTextCallbackData* data) {
    const auto instance = static_cast<ConsoleWindow*>(data->UserData);
    const bool emptyHistory = instance->mHistory.empty();
    const int historyIndex = instance->mHistoryIndex;
    std::string history;

    switch (data->EventKey) {
        case ImGuiKey_Tab:
            instance->mAutoComplete.clear();
            for (auto& [cmd, entry] : instance->mCommands) {
                if (cmd.find(std::string(data->Buf)) != std::string::npos) {
                    instance->mAutoComplete.push_back(cmd);
                }
            }
            instance->mOpenAutocomplete = !instance->mAutoComplete.empty();
            instance->mCmdHint = NULLSTR;
            break;
        case ImGuiKey_UpArrow:
            if (emptyHistory) {
                break;
            }
            if (historyIndex < static_cast<int>(instance->mHistory.size()) - 1) {
                instance->mHistoryIndex += 1;
            }
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, instance->mHistory[instance->mHistoryIndex].c_str());
            instance->mCmdHint = NULLSTR;
            break;
        case ImGuiKey_DownArrow:
            if (emptyHistory) {
                break;
            }
            if (historyIndex > -1) {
                instance->mHistoryIndex -= 1;
            }
            data->DeleteChars(0, data->BufTextLen);
            if (historyIndex >= 0) {
                data->InsertChars(0, instance->mHistory[historyIndex].c_str());
            }
            instance->mCmdHint = NULLSTR;
            break;
        case ImGuiKey_Escape:
            instance->mHistoryIndex = -1;
            data->DeleteChars(0, data->BufTextLen);
            instance->mOpenAutocomplete = false;
            instance->mCmdHint = NULLSTR;
            break;
        default:
            instance->mOpenAutocomplete = false;
            for (auto& [cmd, entry] : instance->mCommands) {
                const std::vector<std::string> cmdArgs = StringHelper::Split(std::string(data->Buf), " ");
                if (data->BufTextLen > 2 && !cmdArgs.empty() && cmd.find(cmdArgs[0]) != std::string::npos) {
                    instance->mCmdHint = cmd + " " + BuildUsage(entry);
                    break;
                }
                instance->mCmdHint = NULLSTR;
            }
    }
    return 0;
}

void ConsoleWindow::Append(const std::string& channel, spdlog::level::level_enum priority, const char* fmt, va_list args) {
    char buf[2048];
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    mLog[channel].push_back({ std::string(buf), priority });
}

void ConsoleWindow::Append(const std::string& channel, spdlog::level::level_enum priority, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Append(channel, priority, fmt, args);
    va_end(args);
}

void ConsoleWindow::SendInfoMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Append("Console", spdlog::level::info, fmt, args);
    va_end(args);
}

void ConsoleWindow::SendErrorMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Append("Console", spdlog::level::err, fmt, args);
    va_end(args);
}

void ConsoleWindow::SendInfoMessage(const std::string& str) {
    Append("Console", spdlog::level::info, str.c_str());
}

void ConsoleWindow::SendErrorMessage(const std::string& str) {
    Append("Console", spdlog::level::err, str.c_str());
}

void ConsoleWindow::ClearLogs(std::string channel) {
    mLog[channel].clear();
}

void ConsoleWindow::ClearLogs() {
    for (auto [key, var] : mLog) {
        var.clear();
    }
}

bool ConsoleWindow::HasCommand(const std::string& command) {
    for (const auto& value : mCommands) {
        if (value.first == command) {
            return true;
        }
    }

    return false;
}

void ConsoleWindow::AddCommand(const std::string& command, CommandEntry entry) {
    if (!HasCommand(command)) {
        mCommands[command] = entry;
    }
}

std::string ConsoleWindow::GetCurrentChannel() {
    return mCurrentChannel;
}
} // namespace LUS
