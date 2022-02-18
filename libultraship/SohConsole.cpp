#include "SohConsole.h"

#include <iostream>

#include "Lib/ImGui/imgui.h"
#include "Utils/StringHelper.h"
#include "SohImGuiImpl.h"

static char filterBuffer[MAX_BUFFER_SIZE];

static bool HelpCommand(std::vector<std::string> args) {
	INFO("SoH Commands:");
	for(auto cmd : SohImGui::console->Commands) {
		INFO((" - " + cmd.first).c_str());
	}
	return CMD_SUCCESS;
}

static bool ClearCommand(std::vector<std::string> args) {
	SohImGui::console->history[SohImGui::console->selected_channel].clear();
	return CMD_SUCCESS;
}


void Console::Init() {
	this->Commands["help"]  = { HelpCommand };
	this->Commands["clear"] = { ClearCommand };
}

void Console::Update() {

}

void Console::Draw() {
	if (!this->opened) return;
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	ImGui::Begin("Console", &this->opened);
		if (ImGui::BeginPopupContextWindow("Context Menu")) {
			if (ImGui::MenuItem("Copy Text")) {
				ImGui::SetClipboardText(this->history[this->selected_channel][this->selectedId].text.c_str());
				this->selectedId = -1;
			}
			ImGui::EndPopup();
		}
		if (this->selectedId != -1 && ImGui::IsMouseClicked(1)) {
			ImGui::OpenPopup("Context Menu");
		}

		if (ImGui::Button("Clear")) this->history[this->selected_channel].clear();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		if (ImGui::BeginCombo("##channel", this->selected_channel.c_str())) {
			for (auto channel : log_channels) {
				const bool is_selected = (channel == std::string(this->selected_channel));
				if (ImGui::Selectable(channel.c_str(), is_selected))
					this->selected_channel = channel;
				if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		if (ImGui::BeginCombo("##level", this->level_filter.c_str())) {
			for (auto filter : priority_filters) {
				const bool is_selected = (filter == std::string(this->level_filter));
				if (ImGui::Selectable(filter.c_str(), is_selected))
					this->level_filter = filter;
					if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::PushItemWidth(-1);
		const size_t filterSize = IM_ARRAYSIZE(filterBuffer);
		if (ImGui::InputTextWithHint("##input", "Filter", filterBuffer, filterSize))this->filter = std::string(filterBuffer);
		ImGui::PopItemWidth();
		const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(.3f, .3f, .3f, 1.0f));
			if (ImGui::BeginTable("History", 1)) {

				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
					if (this->selectedId < this->history.size() - 1) { ++this->selectedId; }
				}
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
					if (this->selectedId > 0) { --this->selectedId; }
				}

				std::vector<ConsoleLine> channel = this->history[this->selected_channel];
				for (int i = 0; i < channel.size(); i++) {
					ConsoleLine line = channel[i];
					if(!this->filter.empty() && line.text.find(this->filter) == std::string::npos) continue;
					if(this->level_filter != "None" && line.priority != (std::ranges::find(priority_filters, this->level_filter) - priority_filters.begin()) - 1) continue;
					std::string id = line.text + "##" + std::to_string(i);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					const bool is_selected = (this->selectedId == i) || std::ranges::find(this->selectedEntries, i) != this->selectedEntries.end();
					ImGui::PushStyleColor(ImGuiCol_Text, this->priority_colors[line.priority]);
					if (ImGui::Selectable(id.c_str(), is_selected)) {
						if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftCtrl)) && !is_selected)
							this->selectedEntries.push_back(i);

						else this->selectedEntries.clear();
						this->selectedId = is_selected ? -1 : i;
					}
					ImGui::PopStyleColor();
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndTable();
			}
			ImGui::PopStyleColor();
			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
		ImGui::EndChild();

		bool input_focus = false;
		static char buffer[MAX_BUFFER_SIZE] = "";
		constexpr size_t bufferSize = IM_ARRAYSIZE(buffer);
		constexpr ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
		ImGui::PushItemWidth(-1);
		if(ImGui::InputTextWithHint("CMDInput", ">", buffer, bufferSize, flags, &Console::CallbackStub, this)) {
			input_focus = true;
			this->Dispatch(std::string(buffer));
			memset(buffer, 0, bufferSize);
		}
		ImGui::SetItemDefaultFocus();
		if (input_focus) ImGui::SetKeyboardFocusHere(-1);
		ImGui::PopItemWidth();
	ImGui::End();
}

void Console::Dispatch(const std::string line) {
	this->history[this->selected_channel].push_back({ "> " + line } );
	const std::vector<std::string> cmd_args = StringHelper::Split(line, " ");
	if (this->Commands.contains(cmd_args[0])) {
		const CommandEntry entry = this->Commands[cmd_args[0]];
		if(!entry.handler(cmd_args) && entry.usage != "None")
			this->history[this->selected_channel].push_back({ "[SOH] Usage: " + entry.usage, ERROR_LVL });
		return;
	}
	this->history[this->selected_channel].push_back({ "[SOH] Command not found", ERROR_LVL });
}

int Console::CallbackStub(ImGuiInputTextCallbackData* data) {
	return 0;
}

void Console::Append(std::string channel, Priority priority, const char* fmt, ...) IM_FMTARGS(4) {
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
	buf[IM_ARRAYSIZE(buf) - 1] = 0;
	va_end(args);
	this->history[channel].push_back({ std::string(buf), priority });
}