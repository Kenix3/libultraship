#pragma once

#include <stdint.h>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <map>
#include <imgui.h>
#include "ship/Component.h"

namespace Ship {

class Console;

/**
 * @brief Signature of a function that handles a console command.
 *
 * @param console Shared pointer to the owning Console (for recursive dispatch).
 * @param args    Tokenised command arguments (index 0 is the command name).
 * @param output  If non-null, the handler should write its response here.
 * @return 0 on success, non-zero on error.
 */
typedef std::function<int32_t(std::shared_ptr<Console> console, std::vector<std::string> args, std::string* output)>
    CommandHandler;

/** @brief Describes the expected type of a command argument. */
enum class ArgumentType { TEXT, NUMBER };

/**
 * @brief Metadata for a single positional argument of a console command.
 */
struct CommandArgument {
    std::string Info;                         ///< Human-readable description of the argument.
    ArgumentType Type = ArgumentType::NUMBER; ///< Expected value type (text or numeric).
    bool Optional = false;                    ///< If true, the argument may be omitted.
};

/**
 * @brief Registration record for a single console command.
 */
struct CommandEntry {
    CommandHandler Handler;                 ///< Callable invoked when the command is executed.
    std::string Description;                ///< Short description shown by the "help" command.
    std::vector<CommandArgument> Arguments; ///< Ordered list of expected arguments.
};

/**
 * @brief In-process command interpreter used by ConsoleWindow.
 *
 * Console stores a registry of named commands and dispatches input strings
 * to the matching handler. Built-in commands (help, bind, set, get, …) are
 * registered by ConsoleWindow during its initialization.
 *
 * **Required Context children (looked up at runtime):**
 * - **Console** itself — command handlers use
 *   `Context::GetChildren().GetFirst<Console>()` to obtain the Console when
 *   executing commands. This is satisfied automatically once Console is added
 *   to the Context.
 *
 * Obtain the instance from `Context::GetChildren().GetFirst<Console>()`.
 */
class Console : public Component {
  public:
    Console();
    ~Console();

    /**
     * @brief Registers built-in commands. Called by Component::Init().
     *
     * Called automatically by Context::CreateDefaultInstance(); do not call manually.
     * Use Component::Init() to trigger initialization.
     *
     * **Required Context children:** None — Console has no external dependencies
     * at initialization time.
     */
    /**
     * @brief Parses and dispatches a command string.
     *
     * Tokenises @p command, looks up the first token in the command registry,
     * and invokes the matching handler.
     *
     * @param command Raw command string entered by the user (e.g. "set gCvar 1").
     * @param output  If non-null, receives the handler's text response.
     * @return Return value of the handler (0 = success).
     */
    int32_t Run(const std::string& command, std::string* output);

    /**
     * @brief Returns true if a command with the given name is registered.
     * @param command Command name to look up (case-sensitive).
     */
    bool HasCommand(const std::string& command);

    /**
     * @brief Registers a new command.
     * @param command Case-sensitive command name.
     * @param entry   Entry containing the handler, description, and argument list.
     */
    void AddCommand(const std::string& command, CommandEntry entry);

    /**
     * @brief Builds a usage string from the command's argument list.
     * @param command Registered command name.
     * @return Human-readable usage string (e.g. "set <name> <value>").
     */
    std::string BuildUsage(const std::string& command);

    /**
     * @brief Builds a usage string directly from a CommandEntry.
     * @param entry CommandEntry to format.
     * @return Human-readable usage string.
     */
    std::string BuildUsage(const CommandEntry& entry);

    /**
     * @brief Returns a reference to the CommandEntry for the given command name.
     * @param command Registered command name.
     */
    CommandEntry& GetCommand(const std::string& command);

    /**
     * @brief Returns the full command registry (name → entry map).
     *
     * Intended for auto-complete and help listing; prefer HasCommand() / Run() for dispatch.
     */
    std::map<std::string, CommandEntry>& GetCommands();

  protected:
    /**
     * @brief Registers built-in commands. Called automatically by Component::Init().
     *
     * **Required Context children:** None.
     */
    void OnInit(const nlohmann::json& initArgs = nlohmann::json::object()) override;

  private:
    std::map<std::string, CommandEntry> mCommands;
};

} // namespace Ship
