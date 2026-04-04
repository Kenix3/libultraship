#pragma once

#undef _DLL

#include <string>
#include <stdint.h>
#include <string>
#include <mutex>
#include <vector>

#include "zip.h"

#include "ship/resource/File.h"
#include "ship/resource/Resource.h"
#include "ship/resource/archive/Archive.h"

namespace Ship {
struct File;

class O2rArchive final : virtual public Archive {
  public:
    O2rArchive(const std::string& archivePath);
    ~O2rArchive();

    bool Open();
    bool Close();
    bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data);

    std::shared_ptr<File> LoadFile(const std::string& filePath);
    std::shared_ptr<File> LoadFile(uint64_t hash);

  private:
    zip_t* GetZipHandle();
    void ReleaseZipHandle(zip_t* handle);
    zip_t* mZipArchive;
    std::mutex mPoolMutex;
    std::vector<zip_t*> mZipArchivePool;
};
} // namespace Ship
