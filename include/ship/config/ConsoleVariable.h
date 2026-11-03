#pragma once

#include "ship/utils/color.h"
#include <nlohmann/json.hpp>
#include <stdint.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <atomic>
#include <deque>
#include <vector>

namespace Ship {

/**
 * @brief Discriminator tag for the active field of the CVar union.
 *
 * None means the entry carries no value: it is either freshly created and not yet
 * filled in, or it has been cleared. Lookups treat it as if the name did not exist.
 */
typedef enum class ConsoleVariableType { None, Integer, Float, String, Color, Color24 } ConsoleVariableType;

/**
 * @brief A single console variable (CVar) holding a typed value.
 *
 * CVars are the primary runtime-tunable settings system. Each CVar stores exactly
 * one value whose type is determined by the Type field. The union members Integer,
 * Float, String, Color, and Color24 are mutually exclusive.
 *
 * @note The String member points into the owning ConsoleVariable's string storage. It is
 *       not owned by the CVar and must not be freed; it stays valid for as long as that
 *       ConsoleVariable exists.
 */
typedef struct CVar {
    /** @brief Discriminator indicating which union field is active. */
    ConsoleVariableType Type = ConsoleVariableType::None;
    union {
        int32_t Integer;              ///< Active when Type == ConsoleVariableType::Integer.
        float Float;                  ///< Active when Type == ConsoleVariableType::Float.
        const char* String = nullptr; ///< Active when Type == ConsoleVariableType::String.
        Color_RGBA8 Color;            ///< Active when Type == ConsoleVariableType::Color.
        Color_RGB8 Color24;           ///< Active when Type == ConsoleVariableType::Color24.
    };
} CVar;

/**
 * @brief Manages a named collection of console variables (CVars).
 *
 * ConsoleVariable persists CVar values across sessions by serialising them to and
 * from a JSON file via the Config layer. Values can be registered with defaults,
 * queried, mutated, copied, and cleared at runtime.
 *
 * Obtain the singleton instance from Context::GetConsoleVariables().
 */
class ConsoleVariable {
  public:
    ConsoleVariable();
    ~ConsoleVariable();

    /**
     * @brief Returns the raw CVar entry for the given name, or nullptr if not found.
     * @param name CVar name (case-sensitive).
     *
     * A cleared name reports as not found, matching the behaviour callers expect from a
     * name that was never set.
     *
     * The returned pointer stays valid for as long as this ConsoleVariable exists: entries
     * are owned here and are never individually destroyed, so callers may hold it. Reads are
     * lock free, so this is safe to call from the audio thread while another thread writes.
     */
    CVar* Get(const char* name);

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
     * @return Pointer to interned storage owned by this ConsoleVariable. It must not be
     *         freed, and it stays valid until this ConsoleVariable is destroyed, so callers
     *         may hold it. A later SetString on the same name leaves it untouched and
     *         publishes a separate string rather than overwriting this one.
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
    using VariableMap = std::unordered_map<std::string, CVar*, TransparentStringHash, TransparentStringEqual>;

    /* Reads happen on the audio thread thousands of times a second and take no lock:
     * they load the published map and follow a pointer into storage. That works
     * because nothing a reader can be holding is ever destroyed while this object is
     * alive, and because that map is replaced rather than edited in place. The three
     * groups below are what that costs. */

    /* The name -> entry map readers are currently allowed to see. Writers never edit
     * it; they publish a replacement. */
    std::atomic<const VariableMap*> mPublished;

    /* Kept until the destructor, because a reader may still be inside any of it.
     * deque keeps CVar addresses stable as it grows, unordered_set keeps interned
     * string addresses stable across rehashes, and mPublishedMaps owns every map ever
     * published, the live one included, so a superseded map is simply left alone rather
     * than deleted. Clearing a variable retypes it to None in place; interning is what
     * keeps never freeing a string affordable. */
    std::deque<CVar> mStorage;
    std::unordered_set<std::string> mStrings;
    std::vector<std::unique_ptr<const VariableMap>> mPublishedMaps;

    /* Writer side, all of it under mWriteMutex, which readers never take. Recursive
     * because the write paths nest: Load() -> SetInteger(), RegisterInteger() ->
     * SetInteger(). Only a name never seen before rebuilds the map; mPendingPublish
     * stages that rebuild so Load(), which touches every key, publishes once at the end
     * rather than once per variable. */
    std::recursive_mutex mWriteMutex;
    VariableMap mOwned;
    std::unique_ptr<VariableMap> mPendingPublish;
    int mBulkWriteDepth = 0;

    /* The map readers should search, as a reference rather than a pointer. */
    const VariableMap& Published() const {
        return *mPublished.load(std::memory_order_acquire);
    }

    /* All of these require mWriteMutex to be held. */
    void PublishMap(std::unique_ptr<VariableMap> next);
    CVar* FindOwned(const char* name);
    CVar* GetOrCreate(const char* name);
    const char* Intern(const char* value);
    static void Publish(CVar* variable, ConsoleVariableType type);
    static void Clear(CVar* variable);
    void BeginBulkWrite();
    void EndBulkWrite();
};
} // namespace Ship
