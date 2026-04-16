#pragma once

#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

/**
 * @brief Collection of static string utility methods.
 *
 * StringHelper provides common string operations that are not part of the C++
 * standard library: tokenisation, whitespace stripping, substring replacement,
 * case-insensitive comparison, hex encoding/decoding, and more.
 *
 * All methods are static — StringHelper is not intended to be instantiated.
 */
class StringHelper {
  public:
    /**
     * @brief Splits @p s at each occurrence of @p delimiter and returns the pieces.
     * @param s         Input string to split.
     * @param delimiter Separator string.
     * @return Vector of substrings (the delimiter is not included in any piece).
     */
    static std::vector<std::string> Split(std::string s, const std::string& delimiter);

    /**
     * @brief Splits a string_view at each occurrence of @p delimiter.
     * @param s         Input view to split.
     * @param delimiter Separator string.
     * @return Vector of string_view pieces referencing the original data.
     */
    static std::vector<std::string_view> Split(std::string_view s, const std::string& delimiter);

    /**
     * @brief Removes all occurrences of @p delimiter from @p s.
     * @param s         String to strip.
     * @param delimiter Characters/substring to remove.
     * @return Stripped copy of @p s.
     */
    static std::string Strip(std::string s, const std::string& delimiter);

    /**
     * @brief Replaces the first occurrence of @p from with @p to in @p str.
     * @param str  Input string.
     * @param from Substring to find.
     * @param to   Replacement string.
     * @return Modified copy of @p str.
     */
    static std::string Replace(std::string str, const std::string& from, const std::string& to);

    /**
     * @brief Replaces the first occurrence of @p from with @p to in @p str in-place.
     * @param str  String to modify.
     * @param from Substring to find.
     * @param to   Replacement string.
     */
    static void ReplaceOriginal(std::string& str, const std::string& from, const std::string& to);

    /**
     * @brief Returns true if @p s starts with @p input.
     * @param s     String to test.
     * @param input Prefix to look for.
     */
    static bool StartsWith(const std::string& s, const std::string& input);

    /**
     * @brief Returns true if @p s contains @p input as a substring.
     * @param s     String to search.
     * @param input Substring to find.
     */
    static bool Contains(const std::string& s, const std::string& input);

    /**
     * @brief Returns true if @p s ends with @p input.
     * @param s     String to test.
     * @param input Suffix to look for.
     */
    static bool EndsWith(const std::string& s, const std::string& input);

    /**
     * @brief Formats a string using printf-style arguments and returns it.
     * @param format printf-style format string.
     * @param ...    Format arguments.
     * @return Formatted std::string.
     */
    static std::string Sprintf(const char* format, ...);

    /**
     * @brief Joins a vector of strings into a single string, separated by @p separator.
     * @param elements  Strings to join.
     * @param separator Separator inserted between consecutive elements.
     * @return Joined string.
     */
    static std::string Implode(std::vector<std::string>& elements, const char* const separator);

    /**
     * @brief Converts a string to a 64-bit signed integer.
     * @param str  Input string (may have a 0x prefix for base 16).
     * @param base Numeric base (default 10).
     * @return Parsed integer value.
     */
    static int64_t StrToL(const std::string& str, int32_t base = 10);

    /**
     * @brief Returns "true" or "false" as a std::string.
     * @param b Boolean value.
     */
    static std::string BoolStr(bool b);

    /**
     * @brief Returns true if @p str consists entirely of decimal digit characters.
     * @param str String to check.
     */
    static bool HasOnlyDigits(const std::string& str);

    /**
     * @brief Returns true if @p str is a valid hexadecimal string (optional "0x" prefix).
     * @param str String view to validate.
     */
    static bool IsValidHex(std::string_view str);

    /**
     * @brief Returns true if @p str is a valid hexadecimal string (optional "0x" prefix).
     * @param str String to validate.
     */
    static bool IsValidHex(const std::string& str);

    /**
     * @brief Returns true if @p str is a valid memory offset (decimal or hex, non-negative).
     * @param str String view to validate.
     */
    static bool IsValidOffset(std::string_view str);

    /**
     * @brief Returns true if @p str is a valid memory offset (decimal or hex, non-negative).
     * @param str String to validate.
     */
    static bool IsValidOffset(const std::string& str);

    /**
     * @brief Case-insensitive string equality.
     * @param a First string.
     * @param b Second string.
     * @return true if @p a and @p b differ only in case.
     */
    static bool IEquals(const std::string& a, const std::string& b);

    /**
     * @brief Decodes a hexadecimal string (with or without "0x" prefix) into raw bytes.
     * @param hex Hexadecimal string (e.g. "DEADBEEF" or "0xDE0xAD…").
     * @return Decoded bytes.
     */
    static std::vector<uint8_t> HexToBytes(const std::string& hex);

    /**
     * @brief Encodes a byte buffer as a lowercase hexadecimal string.
     * @param bytes Byte buffer to encode.
     * @return Hex string (no "0x" prefix, two characters per byte).
     */
    static std::string BytesToHex(const std::vector<unsigned char>& bytes);
};