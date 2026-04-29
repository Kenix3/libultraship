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

/**
 * @brief Archive implementation backed by an on-disk directory tree.
 *
 * FolderArchive treats an ordinary filesystem directory as a virtual archive.
 * Files inside the directory are addressed by their relative paths, just like
 * files inside an OTR or O2R archive. This makes it easy to load loose-file
 * mods or development assets without packing them first.
 */
class FolderArchive final : virtual public Archive {
  public:
    /**
     * @brief Constructs a FolderArchive rooted at the given directory.
     * @param archivePath Path to the directory to use as the archive root.
     */
    FolderArchive(const std::string& archivePath);
    ~FolderArchive();

    /**
     * @brief Scans the directory and indexes all files found within it.
     * @return true on success, false if the directory does not exist or cannot be read.
     */
    bool Open();

    /**
     * @brief Closes the archive and clears the internal file index.
     * @return true on success.
     */
    bool Close();

    /**
     * @brief Writes raw data to a file inside the archive directory.
     * @param filename Relative path within the archive directory.
     * @param data     Raw bytes to write.
     * @return true on success.
     */
    bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data);

    /**
     * @brief Loads a file by its virtual path relative to the archive root.
     * @param filePath Relative file path (e.g. "textures/foo.tex").
     * @return Loaded File with a populated buffer, or nullptr if not found.
     */
    std::shared_ptr<File> LoadFile(const std::string& filePath);

    /**
     * @brief Loads a file by its 64-bit path hash.
     * @param hash CRC/hash of the file path.
     * @return Loaded File with a populated buffer, or nullptr if not found.
     */
    std::shared_ptr<File> LoadFile(uint64_t hash);

  protected:
    /**
     * @brief Reads a file from disk without header parsing.
     * @param filePath Relative file path within the archive directory.
     * @return Raw File, or nullptr if the file cannot be read.
     */
    std::shared_ptr<File> LoadFileRaw(const std::string& filePath);

    /**
     * @brief Reads a file from disk by hash without header parsing.
     * @param hash 64-bit hash of the file path.
     * @return Raw File, or nullptr if the hash is unknown or the file cannot be read.
     */
    std::shared_ptr<File> LoadFileRaw(uint64_t hash);

  private:
    std::string mArchiveBasePath;
};
} // namespace Ship