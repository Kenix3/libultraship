#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
bool glob_match(char const* pat, char const* str);
#ifdef __cplusplus
};
#endif