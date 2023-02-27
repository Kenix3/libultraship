#include "ConsoleVariable.h"

#include <functional>
#include <Utils/File.h>
#include <misc/Utils.h>
#include "Mercury.h"
#include "core/Window.h"

namespace Ship {

ConsoleVariable::ConsoleVariable() {
}

std::shared_ptr<CVar> ConsoleVariable::Get(const std::string& name) {
    auto it = mVariables.find(name);
    return it != mVariables.end() ? it->second : nullptr;
}

int32_t ConsoleVariable::GetInteger(const std::string& name, int32_t defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::Integer) {
        assert(Get<int32_t>(name) == variable->Integer);

        return variable->Integer;
    }

    RegisterEmbedded(name, defaultValue);
    assert(Get<int32_t>(name) == defaultValue);

    return defaultValue;
}

float ConsoleVariable::GetFloat(const std::string& name, float defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::Float) {
        assert(Get<float>(name) == variable->Float);

        return variable->Float;
    }

    RegisterEmbedded(name, defaultValue);
    assert(Get<float>(name) == defaultValue);

    return defaultValue;
}

const char* ConsoleVariable::GetString(const std::string& name, const char* defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::String) {
        assert(Get<std::string>(name) == variable->String);

        return variable->String.c_str();
    }

    RegisterEmbedded(name, std::string(defaultValue));
    assert(Get<std::string>(name) == defaultValue);

    return defaultValue;
}

Color_RGBA8 ConsoleVariable::GetColor(const std::string& name, Color_RGBA8 defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::Color) {
        assert(Get<Color_RGBA8>(name) == variable->Color);

        return variable->Color;
    } else if (variable != nullptr && variable->Type == ConsoleVariableType::Color24) {
        Color_RGBA8 temp;
        temp.r = variable->Color24.r;
        temp.g = variable->Color24.g;
        temp.b = variable->Color24.b;
        temp.a = 255;
        return temp;
    }

    RegisterEmbedded(name, defaultValue);
    assert(Get<Color_RGBA8>(name) == defaultValue);

    return defaultValue;
}

Color_RGB8 ConsoleVariable::GetColor24(const std::string& name, Color_RGB8 defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::Color24) {
        assert(Get<Color_RGB8>(name) == variable->Color24);

        return variable->Color24;
    } else if (variable != nullptr && variable->Type == ConsoleVariableType::Color) {
        Color_RGB8 temp;
        temp.r = variable->Color.r;
        temp.g = variable->Color.g;
        temp.b = variable->Color.b;
        return temp;
    }

    RegisterEmbedded(name, defaultValue);
    assert(Get<Color_RGB8>(name) == defaultValue);

    return defaultValue;
}

void ConsoleVariable::SetInteger(const std::string& name, int32_t value) {
    auto& variable = mVariables[name];
    if (variable == nullptr) {
        variable = std::make_shared<CVar>();

        RegisterEmbedded(name, value);
    }

    variable->Type = ConsoleVariableType::Integer;
    variable->Integer = value;

    Set<int32_t>(name, value);
}

void ConsoleVariable::SetFloat(const std::string& name, float value) {
    auto& variable = mVariables[name];
    if (variable == nullptr) {
        variable = std::make_shared<CVar>();

        RegisterEmbedded(name, value);
    }

    variable->Type = ConsoleVariableType::Float;
    variable->Float = value;

    Set<float>(name, value);
}

void ConsoleVariable::SetString(const std::string& name, const char* value) {
    auto& variable = mVariables[name];
    if (variable == nullptr) {
        variable = std::make_shared<CVar>();

        RegisterEmbedded(name, std::string(value));
    }

    variable->Type = ConsoleVariableType::String;
    variable->String = std::string(value);

    Set<std::string>(name, value);
}

void ConsoleVariable::SetColor(const std::string& name, Color_RGBA8 value) {
    auto& variable = mVariables[name];
    if (!variable) {
        variable = std::make_shared<CVar>();

        RegisterEmbedded(name, value);
    }

    variable->Type = ConsoleVariableType::Color;
    variable->Color = value;

    Set<Color_RGBA8>(name, value);
}

void ConsoleVariable::SetColor24(const std::string& name, Color_RGB8 value) {
    auto& variable = mVariables[name];
    if (!variable) {
        variable = std::make_shared<CVar>();

        RegisterEmbedded(name, value);
    }

    variable->Type = ConsoleVariableType::Color24;
    variable->Color24 = value;

    Set<Color_RGB8>(name, value);
}

void ConsoleVariable::RegisterInteger(const std::string& name, int32_t defaultValue) {
    if (Get(name) == nullptr) {
        SetInteger(name, defaultValue);
    }
}

void ConsoleVariable::RegisterFloat(const std::string& name, float defaultValue) {
    if (Get(name) == nullptr) {
        SetFloat(name, defaultValue);
    }
}

void ConsoleVariable::RegisterString(const std::string& name, const char* defaultValue) {
    if (Get(name) == nullptr) {
        SetString(name, defaultValue);
    }
}

void ConsoleVariable::RegisterColor(const std::string& name, Color_RGBA8 defaultValue) {
    if (Get(name) == nullptr) {
        SetColor(name, defaultValue);
    }
}

void ConsoleVariable::RegisterColor24(const std::string& name, Color_RGB8 defaultValue) {
    if (Get(name) == nullptr) {
        SetColor24(name, defaultValue);
    }
}

void ConsoleVariable::ClearVariable(const std::string& name) {
    std::shared_ptr<Mercury> conf = Ship::Window::GetInstance()->GetConfig();
    mVariables.erase(name);
    conf->erase(StringHelper::Sprintf("CVars.%s", name.c_str()));
    mVariables_New.erase(name);
    conf->erase(StringHelper::Sprintf("CVars2.%s", name.c_str()));
}

void ConsoleVariable::Save() {
    std::shared_ptr<Mercury> conf = Ship::Window::GetInstance()->GetConfig();

    for (const auto& variable : mVariables) {
        const std::string key = StringHelper::Sprintf("CVars.%s", variable.first.c_str());

        if (variable.second->Type == ConsoleVariableType::String && variable.second != nullptr &&
            variable.second->String.length() > 0) {
            conf->setString(key, std::string(variable.second->String));
        } else if (variable.second->Type == ConsoleVariableType::Integer) {
            conf->setInt(key, variable.second->Integer);
        } else if (variable.second->Type == ConsoleVariableType::Float) {
            conf->setFloat(key, variable.second->Float);
        } else if (variable.second->Type == ConsoleVariableType::Color ||
                   variable.second->Type == ConsoleVariableType::Color24) {
            auto keyStr = key.c_str();
            Color_RGBA8 clr = variable.second->Color;
            conf->setUInt(StringHelper::Sprintf("%s.R", keyStr), clr.r);
            conf->setUInt(StringHelper::Sprintf("%s.G", keyStr), clr.g);
            conf->setUInt(StringHelper::Sprintf("%s.B", keyStr), clr.b);
            if (variable.second->Type == ConsoleVariableType::Color) {
                conf->setUInt(StringHelper::Sprintf("%s.A", keyStr), clr.a);
                conf->setString(StringHelper::Sprintf("%s.Type", keyStr), mercuryRGBAObjectType);
            } else {
                conf->setString(StringHelper::Sprintf("%s.Type", keyStr), mercuryRGBObjectType);
            }
        }
    }

    for (const auto& variable : mVariables_New) {
        if (variable.second->ShouldSave()) {
            const std::string key = StringHelper::Sprintf("CVars2.%s", variable.first.c_str());
            CVarInterface::VauleType cvarValue = variable.second->Get();
            if (int32_t* data = std::get_if<int32_t>(&cvarValue)) {
                conf->setInt(key, *data);
            } else if (float* data = std::get_if<float>(&cvarValue)) {
                conf->setFloat(key, *data);
            } else if (std::string* data = std::get_if<std::string>(&cvarValue)) {
                if (data->length() > 0) {
                    conf->setString(key, *data);
                }
            } else if (Color_RGBA8* data = std::get_if<Color_RGBA8>(&cvarValue)) {
                const char* keyStr = key.c_str();
                conf->setUInt(StringHelper::Sprintf("%s.R", keyStr), data->r);
                conf->setUInt(StringHelper::Sprintf("%s.G", keyStr), data->g);
                conf->setUInt(StringHelper::Sprintf("%s.B", keyStr), data->b);
                conf->setUInt(StringHelper::Sprintf("%s.A", keyStr), data->a);
                conf->setString(StringHelper::Sprintf("%s.Type", keyStr), mercuryRGBAObjectType);
            } else if (Color_RGB8* data = std::get_if<Color_RGB8>(&cvarValue)) {
                const char* keyStr = key.c_str();
                conf->setUInt(StringHelper::Sprintf("%s.R", keyStr), data->r);
                conf->setUInt(StringHelper::Sprintf("%s.G", keyStr), data->g);
                conf->setUInt(StringHelper::Sprintf("%s.B", keyStr), data->b);
                conf->setString(StringHelper::Sprintf("%s.Type", keyStr), mercuryRGBObjectType);
            }
        }
    }

    conf->save();
}

void ConsoleVariable::Load() {
    std::shared_ptr<Mercury> conf = Ship::Window::GetInstance()->GetConfig();
    conf->reload();

    LoadFromPath("", conf->rjson["CVars"].items());
    LoadFromPath("", conf->rjson["CVars2"].items());

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
                    SetColor(itemPath, clr);
                } else if (value.contains("Type") && value["Type"].get<std::string>() == mercuryRGBObjectType) {
                    Color_RGB8 clr;
                    clr.r = value["R"].get<uint8_t>();
                    clr.g = value["G"].get<uint8_t>();
                    clr.b = value["B"].get<uint8_t>();
                    SetColor24(itemPath, clr);
                } else {
                    LoadFromPath(itemPath, value.items());
                }

                break;
            case nlohmann::detail::value_t::string:
                SetString(itemPath, value.get<std::string>().c_str());
                break;
            case nlohmann::detail::value_t::boolean:
                SetInteger(itemPath, value.get<bool>());
                break;
            case nlohmann::detail::value_t::number_unsigned:
            case nlohmann::detail::value_t::number_integer:
                SetInteger(itemPath, value.get<int>());
                break;
            case nlohmann::detail::value_t::number_float:
                SetFloat(itemPath, value.get<float>());
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
