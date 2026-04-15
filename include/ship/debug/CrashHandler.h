#pragma once

#include <cstddef>
#include <memory>

#if (__linux__)
#include <csignal>
#include <cstdio>
#include <cxxabi.h> // for __cxa_demangle
#include <dlfcn.h>  // for dladdr
#include <execinfo.h>
#include <unistd.h>
#include <SDL.h>
#endif

#if _WIN32
#include <windows.h>
#endif

namespace Ship {

/**
 * @brief Callback type invoked by CrashHandler when a crash is detected.
 *
 * The callback receives a mutable character buffer (@p char*) and its current
 * write position (@p size_t*) so that it can append additional context to the
 * crash report before it is flushed.
 */
typedef void (*CrashHandlerCallback)(char*, size_t*);

/**
 * @brief Installs platform-specific signal / exception handlers to capture crash information.
 *
 * CrashHandler sets up OS-level handlers (POSIX signals on Linux, SEH on Windows)
 * that fill a fixed-size text buffer with a human-readable crash report, then invoke
 * any registered CrashHandlerCallback so the application can append game-specific
 * state before the process exits.
 *
 * Obtain the instance from Context::GetCrashHandler().
 */
class CrashHandler {
  public:
    /** @brief Installs the platform crash handlers with no application callback. */
    CrashHandler();
    ~CrashHandler();

    /**
     * @brief Installs the platform crash handlers and immediately registers @p callback.
     * @param callback Function to invoke when a crash is detected; may be nullptr.
     */
    CrashHandler(CrashHandlerCallback callback);

    /**
     * @brief Registers (or replaces) the application callback.
     * @param callback Function to invoke on crash; pass nullptr to clear the callback.
     */
    void RegisterCallback(CrashHandlerCallback callback);

    /**
     * @brief Appends @p str followed by a newline to the crash report buffer.
     * @param str Null-terminated string to append.
     */
    void AppendLine(const char* str);

    /**
     * @brief Appends @p str (without a trailing newline) to the crash report buffer.
     * @param str Null-terminated string to append.
     */
    void AppendStr(const char* str);

    /**
     * @brief Writes platform-agnostic crash header information (OS, build info, etc.)
     * to the report buffer.
     */
    void PrintCommon();

#ifdef __linux__
    /**
     * @brief Appends the values of the CPU registers from @p ctx to the crash report.
     * @param ctx Signal handler context containing the register state.
     */
    void PrintRegisters(ucontext_t* ctx);
#elif _WIN32
    /**
     * @brief Appends the values of the CPU registers from @p ctx to the crash report.
     * @param ctx Windows CONTEXT structure containing the register state.
     */
    void PrintRegisters(CONTEXT* ctx);

    /**
     * @brief Walks the call stack using @p ctx and appends it to the crash report.
     * @param ctx Windows CONTEXT structure used as the starting frame for stack unwinding.
     */
    void PrintStack(CONTEXT* ctx);
#endif

  private:
    CrashHandlerCallback mCallback = nullptr;
    std::unique_ptr<char[]> mOutBuffer;
    static constexpr size_t gMaxBufferSize = 32768;
    size_t mOutBuffersize = 0;

    /** @brief Appends @p str to the buffer, truncating if necessary to avoid overflow. */
    void AppendStrTrunc(const char* str);

    /** @brief Returns true if @p str fits within the remaining buffer space. */
    bool CheckStrLen(const char* str);
};
} // namespace Ship
