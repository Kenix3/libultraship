#include "menu/Console.h"

#include "core/bridge/consolevariablebridge.h"
#include "ImGuiImpl.h"
#include <ImGui/imgui.h>
#include <Utils/StringHelper.h>
#include <ImGui/imgui_internal.h>
#include "misc/Utils.h"
#include <sstream>

namespace Ship {
std::string BuildUsage(const CommandEntry& entry) {
    std::string usage;
    for (const auto& arg : entry.arguments) {
        usage += StringHelper::Sprintf(arg.optional ? "[%s] " : "<%s> ", arg.info.c_str());
    }
    return usage;
}

bool Console::HelpCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args) {
    console->SendInfoMessage("Commands:");
    for (const auto& cmd : console->mCommands) {
        console->SendInfoMessage(" - " + cmd.first);
    }
    return CMD_SUCCESS;
}

bool Console::ClearCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args) {
    console->ClearLogs(console->GetCurrentChannel());
    return CMD_SUCCESS;
}

bool Console::BindCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args) {
    if (args.size() > 2) {
        const ImGuiIO* io = &ImGui::GetIO();
        ;
        for (size_t k = 0; k < std::size(io->KeysData); k++) {
            std::string key(ImGui::GetKeyName(k));

            if (toLowerCase(args[1]) == toLowerCase(key)) {
                std::vector<std::string> tmp;
                const char* const delim = " ";
                std::ostringstream imploded;
                std::copy(args.begin() + 2, args.end(), std::ostream_iterator<std::string>(imploded, delim));
                console->mBindings[k] = imploded.str();
                console->SendInfoMessage("Binding '%s' to %s", args[1].c_str(), console->mBindings[k].c_str());
                break;
            }
        }
    }
    return CMD_SUCCESS;
}

bool Console::BindToggleCommand(std::shared_ptr<Console> console, const std::vector<std::string>& args) {
    if (args.size() > 2) {
        const ImGuiIO* io = &ImGui::GetIO();
        ;
        for (size_t k = 0; k < std::size(io->KeysData); k++) {
            std::string key(ImGui::GetKeyName(k));

            if (toLowerCase(args[1]) == toLowerCase(key)) {
                console->mBindingToggle[k] = args[2];
                console->SendInfoMessage("Binding toggle '%s' to %s", args[1].c_str(),
                                         console->mBindingToggle[k].c_str());
                break;
            }
        }
    }
    return CMD_SUCCESS;
}

void Console::Init() {
    this->mInputBuffer = new char[MAX_BUFFER_SIZE];
    strcpy(this->mInputBuffer, "");
    this->mFilterBuffer = new char[MAX_BUFFER_SIZE];
    strcpy(this->mFilterBuffer, "");
    AddCommand("help", { HelpCommand, "Shows all the commands" });
    AddCommand("clear", { ClearCommand, "Clear the console history" });
    AddCommand("bind", { BindCommand, "Binds key to commands" });
    AddCommand("bind-toggle", { BindToggleCommand, "Bind key as a bool toggle" });
}

void Console::Update() {
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

void Console::Draw() {
    if (!this->mOpened) {
        CVarSetInteger("gConsoleEnabled", 0);
        return;
    }

    bool inputFocus = false;

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Console", &this->mOpened, ImGuiWindowFlags_NoFocusOnAppearing);
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    // SohImGui::ShowCursor(ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_RectOnly),
    // SohImGui::Dialogues::dConsole);

    // Renders autocomplete window
    if (this->mOpenAutocomplete) {
        ImGui::SetNextWindowSize(ImVec2(350, std::min(static_cast<int>(this->mAutoComplete.size()), 3) * 20.f),
                                 ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(pos.x + 8, pos.y + size.y - 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));
        ImGui::Begin("##WndAutocomplete", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
        ImGui::BeginChild("AC_Child", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.3f, .3f, .3f, 1.0f));
        if (ImGui::BeginTable("AC_History", 1)) {
            for (const auto& cmd : this->mAutoComplete) {
                std::string usage = BuildUsage(this->mCommands[cmd]);
                std::string preview = cmd + " - " + this->mCommands[cmd].description;
                std::string autoComplete = (usage == NULLSTR ? cmd : usage);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (ImGui::Selectable(preview.c_str())) {
                    memset(this->mInputBuffer, 0, MAX_BUFFER_SIZE);
                    memcpy(this->mInputBuffer, autoComplete.c_str(), sizeof(char) * autoComplete.size());
                    this->mOpenAutocomplete = false;
                    inputFocus = true;
                }
            }
            ImGui::EndTable();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            this->mOpenAutocomplete = false;
        }
        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::End();
        ImGui::PopStyleVar();
    }

    if (ImGui::BeginPopupContextWindow("Context Menu")) {
        if (ImGui::MenuItem("Copy Text")) {
            ImGui::SetClipboardText(this->mLog[this->mCurrentChannel][this->mSelectedId].text.c_str());
            this->mSelectedId = -1;
        }
        ImGui::EndPopup();
    }
    if (this->mSelectedId != -1 && ImGui::IsMouseClicked(1)) {
        ImGui::OpenPopup("##WndAutocomplete");
    }

    // Renders top bar filters
    if (ImGui::Button("Clear")) {
        this->mLog[this->mCurrentChannel].clear();
    }

    if (CVarGetInteger("gSinkEnabled", 0)) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo("##channel", this->mCurrentChannel.c_str())) {
            for (const auto& channel : mLogChannels) {
                const bool isSelected = (channel == std::string(this->mCurrentChannel));
                if (ImGui::Selectable(channel.c_str(), isSelected)) {
                    this->mCurrentChannel = channel;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    } else {
        this->mCurrentChannel = "Console";
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);

    if (this->mCurrentChannel != "Console") {
        if (ImGui::BeginCombo("##level", spdlog::level::to_string_view(this->mLevelFilter).data())) {
            for (const auto& priorityFilter : mPriorityFilters) {
                const bool isSelected = priorityFilter == this->mLevelFilter;
                if (ImGui::Selectable(spdlog::level::to_string_view(priorityFilter).data(), isSelected)) {
                    this->mLevelFilter = priorityFilter;
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }
            ImGui::EndCombo();
        }
    } else {
        this->mLevelFilter = spdlog::level::trace;
    }
    ImGui::SameLine();
    ImGui::PushItemWidth(-1);
    if (ImGui::InputTextWithHint("##input", "Filter", this->mFilterBuffer, MAX_BUFFER_SIZE)) {
        this->mFilter = std::string(this->mFilterBuffer);
    }
    ImGui::PopItemWidth();

    // Renders console history
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.3f, .3f, .3f, 1.0f));
    if (ImGui::BeginTable("History", 1)) {

        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
            if (this->mSelectedId < (int)this->mLog.size() - 1) {
                ++this->mSelectedId;
            }
        }
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
            if (this->mSelectedId > 0) {
                --this->mSelectedId;
            }
        }

        const std::vector<ConsoleLine> channel = this->mLog[this->mCurrentChannel];
        for (int i = 0; i < static_cast<int>(channel.size()); i++) {
            ConsoleLine line = channel[i];
            if (!this->mFilter.empty() && line.text.find(this->mFilter) == std::string::npos) {
                continue;
            }
            if (this->mLevelFilter > line.priority) {
                continue;
            }
            std::string id = line.text + "##" + std::to_string(i);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const bool isSelected = (this->mSelectedId == i) ||
                                    std::find(this->mSelectedEntries.begin(), this->mSelectedEntries.end(), i) !=
                                        this->mSelectedEntries.end();
            ImGui::PushStyleColor(ImGuiCol_Text, this->mPriorityColours[line.priority]);
            if (ImGui::Selectable(id.c_str(), isSelected)) {
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftCtrl)) && !isSelected) {
                    this->mSelectedEntries.push_back(i);

                } else {
                    this->mSelectedEntries.clear();
                }
                this->mSelectedId = isSelected ? -1 : i;
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

    if (this->mCurrentChannel == "Console") {
        // Renders input textfield
        constexpr ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackEdit |
                                              ImGuiInputTextFlags_CallbackCompletion |
                                              ImGuiInputTextFlags_CallbackHistory;
#ifdef __WIIU__
        ImGui::PushItemWidth(-53.0f * 2.0f);
#else
        ImGui::PushItemWidth(-53.0f);
#endif
        if (ImGui::InputTextWithHint("##CMDInput", ">", this->mInputBuffer, MAX_BUFFER_SIZE, flags,
                                     &Console::CallbackStub, this)) {
            inputFocus = true;
            if (this->mInputBuffer[0] != '\0' && this->mInputBuffer[0] != ' ') {
                this->Dispatch(std::string(this->mInputBuffer));
            }
            memset(this->mInputBuffer, 0, MAX_BUFFER_SIZE);
        }

        if (this->mCmdHint != NULLSTR) {
            if (ImGui::IsItemFocused()) {
                ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + size.y));
                ImGui::SameLine();
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(this->mCmdHint.c_str());
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
        if (ImGui::Button("Submit") && !inputFocus && this->mInputBuffer[0] != '\0' && this->mInputBuffer[0] != ' ') {
            this->Dispatch(std::string(this->mInputBuffer));
            memset(this->mInputBuffer, 0, MAX_BUFFER_SIZE);
        }

        ImGui::SetItemDefaultFocus();
        if (inputFocus) {
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::PopItemWidth();
    }
    ImGui::End();
}

void Console::Dispatch(const std::string& line) {
    this->mCmdHint = NULLSTR;
    this->mHistory.push_back(line);
    SendInfoMessage("> " + line);
    const std::vector<std::string> cmd_args = StringHelper::Split(line, " ");
    if (this->mCommands.contains(cmd_args[0])) {
        const CommandEntry entry = this->mCommands[cmd_args[0]];
        if (!entry.handler(shared_from_this(), cmd_args) && !entry.arguments.empty()) {
            SendErrorMessage("[SOH] Usage: " + cmd_args[0] + " " + BuildUsage(entry));
        }

        return;
    }
    SendErrorMessage("[SOH] Command not found");
}

int Console::CallbackStub(ImGuiInputTextCallbackData* data) {
    const auto instance = static_cast<Console*>(data->UserData);
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
                const std::vector<std::string> cmd_args = StringHelper::Split(std::string(data->Buf), " ");
                if (data->BufTextLen > 2 && !cmd_args.empty() && cmd.find(cmd_args[0]) != std::string::npos) {
                    instance->mCmdHint = cmd + " " + BuildUsage(entry);
                    break;
                }
                instance->mCmdHint = NULLSTR;
            }
    }
    return 0;
}

void Console::Append(const std::string& channel, spdlog::level::level_enum priority, const char* fmt, va_list args) {
    char buf[2048];
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    this->mLog[channel].push_back({ std::string(buf), priority });
}

void Console::Append(const std::string& channel, spdlog::level::level_enum priority, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Append(channel, priority, fmt, args);
    va_end(args);
}

void Console::SendInfoMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Append("Console", spdlog::level::info, fmt, args);
    va_end(args);
}

void Console::SendErrorMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Append("Console", spdlog::level::err, fmt, args);
    va_end(args);
}

void Console::SendInfoMessage(const std::string& str) {
    Append("Console", spdlog::level::info, str.c_str());
}

void Console::SendErrorMessage(const std::string& str) {
    Append("Console", spdlog::level::err, str.c_str());
}

void Console::ClearLogs(std::string channel) {
    mLog[channel].clear();
}

void Console::ClearLogs() {
    for (auto [key, var] : mLog) {
        var.clear();
    }
}

bool Console::HasCommand(const std::string& command) {
    for (const auto& value : mCommands) {
        if (value.first == command) {
            return true;
        }
    }

    return false;
}

void Console::AddCommand(const std::string& command, CommandEntry entry) {
    if (!HasCommand(command)) {
        mCommands[command] = entry;
    }
}

std::string Console::GetCurrentChannel() {
    return mCurrentChannel;
}

bool Console::IsOpened() {
    return mOpened;
}

void Console::Close() {
    mOpened = false;
}

void Console::Open() {
    mOpened = true;
}
} // namespace Ship