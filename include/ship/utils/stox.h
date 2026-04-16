#pragma once
#include <string>
#include <cstdint>

namespace Ship {

/**
 * @brief Converts a string to a boolean value.
 *
 * Recognises "1" and "true" (case-sensitive) as true; everything else returns @p defaultVal.
 *
 * @param s          String to convert.
 * @param defaultVal Value returned when @p s is not a recognised boolean token.
 * @return Parsed boolean, or @p defaultVal on failure.
 */
bool stob(const std::string& s, bool defaultVal = false);

/**
 * @brief Converts a string to a 32-bit signed integer.
 * @param s          String to convert.
 * @param defaultVal Value returned when conversion fails.
 * @return Parsed integer, or @p defaultVal on failure.
 */
int32_t stoi(const std::string& s, int32_t defaultVal = 0);

/**
 * @brief Converts a string to a float.
 * @param s          String to convert.
 * @param defaultVal Value returned when conversion fails.
 * @return Parsed float, or @p defaultVal on failure.
 */
float stof(const std::string& s, float defaultVal = 1.0f);

/**
 * @brief Converts a string to a 64-bit signed integer.
 * @param s          String to convert.
 * @param defaultVal Value returned when conversion fails.
 * @return Parsed 64-bit integer, or @p defaultVal on failure.
 */
int64_t stoll(const std::string& s, int64_t defaultVal = 0);

} // namespace Ship