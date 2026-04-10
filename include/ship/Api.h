#pragma once

#ifdef _WIN32
#ifdef __cplusplus
#define API_EXPORT extern "C" __declspec(dllexport)
#else
#define API_EXPORT __declspec(dllexport)
#endif
#else
#define API_EXPORT
#endif