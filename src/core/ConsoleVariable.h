#pragma once

#include "libultraship/color.h"
#include <nlohmann/json.hpp>
#include <stdint.h>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace Ship {
typedef enum class ConsoleVariableType { Integer, Float, String, Color, Color24 } ConsoleVariableType;
typedef union CVarValue {

} CVarValue;

class CVarInterface {
  public:
    using VauleType = std::variant<int32_t, float, std::string, Color_RGBA8, Color_RGB8>;

    CVarInterface() = default;
    virtual ~CVarInterface() = default;

    virtual VauleType Get() const = 0;

    virtual void Set(const VauleType&) = 0;

    virtual bool ShouldSave() = 0;
};

template<class StorageType>
class CVarDefaulted : public CVarInterface {

    using DefualtType = std::remove_reference<StorageType>::type;

  public:
    CVarDefaulted(StorageType& initialValue, const DefualtType& defaultValue) :
        mValue(initialValue),
        mDefault(defaultValue)
    {
    };

    virtual ~CVarDefaulted() = default;

    VauleType Get() const override {
        return VauleType{ mValue };
    }

    void Set(const VauleType& newValue) override {
        // TODO assert type
        if (const DefualtType* value = std::get_if<DefualtType>(&newValue)) {
            mValue = *value;
        }
    }

    bool ShouldSave() override {
        return mValue != mDefault;
    }

  private:
    StorageType mValue;
    const DefualtType mDefault;
};

// Represents a CVar with embedded storage i.e. the lifetime of the value matches the lifetime of the CVar
template<typename T>
using CVarEmbedded = CVarDefaulted<T>;

// Represents a CVar with embedded storage i.e. the lifetime of the value is independent of that of the CVar's
template<typename T>
using CVarManaged = CVarDefaulted<T&>;

typedef struct CVar {
    const char* Name;
    ConsoleVariableType Type;
    int32_t Integer;
    float Float;
    std::string String;
    Color_RGBA8 Color;
    Color_RGB8 Color24;
} CVar;

class ConsoleVariable {
  public:
    ConsoleVariable();

    std::shared_ptr<CVar> Get(const std::string& name);

    int32_t GetInteger(const std::string& name, int32_t defaultValue);
    float GetFloat(const std::string& name, float defaultValue);
    const char* GetString(const std::string& name, const char* defaultValue);
    Color_RGBA8 GetColor(const std::string& name, Color_RGBA8 defaultValue);
    Color_RGB8 GetColor24(const std::string& name, Color_RGB8 defaultValue);

    void SetInteger(const std::string& name, int32_t value);
    void SetFloat(const std::string& name, float value);
    void SetString(const std::string& name, const char* value);
    void SetColor(const std::string& name, Color_RGBA8 value);
    void SetColor24(const std::string& name, Color_RGB8 value);

    void RegisterInteger(const std::string& name, int32_t defaultValue);
    void RegisterFloat(const std::string& name, float defaultValue);
    void RegisterString(const std::string& name, const char* defaultValue);
    void RegisterColor(const std::string& name, Color_RGBA8 defaultValue);
    void RegisterColor24(const std::string& name, Color_RGB8 defaultValue);

    void ClearVariable(const std::string& name);

    template<typename T>
    T Get(const std::string& name) {
        auto cvarIter = mVariables_New.find(name);
        if (cvarIter == mVariables_New.end()) {
            // TODO assert? CVar was not registered/created
            return T{};
        }

        auto cvarValue = cvarIter->second->Get();
        if (const T* ret = std::get_if<T>(&cvarValue)) {
            return *ret;
        }

        // TODO assert? Incorrect type is probably a mistake

        return T{};
    }
    
    template<typename T>
    void Set(const std::string& name, const T& value) {
        auto cvarIter = mVariables_New.find(name);
        if (cvarIter == mVariables_New.end()) {
            // TODO assert? CVar was not registered/created
            return;
        }

        cvarIter->second->Set(value);
    }

    // Creates a CVar that manages the value
    template <typename T>
    void RegisterEmbedded(const std::string& name, T value) {
        if (mVariables_New.find(name) != mVariables_New.end()) {
            return;
        }

        mVariables_New[name] = std::make_unique<CVarEmbedded<T>>(value, value);
    }

    // Creates a CVar that tracks the value
    template <typename T>
    void RegisterManaged(const std::string& name, T& value) {
        if (mVariables_New.find(name) != mVariables_New.end()) {
            return;
        }

        mVariables_New[name] = std::make_unique<CVarManaged<T>>(value, value);
    }

    void Save();
    void Load();

  private:
    void LoadFromPath(std::string path,
                      nlohmann::detail::iteration_proxy<nlohmann::detail::iter_impl<nlohmann::json>> items);
    void LoadLegacy();

    std::unordered_map<std::string, std::shared_ptr<CVar>> mVariables;
    std::unordered_map<std::string, std::unique_ptr<CVarInterface>> mVariables_New;
};
} // namespace Ship
