#pragma once

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <tinyxml2.h>
#include "utils/binarytools/BinaryReader.h"

namespace LUS {
#define OTR_HEADER_SIZE ((size_t)64)

struct File;
struct ResourceInitData;

class Archive {
    friend class ArchiveManager;

  public:
    Archive(const std::string& path);
    ~Archive();

    void Load();
    void Unload();

    virtual std::shared_ptr<File> LoadFile(const std::string& filePath);
    virtual std::shared_ptr<File> LoadFile(uint64_t hash);
    std::shared_ptr<std::unordered_map<uint64_t, std::string>> ListFiles();
    std::shared_ptr<std::unordered_map<uint64_t, std::string>> ListFiles(const std::string& filter);
    bool HasFile(const std::string& filePath);
    bool HasFile(uint64_t hash);
    bool HasGameVersion();
    uint32_t GetGameVersion();
    const std::string& GetPath();
    bool IsLoaded();

    virtual bool LoadRaw() = 0;
    virtual bool UnloadRaw() = 0;
    virtual std::shared_ptr<File> LoadFileRaw(const std::string& filePath) = 0;
    virtual std::shared_ptr<File> LoadFileRaw(uint64_t hash) = 0;

  protected:
    static std::shared_ptr<ResourceInitData> ReadResourceInitDataBinary(const std::string& filePath,
                                                                        std::shared_ptr<BinaryReader> headerReader);
    static std::shared_ptr<ResourceInitData> ReadResourceInitDataXml(const std::string& filePath,
                                                                     std::shared_ptr<tinyxml2::XMLDocument> document);
    virtual std::shared_ptr<ResourceInitData> LoadFileMeta(const std::string& filePath) = 0;
    virtual std::shared_ptr<ResourceInitData> LoadFileMeta(uint64_t hash) = 0;

    void SetLoaded(bool isLoaded);
    void SetGameVersion(uint32_t gameVersion);
    void AddFile(const std::string& filePath);

  private:
    static std::shared_ptr<ResourceInitData> CreateDefaultResourceInitData();

    bool mIsLoaded;
    bool mHasGameVersion;
    uint32_t mGameVersion;
    std::string mPath;
    std::shared_ptr<std::unordered_map<uint64_t, std::string>> mHashes;
};
} // namespace LUS
