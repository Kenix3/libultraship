#ifndef FILEDROPMGR_H
#define FILEDROPMGR_H

namespace Ship {

class FileDropMgr {
  public:
    FileDropMgr() = default;
    ~FileDropMgr();
    void SetDroppedFile(char* path);
    void ClearDroppedFile();
    bool FileDropped() const;
    char* GetDroppedFile() const;

  private:
    char* mPath = nullptr;
    bool mFileDropped = false;
};

} // namespace Ship

#endif // FILEDROPMGR_H
