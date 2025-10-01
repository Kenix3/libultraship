#pragma once
#include <vector>

typedef bool (*FileDroppedFunc)(char*);

namespace Ship {

class FileDropMgr {
  public:
    FileDropMgr() = default;
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
