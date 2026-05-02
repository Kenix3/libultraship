#include "ship/debug/Console.h"
#include "ship/utils/StringHelper.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace Ship {
Console::Console() : Component("Console") {
}

Console::~Console() {
    SPDLOG_TRACE("destruct console");
}

void Console::OnInit() {
}

std::string Console::BuildUsage(const CommandEntry& entry) {
    std::string usage;
    for (const auto& arg : entry.Arguments) {
        usage += StringHelper::Sprintf(arg.Optional ? "[%s] " : "<%s> ", arg.Info.c_str());
    }
    return usage;
}

std::string Console::BuildUsage(const std::string& command) {
    return BuildUsage(GetCommand(command));
}

int32_t Console::Run(const std::string& command, std::string* output) {
    const std::vector<std::string> cmdArgs = StringHelper::Split(command, " ");
    if (cmdArgs.empty()) {
        SPDLOG_INFO("Could not parse command: {}", command);
        return false;
    }

    const std::string& commandName = cmdArgs[0];
    auto it = mCommands.find(commandName);
    if (it == mCommands.end()) {
        SPDLOG_INFO("Command handler not found: {}", commandName);
        return false;
    }

    const CommandEntry& entry = it->second;
    int32_t commandResult = entry.Handler(std::static_pointer_cast<Console>(GetSharedComponent()), cmdArgs, output);
    if (output) {
        SPDLOG_INFO("Command \"{}\" returned {} with output: {}", command, commandResult, *output);
    } else {
        SPDLOG_INFO("Command \"{}\" returned {}", command, commandResult);
    }
    return commandResult;
}

bool Console::HasCommand(const std::string& command) {
    return mCommands.contains(command);
}

void Console::AddCommand(const std::string& command, CommandEntry entry) {
    if (!HasCommand(command)) {
        mCommands[command] = entry;
    } else {
        SPDLOG_WARN("Attempting to add command {} that already exists", command);
    }
}

std::map<std::string, CommandEntry>& Console::GetCommands() {
    return mCommands;
}

CommandEntry& Console::GetCommand(const std::string& command) {
    auto it = mCommands.find(command);
    if (it == mCommands.end()) {
        throw std::out_of_range("Command not found: " + command);
    }
    return it->second;
}
} // namespace Ship
