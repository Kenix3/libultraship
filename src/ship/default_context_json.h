#pragma once

// Auto-generated from default_context.json — do not edit manually.
// clang-format off
namespace Ship {
inline constexpr const char* kDefaultContextJson = R"JSON({
    "components": [
        {
            "type": "Logger",
            "name": "Logger"
        },
        {
            "type": "Config",
            "name": "Config"
        },
        {
            "type": "ConsoleVariable",
            "name": "ConsoleVariable"
        },
        {
            "type": "ThreadPool",
            "name": "ThreadPool"
        },
        {
            "type": "Keystore",
            "name": "Keystore",
            "condition": "ENABLE_SCRIPTING"
        },
        {
            "type": "ResourceManager",
            "name": "ResourceManager"
        },
        {
            "type": "ControlDeck",
            "name": "ControlDeck"
        },
        {
            "type": "CrashHandler",
            "name": "CrashHandler"
        },
        {
            "type": "Console",
            "name": "Console"
        },
        {
            "type": "Window",
            "name": "Window"
        },
        {
            "type": "Audio",
            "name": "Audio"
        },
        {
            "type": "GfxDebugger",
            "name": "GfxDebugger"
        },
        {
            "type": "Events",
            "name": "Events"
        },
        {
            "type": "FileDrop",
            "name": "FileDrop"
        },
        {
            "type": "ScriptLoader",
            "name": "ScriptLoader",
            "condition": "ENABLE_SCRIPTING"
        }
    ]
}
)JSON";
} // namespace Ship
// clang-format on
