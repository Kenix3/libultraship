#pragma once

#undef _DLL

#include <string>
#include <stdint.h>
#include <string>

#include "ship/resource/File.h"
#include "ship/resource/Resource.h"
#include "ship/resource/archive/Archive.h"

namespace Ship {
struct File;

class FolderArchive final : virtual public Archive {
  public:
    FolderArchive(const std::string& archivePath);
    ~FolderArchive();

    bool Open();
    bool Close();
    bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data);
    std::shared_ptr<File> LoadFile(const std::string& filePath);
    std::shared_ptr<File> LoadFile(uint64_t hash);

  protected:
    std::shared_ptr<File> LoadFileRaw(const std::string& filePath);
    std::shared_ptr<File> LoadFileRaw(uint64_t hash);

  private:
    std::string mArchiveBasePath;
};
} // namespace Ship