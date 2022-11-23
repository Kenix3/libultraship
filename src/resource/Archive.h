#pragma once

#undef _DLL

#include <string>

#include <stdint.h>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <unordered_set>
#include "Resource.h"
#include <StormLib.h>

namespace Ship {
struct OtrFile;

class Archive : public std::enable_shared_from_this<Archive> {
  public:
    Archive(const std::string& mainPath, bool enableWriting);
    Archive(const std::string& mainPath, const std::string& patchesPath,
            const std::unordered_set<uint32_t>& validHashes, bool enableWriting, bool generateCrcMap = true);
    Archive(const std::vector<std::string>& otrFiles, const std::unordered_set<uint32_t>& validHashes,
            bool enableWriting, bool generateCrcMap = true);
    ~Archive();

    bool IsMainMPQValid();

    static std::shared_ptr<Archive> CreateArchive(const std::string& archivePath, int fileCapacity);

    std::shared_ptr<OtrFile> LoadFile(const std::string& filePath, bool includeParent = true,
                                      std::shared_ptr<OtrFile> fileToLoad = nullptr);

    bool AddFile(const std::string& path, uintptr_t fileData, DWORD fileSize);
    bool RemoveFile(const std::string& path);
    bool RenameFile(const std::string& oldPath, const std::string& newPath);
    std::vector<SFILE_FIND_DATA> ListFiles(const std::string& searchMask) const;
    bool HasFile(const std::string& searchMask) const;
    const std::string* HashToString(uint64_t hash) const;
    std::vector<uint32_t> GetGameVersions();
    void PushGameVersion(uint32_t newGameVersion);

  protected:
    bool Load(bool enableWriting, bool generateCrcMap);
    bool Unload();

  private:
    std::string mMainPath;
    std::string mPatchesPath;
    std::vector<std::string> mOtrFiles;
    std::unordered_set<uint32_t> mValidHashes;
    std::map<std::string, HANDLE> mMpqHandles;
    std::vector<std::string> mAddedFiles;
    std::vector<uint32_t> mGameVersions;
    std::unordered_map<uint64_t, std::string> mHashes;
    HANDLE mMainMpq;

    bool LoadMainMPQ(bool enableWriting, bool generateCrcMap);
    bool LoadPatchMPQs();
    bool LoadPatchMPQ(const std::string& path, bool validateVersion = false);
    void GenerateCrcMap();
    bool ProcessOtrVersion(HANDLE mpqHandle = nullptr);
    std::shared_ptr<OtrFile> LoadFileFromHandle(const std::string& filePath, bool includeParent = true,
                                                std::shared_ptr<OtrFile> fileToLoad = nullptr,
                                                HANDLE mpqHandle = nullptr);
};
} // namespace Ship
