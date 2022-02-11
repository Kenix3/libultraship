#include "SohConsole.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

std::map<std::string, CommandFunc> Commands;
char                  InputBuf[256];
ImVector<ConsoleEntry>       Items;
int                   HistoryPos;
ImGuiTextFilter       Filter;
bool                  AutoScroll;
bool                  ScrollToBottom;

SohConsole::SohConsole() {
    ClearLog();
    memset(InputBuf, 0, sizeof(InputBuf));
    HistoryPos = -1;
    AutoScroll = true;
    ScrollToBottom = false;
}

void SohConsole::ClearLog() {
    Items.clear();
}

void SohConsole::AddLog( WarnLevels level, const char* fmt, ...) IM_FMTARGS(3) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);
    Items.push_back({ strdup(buf), level });
}

void SohConsole::Draw(const char* title, bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Close Console"))
            *p_open = false;
        ImGui::EndPopup();
    }

    if (ImGui::Button("Clear")) { ClearLog(); }
    // static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); LOG(this, "Spam %f", t); }
    ImGui::SameLine();

    if (ImGui::BeginPopup("Options")) {
        ImGui::Checkbox("Auto-scroll", &AutoScroll);
        ImGui::EndPopup();
    }

    Filter.Draw(R"(Filter ("incl,-excl") ("error"))", 300);
    ImGui::Separator();

    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
    if (ImGui::BeginPopupContextWindow()) {
        if (ImGui::Selectable("Clear")) ClearLog();
        ImGui::EndPopup();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
    for (int i = 0; i < Items.Size; i++)
    {
        ConsoleEntry entry = Items[i];

        const char* item = Items[i].text;
        if (!Filter.PassFilter(item))
            continue;

        ImVec4 warnColors[] = {
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
            ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
            ImVec4(0.9f, 0.8f, 0.4f, 0.01f),
            ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
        };

        ImGui::PushStyleColor(ImGuiCol_Text, warnColors[entry.level]);
        ImGui::TextUnformatted(item);
	ImGui::PopStyleColor();
    }

    if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        ImGui::SetScrollHereY(1.0f);
    ScrollToBottom = false;

    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();

    bool reclaim_focus = false;
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this)) {
        char* s = InputBuf;
        strtrim(s);
        if (s[0]) ExecCommand(s);
        strcpy(s, "");
        reclaim_focus = true;
    }

    ImGui::SetItemDefaultFocus();
    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

    ImGui::End();
}

void SohConsole::ExecCommand(const char* command_line) {

    std::string cmd(command_line);
    INFO(this, "[SOH] %s", command_line);

    HistoryPos = -1;
    for (int i = History.Size - 1; i >= 0; i--)
        if (History[i].text == command_line) {
            History.erase(History.begin() + i);
            break;
        }

    History.push_back({ command_line, LVL_INFO });
    ScrollToBottom = true;

    if (cmd == "clear") {
        ClearLog();
        return;
    }
    if (cmd == "help") {
        INFO(this, "Commands:");
        for (auto const& cmd : Commands)
            INFO(this, " - %s", cmd.first.c_str());
        return;
    }
    if (cmd == "history") {
        int first = History.Size - 10;
        for (int i = first > 0 ? first : 0; i < History.Size; i++)
            INFO(this, "[SOH] %3d: %s\n", i, History[i].text);
        return;
    }
    std::vector<std::string> args = split(cmd);
    if (Commands.contains(args[0])) {
        Commands[args[0]](this, args);
        return;
    }
    ERROR(this, "[SOH] Unknown command: '%s'\n", args[0].c_str());
}

int SohConsole::TextEditCallback(ImGuiInputTextCallbackData* data) {
    switch (data->EventFlag) {
	    case ImGuiInputTextFlags_CallbackCompletion: {
	        const char* word_end = data->Buf + data->CursorPos;
	        const char* word_start = word_end;
	        while (word_start > data->Buf)
	        {
	            const char c = word_start[-1];
	            if (c == ' ' || c == '\t' || c == ',' || c == ';')
	                break;
	            word_start--;
	        }

	        // Build a list of candidates
	        ImVector<const char*> candidates;
	        for (auto const& cmd : Commands)
                if(cmd.first.find(std::string(word_start)) != std::string::npos)
			candidates.push_back(cmd.first.c_str());

            if (candidates.Size == 0)
                ERROR(this, "No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
	        else if (candidates.Size == 1) {
	            data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
	            data->InsertChars(data->CursorPos, candidates[0]);
	            data->InsertChars(data->CursorPos, " ");
	        } else {
	            int match_len = (int)(word_end - word_start);
	            for (;;)
	            {
	                int c = 0;
	                bool all_candidates_matches = true;
	                for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
	                    if (i == 0)
	                        c = toupper(candidates[i][match_len]);
	                    else if (c == 0 || c != toupper(candidates[i][match_len]))
	                        all_candidates_matches = false;
	                if (!all_candidates_matches)
	                    break;
	                match_len++;
	            }

	            if (match_len > 0)
	            {
	                data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
	                data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
	            }

	            INFO(this, "Possible matches:\n");
	            for (int i = 0; i < candidates.Size; i++)
                    INFO(this, "- %s\n", candidates[i]);
	        }

	        break;
	    }
	    case ImGuiInputTextFlags_CallbackHistory: {
	        const int prev_history_pos = HistoryPos;
	        if (data->EventKey == ImGuiKey_UpArrow)
	        {
	            if (HistoryPos == -1)
	                HistoryPos = History.Size - 1;
	            else if (HistoryPos > 0)
	                HistoryPos--;
	        }
	        else if (data->EventKey == ImGuiKey_DownArrow)
	        {
	            if (HistoryPos != -1)
	                if (++HistoryPos >= History.Size)
	                    HistoryPos = -1;
	        }

	        // A better implementation would preserve the data on the current input line along with cursor position.
	        if (prev_history_pos != HistoryPos)
	        {
	            const char* history_str = (HistoryPos >= 0) ? History[HistoryPos].text : "";
	            data->DeleteChars(0, data->BufTextLen);
	            data->InsertChars(0, history_str);
	        }
	    }
    }
    return 0;
}