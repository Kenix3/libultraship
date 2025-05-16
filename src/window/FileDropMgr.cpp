#include "FileDropMgr.h"
#include <cstdlib>
#include <cstring>
#include <spdlog/spdlog.h>
#ifdef _MSC_VER
#define strdup _strdup
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

} // namespace Ship