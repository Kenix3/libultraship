#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#include <stddef.h>

typedef void (*CrashHandlerCallback)(char*, size_t*);

#if (__linux) 
#include <csignal>
#include <cstdio>
#include <cxxabi.h> // for __cxa_demangle
#include <dlfcn.h>  // for dladdr
#include <execinfo.h>
#include <unistd.h>
#include <SDL.h>
#endif

class CrashHandler {
  public:
    CrashHandler();
    CrashHandler(CrashHandlerCallback callback);

    void RegisterCallback(CrashHandlerCallback callback);
    void AppendLine(const char* str);
    void AppendStr(const char* str);
#ifdef __linux__
    void PrintRegisters(ucontext_t* ctx);
#elif _WIN32
    void PrintRegisters(CONTEXT* ctx);
    void PrintStack(CONTEXT* ctx);
#endif
    void PrintCommon();

  private:
    CrashHandlerCallback mCallback = nullptr;
    char* mOutBuffer = nullptr;
    const size_t MAX_BUFFER_SIZE = 32768;
    size_t mOutBuffersize = 0;


    void AppendStrTrunc(const char* str);
    bool CheckStrLen(const char* str);
};


#endif // CRASH_HANDLER_H
