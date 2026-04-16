#pragma once

#include "stdint.h"
#include "libultraship/color.h"
#include "ship/Api.h"

#ifdef __cplusplus
#include <memory>
#include "ship/config/ConsoleVariable.h"

/**
 * @brief Returns a shared pointer to the CVar with the given name, or nullptr if it does not exist.
 * @param name CVar name (e.g. "gMyOption").
 */
std::shared_ptr<Ship::CVar> CVarGet(const char* name);

extern "C" {
#endif

/**
 * @brief Returns the integer value of the named CVar, or @p defaultValue if it is not set.
 * @param name         CVar name.
 * @param defaultValue Value returned when the CVar does not exist.
 */
API_EXPORT int32_t CVarGetInteger(const char* name, int32_t defaultValue);

/**
 * @brief Returns the float value of the named CVar, or @p defaultValue if it is not set.
 * @param name         CVar name.
 * @param defaultValue Value returned when the CVar does not exist.
 */
API_EXPORT float CVarGetFloat(const char* name, float defaultValue);

/**
 * @brief Returns the string value of the named CVar, or @p defaultValue if it is not set.
 * @param name         CVar name.
 * @param defaultValue Value returned when the CVar does not exist (must outlive the call).
 */
API_EXPORT const char* CVarGetString(const char* name, const char* defaultValue);

/**
 * @brief Returns the RGBA colour value of the named CVar, or @p defaultValue if it is not set.
 * @param name         CVar name.
 * @param defaultValue Default RGBA colour.
 */
API_EXPORT Color_RGBA8 CVarGetColor(const char* name, Color_RGBA8 defaultValue);

/**
 * @brief Returns the RGB colour value of the named CVar, or @p defaultValue if it is not set.
 * @param name         CVar name.
 * @param defaultValue Default RGB colour.
 */
API_EXPORT Color_RGB8 CVarGetColor24(const char* name, Color_RGB8 defaultValue);

/**
 * @brief Sets the named CVar to an integer value (creates it if it does not exist).
 * @param name  CVar name.
 * @param value New integer value.
 */
API_EXPORT void CVarSetInteger(const char* name, int32_t value);

/**
 * @brief Sets the named CVar to a float value (creates it if it does not exist).
 * @param name  CVar name.
 * @param value New float value.
 */
API_EXPORT void CVarSetFloat(const char* name, float value);

/**
 * @brief Sets the named CVar to a string value (creates it if it does not exist).
 * @param name  CVar name.
 * @param value New string value (copied internally).
 */
API_EXPORT void CVarSetString(const char* name, const char* value);

/**
 * @brief Sets the named CVar to an RGBA colour value (creates it if it does not exist).
 * @param name  CVar name.
 * @param value New RGBA colour.
 */
API_EXPORT void CVarSetColor(const char* name, Color_RGBA8 value);

/**
 * @brief Sets the named CVar to an RGB colour value (creates it if it does not exist).
 * @param name  CVar name.
 * @param value New RGB colour.
 */
API_EXPORT void CVarSetColor24(const char* name, Color_RGB8 value);

/**
 * @brief Registers an integer CVar with a default value if it has not been set before.
 *
 * Unlike CVarSetInteger(), this is a no-op when the CVar already has a value.
 *
 * @param name         CVar name.
 * @param defaultValue Default value to use when the CVar is absent.
 */
API_EXPORT void CVarRegisterInteger(const char* name, int32_t defaultValue);

/**
 * @brief Registers a float CVar with a default value if it has not been set before.
 * @param name         CVar name.
 * @param defaultValue Default float value.
 */
API_EXPORT void CVarRegisterFloat(const char* name, float defaultValue);

/**
 * @brief Registers a string CVar with a default value if it has not been set before.
 * @param name         CVar name.
 * @param defaultValue Default string value.
 */
API_EXPORT void CVarRegisterString(const char* name, const char* defaultValue);

/**
 * @brief Registers an RGBA colour CVar with a default value if it has not been set before.
 * @param name         CVar name.
 * @param defaultValue Default RGBA colour.
 */
API_EXPORT void CVarRegisterColor(const char* name, Color_RGBA8 defaultValue);

/**
 * @brief Registers an RGB colour CVar with a default value if it has not been set before.
 * @param name         CVar name.
 * @param defaultValue Default RGB colour.
 */
API_EXPORT void CVarRegisterColor24(const char* name, Color_RGB8 defaultValue);

/**
 * @brief Removes the named CVar from the runtime store (does not affect persisted config).
 * @param name CVar name to remove.
 */
API_EXPORT void CVarClear(const char* name);

/**
 * @brief Returns true if the named CVar currently has a value in the runtime store.
 * @param name CVar name.
 */
API_EXPORT bool CVarExists(const char* name);

/**
 * @brief Removes all CVars whose names begin with the given prefix.
 * @param name Prefix string (e.g. "gMod.").
 */
API_EXPORT void CVarClearBlock(const char* name);

/**
 * @brief Copies the value of the CVar named @p from to a new CVar named @p to.
 * @param from Source CVar name.
 * @param to   Destination CVar name (created if it does not exist).
 */
API_EXPORT void CVarCopy(const char* from, const char* to);

/**
 * @brief Loads all CVar values from the persisted config file into the runtime store.
 */
API_EXPORT void CVarLoad();

/**
 * @brief Saves all runtime CVar values to the persisted config file.
 */
API_EXPORT void CVarSave();

#ifdef __cplusplus
};
#endif
