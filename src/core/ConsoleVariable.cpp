#include "ConsoleVariable.h"

#include <functional>
#include <Utils/File.h>
#include <misc/Utils.h>
#include "Mercury.h"
#include "core/Window.h"

namespace Ship {

ConsoleVariable::ConsoleVariable() {
}

std::shared_ptr<CVar> ConsoleVariable::Get(const char* name) {
    auto it = mVariables.find(name);
    return it != mVariables.end() ? it->second : nullptr;
}

int32_t ConsoleVariable::GetInteger(const char* name, int32_t defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::Integer) {
        return variable->Integer;
    }

    return defaultValue;
}

float ConsoleVariable::GetFloat(const char* name, float defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::Float) {
        return variable->Integer;
    }

    return defaultValue;
}

const char* ConsoleVariable::GetString(const char* name, const char* defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::String) {
        return variable->String.c_str();
    }

    return defaultValue;
}

Color_RGBA8 ConsoleVariable::GetColor(const char* name, Color_RGBA8 defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::Color) {
        return variable->Color;
    }

    return defaultValue;
}

void ConsoleVariable::SetInteger(const char* name, int32_t value) {
    auto variable = Get(name);
    if (variable == nullptr) {
        variable = std::make_shared<CVar>();
    }

    variable->Type = ConsoleVariableType::Integer;
    variable->Integer = value;
}

void ConsoleVariable::SetFloat(const char* name, float value) {
    auto variable = Get(name);
    if (variable == nullptr) {
        variable = std::make_shared<CVar>();
    }

    variable->Type = ConsoleVariableType::Float;
    variable->Float = value;
}

void ConsoleVariable::SetString(const char* name, const char* value) {
    auto variable = Get(name);
    if (variable == nullptr) {
        variable = std::make_shared<CVar>();
    }

    variable->Type = ConsoleVariableType::String;
    variable->String = value;
}

void ConsoleVariable::SetColor(const char* name, Color_RGBA8 value) {
    auto variable = Get(name);
    if (!variable) {
        variable = std::make_shared<CVar>();
    }

    variable->Type = ConsoleVariableType::Color;
    variable->Color = value;
}

void ConsoleVariable::RegisterInteger(const char* name, int32_t defaultValue) {
    if (Get(name) != nullptr) {
        SetInteger(name, defaultValue);
    }
}

void ConsoleVariable::RegisterFloat(const char* name, float defaultValue) {
    if (Get(name) != nullptr) {
        SetFloat(name, defaultValue);
    }
}

void ConsoleVariable::RegisterString(const char* name, const char* defaultValue) {
    if (Get(name) != nullptr) {
        SetString(name, defaultValue);
    }
}

void ConsoleVariable::RegisterColor(const char* name, Color_RGBA8 defaultValue) {
    if (Get(name) != nullptr) {
        SetColor(name, defaultValue);
    }
}

void ConsoleVariable::ClearVariable(const char* name) {
    std::shared_ptr<Mercury> conf = Ship::Window::GetInstance()->GetConfig();
    mVariables.erase(name);
    conf->erase(StringHelper::Sprintf("CVars.%s", name));
}

void ConsoleVariable::Save() {
    std::shared_ptr<Mercury> conf = Ship::Window::GetInstance()->GetConfig();

    for (const auto& variable : mVariables) {
        const std::string key = StringHelper::Sprintf("CVars.%s", variable.first.c_str());

        if (variable.second->Type == ConsoleVariableType::String && variable.second->String.length() > 0) {
            conf->setString(key, std::string(variable.second->String));
        } else if (variable.second->Type == ConsoleVariableType::Integer) {
            conf->setInt(key, variable.second->Integer);
        } else if (variable.second->Type == ConsoleVariableType::Float) {
            conf->setFloat(key, variable.second->Float);
        } else if (variable.second->Type == ConsoleVariableType::Color) {
            auto keyStr = key.c_str();
            Color_RGBA8 clr = variable.second->Color;
            conf->setUInt(StringHelper::Sprintf("%s.R", keyStr), clr.r);
            conf->setUInt(StringHelper::Sprintf("%s.G", keyStr), clr.g);
            conf->setUInt(StringHelper::Sprintf("%s.B", keyStr), clr.b);
            conf->setUInt(StringHelper::Sprintf("%s.A", keyStr), clr.a);
            conf->setString(StringHelper::Sprintf("%s.Type", keyStr), mercuryRGBAObjectType);
        }
    }

    conf->save();
}

void ConsoleVariable::Load() {
    std::shared_ptr<Mercury> conf = Ship::Window::GetInstance()->GetConfig();
    conf->reload();

    LoadFromPath("", conf->rjson["CVars"].items());

    LoadLegacy();
}

void ConsoleVariable::LoadFromPath(
    std::string path, nlohmann::detail::iteration_proxy<nlohmann::detail::iter_impl<nlohmann::json>> items) {
    if (!path.empty()) {
        path += ".";
    }

    for (const auto& item : items) {
        std::string itemPath = path + item.key();
        auto value = item.value();
        switch (value.type()) {
            case nlohmann::detail::value_t::array:
                break;
            case nlohmann::detail::value_t::object:
                if (value.contains("Type") && value["Type"].get<std::string>() == mercuryRGBAObjectType) {
                    Color_RGBA8 clr;
                    clr.r = value["R"].get<uint8_t>();
                    clr.g = value["G"].get<uint8_t>();
                    clr.b = value["B"].get<uint8_t>();
                    clr.a = value["A"].get<uint8_t>();
                    SetColor(itemPath.c_str(), clr);
                } else {
                    LoadFromPath(itemPath, value.items());
                }

                break;
            case nlohmann::detail::value_t::string:
                SetString(itemPath.c_str(), value.get<std::string>().c_str());
                break;
            case nlohmann::detail::value_t::boolean:
                SetInteger(itemPath.c_str(), value.get<bool>());
                break;
            case nlohmann::detail::value_t::number_unsigned:
            case nlohmann::detail::value_t::number_integer:
                SetInteger(itemPath.c_str(), value.get<int>());
                break;
            case nlohmann::detail::value_t::number_float:
                SetFloat(itemPath.c_str(), value.get<float>());
                break;
            default:;
        }
    }
}
void ConsoleVariable::LoadLegacy() {
    auto conf = Ship::Window::GetPathRelativeToAppDirectory("cvars.cfg");
    if (File::Exists(conf)) {
        const auto lines = File::ReadAllLines(conf);

        for (const std::string& line : lines) {
            std::vector<std::string> cfg = StringHelper::Split(line, " = ");
            if (line.empty()) {
                continue;
            }
            if (cfg.size() < 2) {
                continue;
            }

            if (cfg[1].find("\"") == std::string::npos && (cfg[1].find("#") != std::string::npos)) {
                std::string value(cfg[1]);
                value.erase(std::remove_if(value.begin(), value.end(), [](char c) { return c == '#'; }), value.end());
                auto splitTest = StringHelper::Split(value, "\r")[0];

                uint32_t val = std::stoul(splitTest, nullptr, 16);
                Color_RGBA8 clr;
                clr.r = val >> 24;
                clr.g = val >> 16;
                clr.b = val >> 8;
                clr.a = val & 0xFF;
                SetColor(cfg[0].c_str(), clr);
            }

            if (cfg[1].find("\"") != std::string::npos) {
                std::string value(cfg[1]);
                value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
#ifdef _MSC_VER
                SetString(cfg[0].c_str(), _strdup(value.c_str()));
#else
                SetString(cfg[0].c_str(), strdup(value.c_str()));
#endif
            }
            if (Math::IsNumber<float>(cfg[1])) {
                SetFloat(cfg[0].c_str(), std::stof(cfg[1]));
            }
            if (Math::IsNumber<int>(cfg[1])) {
                SetInteger(cfg[0].c_str(), std::stoi(cfg[1]));
            }
        }

        fs::remove(conf);
    }
}
} // namespace Ship
