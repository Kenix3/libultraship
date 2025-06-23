#include "FileDropMgr.h"
#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>
#ifdef _MSC_VER
#define strdup _strdup
#include "dgbhelp.h"
#endif
#include "Context.h"
#include "Window.h"
#ifdef __unix__
#include <dlfcn.h>
#include <cxxabi.h>
#endif

namespace Ship {
FileDropMgr::~FileDropMgr() {
    if (mPath != nullptr) {
        free(mPath);
    }
}

void FileDropMgr::SetDroppedFile(char* path) {
    if (mPath != nullptr) {
        SPDLOG_WARN("Overwriting dropped file: {} with {}", mPath, path);
        free(mPath);
    }
    mPath = strdup(path);
    mFileDropped = true;
    CallHandlers();
}

void FileDropMgr::ClearDroppedFile() {
    if (mPath != nullptr) {
        free(mPath);
        mPath = nullptr;
    }
    mFileDropped = false;
}

bool FileDropMgr::FileDropped() const {
    return mFileDropped;
}

char* FileDropMgr::GetDroppedFile() const {
    return mPath;
}

static void PrintRegError(void* funcAddr) {
#ifdef __unix__
    Dl_info info;
    int gotAddress = dladdr(funcAddr, &info);
    const char* nameFound = info.dli_sname;
    char* demangledName = nullptr;
    if (gotAddress && info.dli_sname != nullptr) {
        int status;
        demangledName = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
        if (status == 0) {
            nameFound = demangledName;
        }
    }
    SPDLOG_WARN("Trying to register {}. Already registered.", nameFound);
    if (demangledName != nullptr) {
        free (demangledName);
    }
#elif _MSC_VER
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME + sizeof(TCHAR)];
    char module[512];

    PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;
    HANDLE hProcess = GetCurrentProcess();
    SymInitialize(hProcess, "debug", true);
    SymFromAddr(hProcess, funcAddr, nullptr, symbol)
    SPDLOG_WARN("Trying to register {}. Already registered.", symbol->Name);
    SymCleanup(hProcess);
#endif
}

bool FileDropMgr::RegisterDropHandler(FileDroppedFunc func) {
    for (const auto f : mRegisteredFuncs) {
        if (func == f) {
            PrintRegError((void*)func);
            return false;
        }
    }
    mRegisteredFuncs.push_back(func);
    return true;
}

bool FileDropMgr::UnregisterDropHandler(FileDroppedFunc func) {
    for (auto it = mRegisteredFuncs.begin(); it != mRegisteredFuncs.end(); ++it) {
        if (*it == func) {
            mRegisteredFuncs.erase(it);
            return true;
        }
    }
    return false;
}

void FileDropMgr::CallHandlers() {
    for (const auto f : mRegisteredFuncs) {
        if (f(mPath)) {
            return;
        }
    }
    SPDLOG_WARN("Dropped file {} not handled by any registered.", mPath);
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    gui->GetGameOverlay()->TextDrawNotification(30.0f, true, "Unsupported file dropped, ignoring");
}


} // namespace Ship