#ifdef INCLUDE_MPQ_SUPPORT

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

#include "ship/resource/Resource.h"
#include "ship/resource/archive/Archive.h"

namespace Ship {
struct File;

/**
 * @brief Archive implementation backed by an MPQ file (Blizzard's MoPaQ format).
 *
 * OtrArchive wraps StormLib to expose a Ship::Archive over an ".otr" file, which is
 * structurally an MPQ archive. This class is only compiled when the project is built
 * with MPQ support enabled (`-DINCLUDE_MPQ_SUPPORT`).
 *
 * All file I/O operations are thread-safe via the StormLib handle.
 */
class OtrArchive final : virtual public Archive {
  public:
    /**
     * @brief Constructs an OtrArchive for the given MPQ file path.
     * @param archivePath Absolute or relative filesystem path to the ".otr" file.
     */
    OtrArchive(const std::string& archivePath);
    ~OtrArchive();

    /**
     * @brief Opens the underlying MPQ file via StormLib.
     * @return true if the file was opened successfully.
     */
    bool Open();

    /**
     * @brief Closes the MPQ file handle and releases StormLib resources.
     * @return true on success.
     */
    bool Close();

    /**
     * @brief Writes raw data to a file entry inside the MPQ archive.
     * @param filename Target path within the archive.
     * @param data     Raw bytes to write.
     * @return true on success.
     */
    bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data);

    /**
     * @brief Loads a file from the archive by its virtual path.
     * @param filePath Virtual path of the file within the MPQ.
     * @return Loaded File with a populated buffer, or nullptr if not found.
     */
    std::shared_ptr<File> LoadFile(const std::string& filePath);

    /**
     * @brief Loads a file from the archive by its 64-bit hash.
     * @param hash CRC/hash of the file path.
     * @return Loaded File with a populated buffer, or nullptr if not found.
     */
    std::shared_ptr<File> LoadFile(uint64_t hash);

  private:
    HANDLE mHandle;
};
} // namespace Ship

#endif // INCLUDE_MPQ_SUPPORT
