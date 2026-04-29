/**
 * @file Api.h
 * @brief API export/import macros for cross-platform shared library symbol visibility.
 *
 * Provides the API_EXTERN and API_EXPORT macros used to mark functions and variables
 * that need C linkage and/or DLL export/import attributes on Windows.
 */

#pragma once

/**
 * @brief Applies C linkage when compiling as C++, or plain extern in C.
 *
 * Wraps symbols with `extern "C"` in C++ translation units so they are
 * accessible from C code or from dynamic loaders that expect C-style names.
 */
#ifdef __cplusplus
#define API_EXTERN extern "C"
#else
#define API_EXTERN extern
#endif

/**
 * @brief Marks a symbol for export from (or import into) a shared library.
 *
 * On Windows, this resolves to `__declspec(dllexport)` when building the library
 * and `__declspec(dllimport)` when consuming it (i.e. when __DLL__ is defined).
 * On other platforms it is equivalent to API_EXTERN.
 */
#ifdef _WIN32
#ifndef __DLL__
#define API_EXPORT API_EXTERN __declspec(dllexport)
#else
#define API_EXPORT API_EXTERN __declspec(dllimport)
#endif
#else
#define API_EXPORT API_EXTERN
#endif