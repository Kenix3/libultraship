#pragma once

#ifdef __cplusplus
#define API_EXTERN extern "C"
#else
#define API_EXTERN extern
#endif

#ifdef _WIN32
#ifndef __DLL__
#define API_EXPORT API_EXTERN __declspec(dllexport)
#else
#define API_EXPORT API_EXTERN __declspec(dllimport)
#endif
#else
#define API_EXPORT API_EXTERN
#endif