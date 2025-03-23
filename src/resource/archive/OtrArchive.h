#ifndef EXCLUDE_MPQ_SUPPORT

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

namespace Ship {
struct File;

class OtrArchive : virtual public Archive {
  public:
    OtrArchive(const std::string& archivePath);
    ~OtrArchive();

    bool Open();
    bool Close();
    bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data);

    std::shared_ptr<File> LoadFile(const std::string& filePath);
    std::shared_ptr<File> LoadFile(uint64_t hash);

  private:
    HANDLE mHandle;
};
} // namespace Ship

#endif // EXCLUDE_MPQ_SUPPORT
