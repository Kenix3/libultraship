#pragma once

#include "../../ImGui/imgui.h"
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include <functional>

class SohConsole;

enum WarnLevels {
    LVL_INFO,
    LVL_LOG,
    LVL_WARNING,
    LVL_ERROR
};

struct ConsoleEntry {
    const char* text;
    WarnLevels level;
};

typedef std::function<void(SohConsole* console, std::vector<std::string> args)> CommandFunc;
extern std::map<std::string, CommandFunc> Commands;

#define CMD_SUCCESS true
#define CMD_FAILED  false

#define LOG(console, msg, ...) console->AddLog(WarnLevels::LVL_LOG, msg, __VA_ARGS__)
#define INFO(console, msg, ...) console->AddLog(WarnLevels::LVL_INFO, msg, __VA_ARGS__)
#define WARNING(console, msg, ...) console->AddLog(WarnLevels::LVL_WARNING, msg, __VA_ARGS__)
#define ERROR(console, msg, ...) console->AddLog(WarnLevels::LVL_ERROR, msg, __VA_ARGS__)


class SohConsole {
public:
    ImVector<ConsoleEntry>       History;

    SohConsole();
    ~SohConsole() {
        ClearLog();
    }

	static char* strdup(const char* s) { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
    static void  strtrim(char* s) { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

    std::vector<std::string> split(std::string text, char separator = ' ', bool keep_quotes = false)
    {
        std::vector<std::string> args;
        char* input = strdup(text.c_str());
        const size_t length = strlen(input);

        bool inQuotes = false;
        size_t count = 0, from = 0;

        for (size_t i = 0; i < length; i++)
        {
            if (input[i] == '"')
            {
                inQuotes = !inQuotes;
            }
            else if (input[i] == separator && !inQuotes)
            {
                size_t strlen = i - from;

                if (strlen > 0)
                {
                    if (!keep_quotes && input[from] == '"' && input[i - 1] == '"')
                    {
                        from++; strlen -= 2;
                    }

                    char* tmp = new char[strlen + 1]();
                    strncpy(tmp, &input[from], strlen);
                    count++;
                    args.emplace_back(tmp);
                }

                from = i + 1;
            }
        }

        if (from < length)
        {
            size_t strlen = length - from;

            if (!keep_quotes && input[from] == L'"' && input[length - 1] == L'"')
            {
                from++; strlen -= 2;
            }

            char* tmp = new char[strlen + 1]();
            strncpy(tmp, &input[from], strlen);
            count++;
            args.emplace_back(tmp);
        }

        return args;
    }

    static void ClearLog();
    static void AddLog( WarnLevels level, const char* fmt, ...);
    void Draw(const char* title, bool* p_open);
    void ExecCommand(const char* input);
	static int TextEditCallbackStub(ImGuiInputTextCallbackData* data) {
        SohConsole* console = static_cast<SohConsole*>(data->UserData);
        return console->TextEditCallback(data);
    }
    int TextEditCallback(ImGuiInputTextCallbackData* data);
};