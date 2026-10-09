#include "ship/config/ConsoleVariable.h"

#include <functional>
#include "ship/utils/filesystemtools/DiskFile.h"
#include "ship/utils/Utils.h"
#include "ship/config/Config.h"
#include "ship/Context.h"

namespace Ship {

ConsoleVariable::ConsoleVariable() {
    PublishMap(std::make_unique<VariableMap>());
    Load();
}

ConsoleVariable::~ConsoleVariable() {
    SPDLOG_TRACE("destruct console variables");
}

// Hands `next` to readers and keeps it alive for good. The map it replaces is not freed:
// a reader may still be walking it, and there is no lock that would say when it stopped.
// Ownership moves into mPublishedMaps before the store, so what readers can reach is
// always something the vector already owns. Growing that vector only moves the pointers,
// never the maps, so a pointer a reader is already following stays good.
void ConsoleVariable::PublishMap(std::unique_ptr<VariableMap> next) {
    const VariableMap* published = next.get();
    mPublishedMaps.push_back(std::move(next));
    mPublished.store(published, std::memory_order_release);
}

CVar* ConsoleVariable::FindOwned(const char* name) {
    auto it = mOwned.find(name);
    return it != mOwned.end() ? it->second : nullptr;
}

// Returns the entry for name, publishing a replacement map if the name is new.
CVar* ConsoleVariable::GetOrCreate(const char* name) {
    CVar* owned = FindOwned(name);
    if (owned == nullptr) {
        mStorage.emplace_back();
        owned = &mStorage.back();
        mOwned.emplace(name, owned);
    }

    if (mPendingPublish != nullptr) {
        mPendingPublish->insert_or_assign(std::string(name), owned);
        return owned;
    }

    const VariableMap* current = mPublished.load(std::memory_order_acquire);
    if (current->find(name) != current->end()) {
        return owned;
    }

    auto next = std::make_unique<VariableMap>(*current);
    next->insert_or_assign(std::string(name), owned);
    PublishMap(std::move(next));
    return owned;
}

// Stages an empty key set. Entries themselves stay owned, so names that come back during
// the bulk write reuse their existing CVar and any pointer already handed out stays valid.
void ConsoleVariable::BeginBulkWrite() {
    if (mBulkWriteDepth++ == 0) {
        mPendingPublish = std::make_unique<VariableMap>();
    }
}

void ConsoleVariable::EndBulkWrite() {
    if (--mBulkWriteDepth > 0) {
        return;
    }

    PublishMap(std::move(mPendingPublish));
}

// Returns a stable, permanent copy of value. Assigning the same text twice reuses the
// first copy, so a variable that keeps being set to one of a handful of values does not
// accumulate anything after the first time it sees each of them.
const char* ConsoleVariable::Intern(const char* value) {
    return mStrings.emplace(value).first->c_str();
}

/* Makes a filled-in entry visible to readers. The value is stored first and the type
 * only afterwards, so a reader that sees a type also sees the value that belongs to it
 * rather than whatever the union held before. Readers take no lock, so the fence is what
 * keeps the compiler and the CPU from floating the value store past the type store. */
void ConsoleVariable::Publish(CVar* variable, ConsoleVariableType type) {
    std::atomic_thread_fence(std::memory_order_release);
    variable->Type = type;
}

CVar* ConsoleVariable::Get(const char* name) {
    const VariableMap& published = Published();
    auto it = published.find(name);
    if (it == published.end()) {
        return nullptr;
    }
    /* An entry typed None is either not filled in yet or has been cleared. Either way it
     * holds no value, so report it the way a name that was never set is reported. */
    CVar* variable = it->second;
    return variable->Type != ConsoleVariableType::None ? variable : nullptr;
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
        return variable->Float;
    }

    return defaultValue;
}

const char* ConsoleVariable::GetString(const char* name, const char* defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::String) {
        return variable->String;
    }

    return defaultValue;
}

Color_RGBA8 ConsoleVariable::GetColor(const char* name, Color_RGBA8 defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::Color) {
        return variable->Color;
    } else if (variable != nullptr && variable->Type == ConsoleVariableType::Color24) {
        Color_RGBA8 temp;
        temp.r = variable->Color24.r;
        temp.g = variable->Color24.g;
        temp.b = variable->Color24.b;
        temp.a = 255;
        return temp;
    }

    return defaultValue;
}

Color_RGB8 ConsoleVariable::GetColor24(const char* name, Color_RGB8 defaultValue) {
    auto variable = Get(name);

    if (variable != nullptr && variable->Type == ConsoleVariableType::Color24) {
        return variable->Color24;
    } else if (variable != nullptr && variable->Type == ConsoleVariableType::Color) {
        Color_RGB8 temp;
        temp.r = variable->Color.r;
        temp.g = variable->Color.g;
        temp.b = variable->Color.b;
        return temp;
    }

    return defaultValue;
}

void ConsoleVariable::SetInteger(const char* name, int32_t value) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    CVar* variable = GetOrCreate(name);

    variable->Integer = value;
    Publish(variable, ConsoleVariableType::Integer);
}

void ConsoleVariable::SetFloat(const char* name, float value) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    CVar* variable = GetOrCreate(name);

    variable->Float = value;
    Publish(variable, ConsoleVariableType::Float);
}

void ConsoleVariable::SetString(const char* name, const char* value) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    CVar* variable = GetOrCreate(name);

    variable->String = Intern(value);
    Publish(variable, ConsoleVariableType::String);
}

void ConsoleVariable::SetColor(const char* name, Color_RGBA8 value) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    CVar* variable = GetOrCreate(name);

    variable->Color = value;
    Publish(variable, ConsoleVariableType::Color);
}

void ConsoleVariable::SetColor24(const char* name, Color_RGB8 value) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    CVar* variable = GetOrCreate(name);

    variable->Color24 = value;
    Publish(variable, ConsoleVariableType::Color24);
}

void ConsoleVariable::RegisterInteger(const char* name, int32_t defaultValue) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    if (Get(name) == nullptr) {
        SetInteger(name, defaultValue);
    }
}

void ConsoleVariable::RegisterFloat(const char* name, float defaultValue) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    if (Get(name) == nullptr) {
        SetFloat(name, defaultValue);
    }
}

void ConsoleVariable::RegisterString(const char* name, const char* defaultValue) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    if (Get(name) == nullptr) {
        SetString(name, defaultValue);
    }
}

void ConsoleVariable::RegisterColor(const char* name, Color_RGBA8 defaultValue) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    if (Get(name) == nullptr) {
        SetColor(name, defaultValue);
    }
}

void ConsoleVariable::RegisterColor24(const char* name, Color_RGB8 defaultValue) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    if (Get(name) == nullptr) {
        SetColor24(name, defaultValue);
    }
}

// Retypes an entry to None so lookups stop finding it, leaving it in storage and in the
// published map. That keeps clearing off the publishing path: resetting a screen full of
// cosmetics costs a few hundred field writes instead of a few hundred copies of the whole
// map, none of which could ever be freed.
void ConsoleVariable::Clear(CVar* variable) {
    variable->Type = ConsoleVariableType::None;
}

void ConsoleVariable::ClearVariable(const char* name) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    std::shared_ptr<Config> conf = Context::GetRawInstance()->GetConfig();
    CVar* var = FindOwned(name);
    if (var != nullptr) {
        bool color = var->Type == ConsoleVariableType::Color || var->Type == ConsoleVariableType::Color24;
        if (color) {
            std::string a = StringHelper::Sprintf("%s.%s", name, "A");
            std::string b = StringHelper::Sprintf("%s.%s", name, "B");
            std::string g = StringHelper::Sprintf("%s.%s", name, "G");
            std::string r = StringHelper::Sprintf("%s.%s", name, "R");
            std::string t = StringHelper::Sprintf("%s.%s", name, "Type");
            for (const std::string& component : { a, b, g, r, t }) {
                CVar* owned = FindOwned(component.c_str());
                if (owned != nullptr) {
                    Clear(owned);
                }
            }
            conf->Erase(std::string("CVars.") + a);
            conf->Erase(std::string("CVars.") + b);
            conf->Erase(std::string("CVars.") + g);
            conf->Erase(std::string("CVars.") + r);
            conf->Erase(std::string("CVars.") + t);
        }
        Clear(var);
    }
    conf->Erase(StringHelper::Sprintf("CVars.%s", name));
}

void ConsoleVariable::ClearBlock(const char* name) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    std::shared_ptr<Config> conf = Context::GetRawInstance()->GetConfig();
    conf->EraseBlock(StringHelper::Sprintf("CVars.%s", name));
    Load();
}

void ConsoleVariable::CopyVariable(const char* from, const char* to) {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    CVar* variableFrom = Get(from);
    if (variableFrom == nullptr) {
        return;
    }
    CVar* variableTo = GetOrCreate(to);

    const ConsoleVariableType type = variableFrom->Type;
    switch (type) {
        case ConsoleVariableType::Integer:
            variableTo->Integer = variableFrom->Integer;
            break;
        case ConsoleVariableType::Float:
            variableTo->Float = variableFrom->Float;
            break;
        case ConsoleVariableType::String:
            // Interned, so both names can point at the one copy.
            variableTo->String = variableFrom->String;
            break;
        case ConsoleVariableType::Color:
            variableTo->Color = variableFrom->Color;
            break;
        case ConsoleVariableType::Color24:
            variableTo->Color24 = variableFrom->Color24;
            break;
        case ConsoleVariableType::None:
            return;
    }
    Publish(variableTo, type);
}

void ConsoleVariable::Save() {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    std::shared_ptr<Config> conf = Context::GetRawInstance()->GetConfig();

    for (const auto& variable : Published()) {
        const std::string key = StringHelper::Sprintf("CVars.%s", variable.first.c_str());

        if (variable.second->Type == ConsoleVariableType::String && variable.second != nullptr) {
            conf->SetString(key, variable.second->String);
        } else if (variable.second->Type == ConsoleVariableType::Integer) {
            conf->SetInt(key, variable.second->Integer);
        } else if (variable.second->Type == ConsoleVariableType::Float) {
            conf->SetFloat(key, variable.second->Float);
        } else if (variable.second->Type == ConsoleVariableType::Color ||
                   variable.second->Type == ConsoleVariableType::Color24) {
            auto keyStr = key.c_str();
            conf->SetUInt(StringHelper::Sprintf("%s.R", keyStr), variable.second->Type == ConsoleVariableType::Color
                                                                     ? variable.second->Color.r
                                                                     : variable.second->Color24.r);
            conf->SetUInt(StringHelper::Sprintf("%s.G", keyStr), variable.second->Type == ConsoleVariableType::Color
                                                                     ? variable.second->Color.g
                                                                     : variable.second->Color24.g);
            conf->SetUInt(StringHelper::Sprintf("%s.B", keyStr), variable.second->Type == ConsoleVariableType::Color
                                                                     ? variable.second->Color.b
                                                                     : variable.second->Color24.b);
            if (variable.second->Type == ConsoleVariableType::Color) {
                conf->SetUInt(StringHelper::Sprintf("%s.A", keyStr), variable.second->Color.a);
                conf->SetString(StringHelper::Sprintf("%s.Type", keyStr), "RGBA");
            } else {
                conf->SetString(StringHelper::Sprintf("%s.Type", keyStr), "RGB");
            }
        }
    }

    conf->Save();
}

void ConsoleVariable::Load() {
    std::lock_guard<std::recursive_mutex> lock(mWriteMutex);
    std::shared_ptr<Config> conf = Context::GetRawInstance()->GetConfig();
    conf->Reload();

    BeginBulkWrite();
    LoadFromPath("", conf->GetNestedJson()["CVars"].items());
    LoadLegacy();
    EndBulkWrite();
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
                if (value.contains("Type") && value["Type"].get<std::string>() == "RGBA") {
                    Color_RGBA8 clr;
                    clr.r = value["R"].get<uint8_t>();
                    clr.g = value["G"].get<uint8_t>();
                    clr.b = value["B"].get<uint8_t>();
                    clr.a = value["A"].get<uint8_t>();
                    SetColor(itemPath.c_str(), clr);
                } else if (value.contains("Type") && value["Type"].get<std::string>() == "RGB") {
                    Color_RGB8 clr;
                    clr.r = value["R"].get<uint8_t>();
                    clr.g = value["G"].get<uint8_t>();
                    clr.b = value["B"].get<uint8_t>();
                    SetColor24(itemPath.c_str(), clr);
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
    auto conf = Context::GetPathRelativeToAppDirectory("cvars.cfg");
    if (DiskFile::Exists(conf)) {
        const auto lines = DiskFile::ReadAllLines(conf);

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
                SetString(cfg[0].c_str(), value.c_str());
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
