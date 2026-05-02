#pragma once

#include <string>
#include <memory>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <stdint.h>
#include <functional>
#include "ship/resource/File.h"
#include "ship/Component.h"
#include "ship/security/Keystore.h"

namespace Ship {
struct File;
class Archive;
/**
 * @brief Callback invoked when an archive without a trusted key is added.
 *
 * The handler receives a reference to the untrusted Archive and the key entry from
 * the Keystore. Return true to accept the archive anyway, or false to reject it.
 */
using UntrustedArchiveHandler = std::function<bool(Archive& archive, KeystoreEntry& key)>;

/**
 * @brief Manages a prioritised collection of mounted Archive objects.
 *
 * ArchiveManager presents a unified virtual filesystem over all mounted archives.
 * When multiple archives contain a file with the same path, the archive added most
 * recently (highest index) takes precedence, allowing "mod" archives to override
 * base-game assets.
 *
 * File lookups, directory listings, and game-version validation are all delegated
 * here from the ResourceManager layer.
 *
 * **Required Context children (looked up at runtime):**
 * - **ResourceManager** — used by Archive objects during mount validation to
 *   check game versions. ResourceManager must be added to the Context before
 *   ArchiveManager::Init() is called.
 * - **Keystore** — used by Archive objects to verify archive signatures. Keystore
 *   must be added to the Context before ArchiveManager::Init() is called.
 *
 * Obtain the instance from
 * `Context::GetChildren().GetFirst<ResourceManager>()->GetArchiveManager()`.
 */
class ArchiveManager : public Component {
  public:
    ArchiveManager();

    /**
     * @brief Discovers and mounts all archives found under the given paths.
     * @param archivePaths Filesystem paths to archive files or directories to search.
     */
    void Init(const std::vector<std::string>& archivePaths);

    /**
     * @brief Discovers and mounts archives, filtering by acceptable game versions.
     * @param archivePaths       Filesystem paths to archive files or directories.
     * @param validGameVersions  Set of game-version values to accept; others are rejected.
     */
    void Init(const std::vector<std::string>& archivePaths, const std::unordered_set<uint32_t>& validGameVersions);
    ~ArchiveManager();

    /**
     * @brief Opens and adds an archive by filesystem path.
     * @param archivePath Absolute or relative path to the archive file or directory.
     * @return Shared pointer to the newly added Archive, or nullptr on failure.
     */
    std::shared_ptr<Archive> AddArchive(const std::string& archivePath);

    /**
     * @brief Adds a pre-constructed Archive object to the managed collection.
     * @param archive Already-constructed (but not yet loaded) Archive.
     * @return Shared pointer to the added Archive.
     */
    std::shared_ptr<Archive> AddArchive(std::shared_ptr<Archive> archive);

    /** @brief Returns all currently mounted archives in priority order (last = highest priority). */
    std::shared_ptr<std::vector<std::shared_ptr<Archive>>> GetArchives();

    /**
     * @brief Replaces the entire archive list with a new collection.
     * @param archives New ordered list of archives.
     */
    void SetArchives(std::shared_ptr<std::vector<std::shared_ptr<Archive>>> archives);

    /**
     * @brief Removes a specific archive from the managed collection.
     * @param archive Archive to remove.
     * @return Number of archives removed (0 or 1).
     */
    size_t RemoveArchive(std::shared_ptr<Archive> archive);

    /**
     * @brief Removes the archive at the given filesystem path.
     * @param path Filesystem path of the archive to remove.
     * @return Number of archives removed (0 or 1).
     */
    size_t RemoveArchive(const std::string& path);

    /**
     * @brief Returns true if at least one archive has been successfully loaded.
     */
    bool IsLoaded();

    /**
     * @brief Loads raw file bytes from the highest-priority archive that contains the path.
     * @param filePath Virtual path of the file.
     * @return Loaded File, or nullptr if no mounted archive contains the file.
     */
    std::shared_ptr<File> LoadFile(const std::string& filePath);

    /**
     * @brief Loads raw file bytes by 64-bit hash from the highest-priority matching archive.
     * @param hash CRC/hash of the file path.
     * @return Loaded File, or nullptr if not found.
     */
    std::shared_ptr<File> LoadFile(uint64_t hash);

    /**
     * @brief Writes raw data into a specific archive.
     * @param archive  Target archive (must be mounted and writable).
     * @param filename Virtual path to write to within the archive.
     * @param data     Raw bytes to write.
     * @return true on success.
     */
    bool WriteFile(std::shared_ptr<Archive> archive, const std::string& filename, const std::vector<uint8_t>& data);

    /**
     * @brief Checks whether any mounted archive contains a file at the given path.
     * @param filePath Virtual path to check.
     * @return true if at least one archive has the file.
     */
    bool HasFile(const std::string& filePath);

    /**
     * @brief Checks whether any mounted archive contains a file with the given hash.
     * @param hash 64-bit hash of the file path.
     * @return true if at least one archive has the file.
     */
    bool HasFile(uint64_t hash);

    /**
     * @brief Returns the highest-priority archive that contains the given file.
     * @param filePath Virtual path of the file.
     * @return Shared pointer to the owning Archive, or nullptr if not found.
     */
    std::shared_ptr<Archive> GetArchiveFromFile(const std::string& filePath);

    /**
     * @brief Lists virtual paths of all files matching the given search mask across all archives.
     * @param searchMask Glob pattern (empty = list everything).
     * @return Sorted, deduplicated list of matching paths.
     */
    std::shared_ptr<std::vector<std::string>> ListFiles(const std::string& searchMask = "");

    /**
     * @brief Lists virtual paths matching include/exclude glob masks across all archives.
     * @param includes Paths must match at least one of these patterns.
     * @param excludes Paths matching any of these patterns are removed from the result.
     * @return Sorted, deduplicated list of matching paths.
     */
    std::shared_ptr<std::vector<std::string>> ListFiles(const std::list<std::string>& includes,
                                                        const std::list<std::string>& excludes);

    /**
     * @brief Lists virtual directory paths matching the given search mask across all archives.
     * @param searchMask Glob pattern (empty = list all directories).
     * @return Sorted, deduplicated list of matching directory paths.
     */
    std::shared_ptr<std::vector<std::string>> ListDirectories(const std::string& searchMask = "");

    /**
     * @brief Returns the union of all game-version values found across mounted archives.
     * @return Vector of numeric game-version values.
     */
    std::vector<uint32_t> GetGameVersions();

    /**
     * @brief Looks up the virtual path string for a given 64-bit hash.
     * @param hash Hash to resolve.
     * @return Pointer to the path string, or nullptr if the hash is not indexed.
     */
    const std::string* HashToString(uint64_t hash) const;

    /**
     * @brief Looks up the virtual path C-string for a given 64-bit hash.
     * @param hash Hash to resolve.
     * @return C-string pointer, or nullptr if not found.
     */
    const char* HashToCString(uint64_t hash) const;

    /**
     * @brief Returns true if the given game version is in the set of valid versions.
     * @param gameVersion Game version to validate.
     */
    bool IsGameVersionValid(uint32_t gameVersion);

    /**
     * @brief Sets the callback invoked when an untrusted (unsigned/unknown-key) archive is added.
     * @param handler Callback; return true to accept, false to reject the archive.
     */
    void SetUntrustedArchiveHandler(const UntrustedArchiveHandler& handler);

    /** @brief Returns the current untrusted-archive handler. */
    UntrustedArchiveHandler GetUntrustedArchiveHandler() const;

  protected:
    /**
     * @brief Scans the given filesystem paths and returns a flat list of archive file paths.
     * @param archivePaths Directories or explicit file paths to scan.
     * @return List of discovered archive file paths.
     */
    static std::vector<std::string> GetArchiveListInPaths(const std::vector<std::string>& archivePaths);

    /** @brief Adds a game-version value to the internal version set. */
    void AddGameVersion(uint32_t newGameVersion);

    /** @brief Rebuilds the hash-to-path and path-to-archive lookup tables from the current archive list. */
    void ResetVirtualFileSystem();

  private:
    std::vector<std::shared_ptr<Archive>> mArchives;
    std::vector<uint32_t> mGameVersions;
    std::unordered_set<uint32_t> mValidGameVersions;
    std::unordered_map<uint64_t, std::string> mHashes;
    std::unordered_set<std::string> mDirectories;
    std::unordered_map<uint64_t, std::shared_ptr<Archive>> mFileToArchive;
    UntrustedArchiveHandler mUntrustedArchiveHandler;
};
} // namespace Ship
