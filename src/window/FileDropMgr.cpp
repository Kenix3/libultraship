#include "FileDropMgr.h"
#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>
#ifdef _MSC_VER
#define strdup _strdup
#endif
#include "Context.h"
#include "Window.h"

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

bool FileDropMgr::RegisterDropHandler(FileDroppedFunc func) {
    for (const auto f : mRegisteredFuncs) {
        if (func == f) {
            //Dl_info info;
            //dladdr(func, &info);
            //SPDLOG_WARN("Trying to register {}. Already registered.", info.dli_sname);
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