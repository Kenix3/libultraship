#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <map>
#include <ImGui/imgui.h>


namespace LUS {

class Console;
typedef std::function<bool(std::shared_ptr<Console> console, std::vector<std::string> args, std::string* output)> CommandHandler;

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

class Console {
public:
    void Init();
    bool Run(const std::string& command, std::string* output);
    bool HasCommand(const std::string& command);
    void AddCommand(const std::string& command, CommandEntry entry);
    std::string BuildUsage(const std::string& command);
    std::string BuildUsage(const CommandEntry& entry);
    CommandEntry& GetCommand(const std::string& command);
    std::map<std::string, CommandEntry>& GetCommands();
protected:

private:
    std::map<std::string, CommandEntry> mCommands;
};

} // namespace LUS
