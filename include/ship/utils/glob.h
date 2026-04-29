#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tests whether a string matches a glob pattern.
 *
 * Supports the wildcard characters '*' (matches zero or more characters) and
 * '?' (matches exactly one character).
 *
 * @param pat Null-terminated glob pattern.
 * @param str Null-terminated string to test.
 * @return true if @p str matches @p pat, false otherwise.
 */
bool glob_match(char const* pat, char const* str);

#ifdef __cplusplus
};
#endif