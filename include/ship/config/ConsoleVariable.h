#pragma once

#include "ship/utils/color.h"
#include "ship/Component.h"
#include <nlohmann/json.hpp>
#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>

namespace Ship {

/** @brief Discriminator tag for the active field of the CVar union. */
typedef enum class ConsoleVariableType { Integer, Float, String, Color, Color24 } ConsoleVariableType;

/**
 * @brief A single console variable (CVar) holding a typed value.
 *
 * CVars are the primary runtime-tunable settings system. Each CVar stores exactly
 * one value whose type is determined by the Type field. The union members Integer,
 * Float, String, Color, and Color24 are mutually exclusive.
 *
 * @note The String member is heap-allocated and is freed by the destructor.
 */
typedef struct CVar {
    /** @brief Discriminator indicating which union field is active. */
    ConsoleVariableType Type;
    union {
        int32_t Integer;        ///< Active when Type == ConsoleVariableType::Integer.
        float Float;            ///< Active when Type == ConsoleVariableType::Float.
        char* String = nullptr; ///< Active when Type == ConsoleVariableType::String (heap-allocated).
        Color_RGBA8 Color;      ///< Active when Type == ConsoleVariableType::Color.
        Color_RGB8 Color24;     ///< Active when Type == ConsoleVariableType::Color24.
    };
    ~CVar() {
        if (Type == ConsoleVariableType::String && String != nullptr) {
            free(String);
        }
    }
} CVar;

/**
 * @brief Manages a named collection of console variables (CVars).
 *
 * ConsoleVariable persists CVar values across sessions by serialising them to and
 * from a JSON file via the Config layer. Values can be registered with defaults,
 * queried, mutated, copied, and cleared at runtime.
 *
 * **Required Context children (looked up at runtime):**
 * - **Config** — used by Load() and Save() to read and write CVar persistence.
 *   Config must be added to the Context before ConsoleVariable::Load() or
 *   ConsoleVariable::Save() are called.
 *
 * Obtain the instance from `Context::GetChildren().GetFirst<ConsoleVariable>()`.
 */
class ConsoleVariable : public Component {
  public:
    ConsoleVariable();
    ~ConsoleVariable();

    /**
     * @brief Returns the raw CVar entry for the given name, or nullptr if not found.
     * @param name CVar name (case-sensitive).
     */
    std::shared_ptr<CVar> Get(const char* name);

    /**
     * @brief Returns the integer value of a CVar, or the default if not found or wrong type.
     * @param name         CVar name.
     * @param defaultValue Value to return when the CVar is absent.
     */
    int32_t GetInteger(const char* name, int32_t defaultValue);

    /**
     * @brief Returns the float value of a CVar, or the default if not found or wrong type.
     * @param name         CVar name.
     * @param defaultValue Value to return when the CVar is absent.
     */
    float GetFloat(const char* name, float defaultValue);

    /**
     * @brief Returns the string value of a CVar, or the default if not found or wrong type.
     * @param name         CVar name.
     * @param defaultValue Value to return when the CVar is absent.
     * @return Pointer to internal storage; do not free or store long-term.
     */
    const char* GetString(const char* name, const char* defaultValue);

    /**
     * @brief Returns the RGBA colour value of a CVar, or the default if not found or wrong type.
     * @param name         CVar name.
     * @param defaultValue Fallback colour.
     */
    Color_RGBA8 GetColor(const char* name, Color_RGBA8 defaultValue);

    /**
     * @brief Returns the RGB colour value of a CVar, or the default if not found or wrong type.
     * @param name         CVar name.
     * @param defaultValue Fallback colour.
     */
    Color_RGB8 GetColor24(const char* name, Color_RGB8 defaultValue);

    /**
     * @brief Sets (or creates) an integer CVar.
     * @param name  CVar name.
     * @param value New integer value.
     */
    void SetInteger(const char* name, int32_t value);

    /**
     * @brief Sets (or creates) a float CVar.
     * @param name  CVar name.
     * @param value New float value.
     */
    void SetFloat(const char* name, float value);

    /**
     * @brief Sets (or creates) a string CVar.
     * @param name  CVar name.
     * @param value New string value (copied internally).
     */
    void SetString(const char* name, const char* value);

    /**
     * @brief Sets (or creates) an RGBA colour CVar.
     * @param name  CVar name.
     * @param value New RGBA colour.
     */
    void SetColor(const char* name, Color_RGBA8 value);

    /**
     * @brief Sets (or creates) an RGB colour CVar.
     * @param name  CVar name.
     * @param value New RGB colour.
     */
    void SetColor24(const char* name, Color_RGB8 value);

    /**
     * @brief Registers an integer CVar with a default value if it does not already exist.
     * @param name         CVar name.
     * @param defaultValue Default value to set when first registered.
     */
    void RegisterInteger(const char* name, int32_t defaultValue);

    /**
     * @brief Registers a float CVar with a default value if it does not already exist.
     * @param name         CVar name.
     * @param defaultValue Default value.
     */
    void RegisterFloat(const char* name, float defaultValue);

    /**
     * @brief Registers a string CVar with a default value if it does not already exist.
     * @param name         CVar name.
     * @param defaultValue Default string (copied internally).
     */
    void RegisterString(const char* name, const char* defaultValue);

    /**
     * @brief Registers an RGBA colour CVar with a default value if it does not already exist.
     * @param name         CVar name.
     * @param defaultValue Default RGBA colour.
     */
    void RegisterColor(const char* name, Color_RGBA8 defaultValue);

    /**
     * @brief Registers an RGB colour CVar with a default value if it does not already exist.
     * @param name         CVar name.
     * @param defaultValue Default RGB colour.
     */
    void RegisterColor24(const char* name, Color_RGB8 defaultValue);

    /**
     * @brief Removes a single CVar entry from the map.
     * @param name CVar name to remove.
     */
    void ClearVariable(const char* name);

    /**
     * @brief Removes all CVars whose names start with the given prefix.
     * @param name Prefix string to match.
     */
    void ClearBlock(const char* name);

    /**
     * @brief Copies the value of one CVar to another (creating the destination if needed).
     * @param from Source CVar name.
     * @param to   Destination CVar name.
     */
    void CopyVariable(const char* from, const char* to);

    /** @brief Serializes all CVars to the backing JSON config file. */
    void Save();

    /** @brief Loads CVars from the backing JSON config file, overwriting in-memory values. */
    void Load();

  protected:
    void LoadFromPath(std::string path,
                      nlohmann::detail::iteration_proxy<nlohmann::detail::iter_impl<nlohmann::json>> items);
    void LoadLegacy();

  private:
    struct TransparentStringHash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const noexcept {
            return std::hash<std::string_view>{}(sv);
        }
    };
    struct TransparentStringEqual {
        using is_transparent = void;
        bool operator()(std::string_view a, std::string_view b) const noexcept {
            return a == b;
        }
    };
    std::unordered_map<std::string, std::shared_ptr<CVar>, TransparentStringHash, TransparentStringEqual> mVariables;
};
} // namespace Ship
