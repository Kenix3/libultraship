#pragma once

#undef _DLL

#include <string>

#include <stdint.h>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <StormLib.h>

#include "resource/Resource.h"
#include "resource/archive/Archive.h"

namespace LUS {
struct File;

class OtrArchive : virtual public Archive {
  public:
    OtrArchive(const std::string& archivePath);
    ~OtrArchive();

  protected:
    std::shared_ptr<File> LoadFileRaw(const std::string& filePath);
    std::shared_ptr<File> LoadFileRaw(uint64_t hash);
    std::shared_ptr<ResourceInitData> LoadFileMeta(const std::string& filePath);
    std::shared_ptr<ResourceInitData> LoadFileMeta(uint64_t hash);

    bool LoadRaw();
    bool UnloadRaw();

  private:
    HANDLE mHandle;
};
} // namespace LUS
