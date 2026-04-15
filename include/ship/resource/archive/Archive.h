#pragma once

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include "ship/utils/binarytools/BinaryReader.h"

namespace tinyxml2 {
class XMLDocument;
class XMLElement;
} // namespace tinyxml2

namespace Ship {
/** @brief Byte size of the fixed OTR archive header block. */
#define OTR_HEADER_SIZE ((size_t)64)

struct File;
struct ResourceInitData;

/**
 * @brief Metadata block embedded in an archive's manifest file.
 *
 * Populated by Archive subclasses when they open and parse the manifest entry.
 * Fields that are not present in a particular archive are left as empty strings
 * or zero values.
 */
struct ArchiveManifest {
    std::string Name;        ///< Display name of the mod/archive.
    std::string Icon;        ///< Path to the mod's icon within the archive.
    std::string Author;      ///< Author(s) of the archive.
    std::string Version;     ///< Human-readable version string.
    std::string Website;     ///< Author's website URL.
    std::string Description; ///< Short description of the archive contents.
    std::string License;     ///< SPDX license identifier or URL.

    uint32_t CodeVersion; ///< API version of compiled script modules bundled with this archive.
    uint32_t GameVersion; ///< Numeric game version this archive targets.

    std::string Main;                                      ///< Entry-point script path within the archive.
    std::unordered_map<std::string, std::string> Binaries; ///< Platform → binary path map for bundled native libs.
    std::vector<std::string> Dependencies;                 ///< Archive names that must be loaded before this one.

    std::string Checksum;  ///< Hex-encoded SHA-256 checksum of the archive contents.
    std::string Signature; ///< Base64-encoded digital signature over the checksum.
    std::string PublicKey; ///< Base64-encoded public key used to verify the signature.
};

/**
 * @brief Abstract base class representing a mounted archive (OTR, O2R, or folder).
 *
 * An Archive encapsulates a collection of virtual files addressable by path or
 * 64-bit hash. Concrete subclasses (OtrArchive, O2rArchive, FolderArchive) implement
 * the platform- and format-specific I/O logic.
 *
 * Archives are managed by ArchiveManager, which distributes load requests across all
 * currently mounted archives.
 */
class Archive : public std::enable_shared_from_this<Archive> {
    friend class ArchiveManager;

  public:
    /**
     * @brief Constructs an Archive for the given filesystem path.
     * @param path Absolute or relative path to the archive file or directory.
     */
    Archive(const std::string& path);
    ~Archive();

    /** @brief Two archives are equal when they refer to the same underlying path. */
    bool operator==(const Archive& rhs) const;

    /**
     * @brief Opens and indexes the archive, making its contents available for loading.
     *
     * Calls Open() and then validates the manifest and signature if present.
     */
    void Load();

    /**
     * @brief Closes the archive and releases all associated resources.
     *
     * Calls Close() and clears the internal hash table.
     */
    void Unload();

    /**
     * @brief Loads a file by its virtual path.
     * @param filePath Virtual path within the archive (e.g. "textures/foo.tex").
     * @return Loaded File with a populated buffer, or nullptr if not found.
     */
    virtual std::shared_ptr<File> LoadFile(const std::string& filePath) = 0;

    /**
     * @brief Loads a file by its 64-bit content hash.
     * @param hash CRC/hash of the file path.
     * @return Loaded File with a populated buffer, or nullptr if not found.
     */
    virtual std::shared_ptr<File> LoadFile(uint64_t hash) = 0;

    /**
     * @brief Returns a map of all files indexed in this archive (hash → path).
     * @return Shared pointer to the complete hash→path map.
     */
    std::shared_ptr<std::unordered_map<uint64_t, std::string>> ListFiles();

    /**
     * @brief Returns only the files whose paths match the given filter string.
     * @param filter Substring or glob pattern to match against virtual paths.
     * @return Shared pointer to a filtered hash→path map.
     */
    std::shared_ptr<std::unordered_map<uint64_t, std::string>> ListFiles(const std::string& filter);

    /**
     * @brief Checks whether the archive contains a file at the given path.
     * @param filePath Virtual path to look up.
     * @return true if the file exists in this archive.
     */
    bool HasFile(const std::string& filePath);

    /**
     * @brief Checks whether the archive contains a file with the given hash.
     * @param hash 64-bit hash of the file path.
     * @return true if the hash is found in this archive.
     */
    bool HasFile(uint64_t hash);

    /**
     * @brief Returns true if a game version value was found in the manifest.
     */
    bool HasGameVersion();

    /**
     * @brief Returns the game version encoded in the archive manifest.
     * @note Check HasGameVersion() first; the value is undefined if there is none.
     */
    uint32_t GetGameVersion();

    /** @brief Returns the parsed archive manifest. */
    const ArchiveManifest& GetManifest();

    /**
     * @brief Returns true if the archive carries a digital signature in its manifest.
     */
    bool IsSigned();

    /**
     * @brief Returns true if the archive's checksum field matches the computed checksum.
     */
    bool IsChecksumValid();

    /** @brief Returns the filesystem path this archive was opened from. */
    const std::string& GetPath();

    /** @brief Returns true if the archive has been successfully opened. */
    bool IsLoaded();

    /**
     * @brief Opens the underlying archive file or directory for reading.
     * @return true on success.
     */
    virtual bool Open() = 0;

    /**
     * @brief Closes the underlying archive file or directory.
     * @return true on success.
     */
    virtual bool Close() = 0;

    /**
     * @brief Writes raw data into the archive at the given path.
     * @param filename Target virtual path within the archive.
     * @param data     Raw bytes to write.
     * @return true on success.
     */
    virtual bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data) = 0;

  protected:
    /** @brief Sets the loaded state flag. Called by Load() / Unload(). */
    void SetLoaded(bool isLoaded);
    /** @brief Stores the game version parsed from the manifest. */
    void SetGameVersion(uint32_t gameVersion);
    /**
     * @brief Adds a file to the internal hash→path index.
     * @param filePath Virtual path of the file to index.
     */
    void IndexFile(const std::string& filePath);
    /** @brief Validates the manifest checksum and signature, setting mIsSigned / mIsChecksumValid. */
    void Validate();

  private:
    bool mIsLoaded;
    bool mIsSigned;
    bool mIsChecksumValid;
    bool mHasGameVersion;
    uint32_t mGameVersion;
    ArchiveManifest mManifest;
    std::string mPath;
    std::shared_ptr<std::unordered_map<uint64_t, std::string>> mHashes;
};
} // namespace Ship
