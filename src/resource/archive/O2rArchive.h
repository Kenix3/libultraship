#pragma once

#undef _DLL

#include <string>
#include <stdint.h>
#include <string>

#include "zip.h"

#include "resource/File.h"
#include "resource/Resource.h"
#include "resource/archive/Archive.h"

namespace Ship {
struct File;

class O2rArchive : virtual public Archive {
  public:
    O2rArchive(const std::string& archivePath);
    ~O2rArchive();

    bool Open();
    bool Close();
    bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data);

    std::shared_ptr<File> LoadFile(const std::string& filePath);
    std::shared_ptr<File> LoadFile(uint64_t hash);

  private:
    zip_t* mZipArchive;
};
} // namespace Ship
