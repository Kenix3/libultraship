#include "Console.h"
#include "misc/Utils.h"
#include "Utils/StringHelper.h"
#include "core/Context.h"

namespace LUS {
void Console::Init() {
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

bool Console::Run(const std::string& command, std::string* output) {
    const std::vector<std::string> cmdArgs = StringHelper::Split(command, " ");
    if (mCommands.contains(cmdArgs[0])) {
        const CommandEntry entry = mCommands[cmdArgs[0]];
        return entry.Handler(Context::GetInstance()->GetConsole(), cmdArgs, output);
    }

    return false;
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
    } else {
        SPDLOG_WARN("Attempting to add command {} that already exists", command);
    }
}

std::map<std::string, CommandEntry>& Console::GetCommands() {
    return mCommands;
}

CommandEntry& Console::GetCommand(const std::string& command) {
    return mCommands[command];
}
} // namespace LUS
