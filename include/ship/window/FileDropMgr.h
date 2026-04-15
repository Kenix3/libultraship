#pragma once
#include <vector>
#include "ship/Component.h"

typedef bool (*FileDroppedFunc)(char*);

namespace Ship {

class FileDropMgr : public Component {
  public:
    FileDropMgr();
    ~FileDropMgr();
    void SetDroppedFile(char* path);
    void ClearDroppedFile();
    bool FileDropped() const;
    char* GetDroppedFile() const;
    bool RegisterDropHandler(FileDroppedFunc func);
    bool UnregisterDropHandler(FileDroppedFunc func);
    void CallHandlers();

  private:
    std::vector<FileDroppedFunc> mRegisteredFuncs;
    char* mPath = nullptr;
    bool mFileDropped = false;
};

} // namespace Ship
