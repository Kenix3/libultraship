#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stdint.h>
#include "resource/File.h"

namespace Ship {
struct File;
class Archive;

class ArchiveManager {
  public:
    ArchiveManager();
    void Init(const std::vector<std::string>& archivePaths);
    void Init(const std::vector<std::string>& archivePaths, const std::unordered_set<uint32_t>& validGameVersions);
    ~ArchiveManager();

    bool IsArchiveLoaded();
    std::shared_ptr<File> LoadFile(const std::string& filePath, std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_ptr<File> LoadFile(uint64_t hash, std::shared_ptr<ResourceInitData> initData = nullptr);
    bool HasFile(const std::string& filePath);
    bool HasFile(uint64_t hash);
    std::shared_ptr<std::vector<std::string>> ListFiles(const std::string& filter);
    std::shared_ptr<std::vector<std::string>> ListFiles();
    std::vector<uint32_t> GetGameVersions();
    std::vector<std::shared_ptr<Archive>> GetArchives();
    void SetArchives(const std::vector<std::shared_ptr<Archive>>& archives);
    const std::string* HashToString(uint64_t hash) const;
    bool IsGameVersionValid(uint32_t gameVersion);

  protected:
    static std::vector<std::string> GetArchiveListInPaths(const std::vector<std::string>& archivePaths);

    std::shared_ptr<Archive> AddArchive(const std::string& archivePath);
    std::shared_ptr<Archive> AddArchive(std::shared_ptr<Archive> archive);
    void AddGameVersion(uint32_t newGameVersion);

  private:
    std::vector<std::shared_ptr<Archive>> mArchives;
    std::vector<uint32_t> mGameVersions;
    std::unordered_set<uint32_t> mValidGameVersions;
    std::unordered_map<uint64_t, std::string> mHashes;
    std::unordered_map<uint64_t, std::shared_ptr<Archive>> mFileToArchive;
};
} // namespace Ship
