#pragma once
#include <string>
#include <cstdint>

namespace Ship {

/**
 * @brief Converts a string to a boolean value.
 *
 * First attempts a numeric integer parse: "0" → false, any other integer → true.
 * If that fails, retries with @c std::boolalpha, which accepts "false" → false
 * and "true" → true (locale-dependent, but typically case-sensitive for the "C" locale).
 * If both attempts fail, returns @p defaultVal and logs a debug message.
 *
 * @param s          String to convert.
 * @param defaultVal Value returned when @p s cannot be parsed as a boolean.
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