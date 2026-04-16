#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace Ship {

/**
 * @brief Lightweight math utility functions used throughout the engine.
 */
namespace Math {

/**
 * @brief Clamps @p d to the inclusive range [@p min, @p max].
 * @param d   Value to clamp.
 * @param min Lower bound.
 * @param max Upper bound.
 * @return @p min if @p d < @p min, @p max if @p d > @p max, otherwise @p d.
 */
float clamp(float d, float min, float max);

/**
 * @brief Combines two size_t hash values into a single hash.
 *
 * Implements the standard "hash combine" technique from Boost: the result is
 * sensitive to the order of arguments, so HashCombine(a, b) != HashCombine(b, a).
 *
 * @param lhs First hash value.
 * @param rhs Second hash value.
 * @return Combined hash.
 */
size_t HashCombine(size_t lhs, size_t rhs);

/**
 * @brief Returns true if @p s represents a valid number of type @p Numeric.
 *
 * Attempts to parse @p s using the standard stream extraction operator. Returns
 * true only if parsing succeeds and consumes the entire string.
 *
 * @tparam Numeric Numeric type to test against (e.g. int, float, double).
 * @param  s       String to test.
 */
template <typename Numeric> bool IsNumber(const std::string& s) {
    Numeric n;
    return ((std::istringstream(s) >> n >> std::ws).eof());
}
} // namespace Math

/**
 * @brief Splits @p text at each @p separator character, optionally preserving quoted spans.
 * @param text       Input string to split.
 * @param separator  Delimiter character.
 * @param keepQuotes If true, quoted spans (delimited by @c ") are not split even if they
 *                   contain @p separator.
 * @return Vector of token strings (separators are not included).
 */
std::vector<std::string> splitText(const std::string& text, char separator, bool keepQuotes);

/**
 * @brief Returns a copy of @p in with all ASCII letters converted to lower case.
 * @param in Input string.
 * @return Lower-cased copy.
 */
std::string toLowerCase(std::string in);
} // namespace Ship
