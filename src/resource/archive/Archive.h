#pragma once

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <tinyxml2.h>
#include "utils/binarytools/BinaryReader.h"

namespace Ship {
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

    virtual std::shared_ptr<File> LoadFile(const std::string& filePath,
                                           std::shared_ptr<ResourceInitData> initData = nullptr);
    virtual std::shared_ptr<File> LoadFile(uint64_t hash, std::shared_ptr<ResourceInitData> initData = nullptr);
    std::shared_ptr<std::unordered_map<uint64_t, std::string>> ListFiles();
    std::shared_ptr<std::unordered_map<uint64_t, std::string>> ListFiles(const std::string& filter);
    bool HasFile(const std::string& filePath);
    bool HasFile(uint64_t hash);
    bool HasGameVersion();
    uint32_t GetGameVersion();
    const std::string& GetPath();
    bool IsLoaded();

    virtual bool Open() = 0;
    virtual bool Close() = 0;

  protected:
    void SetLoaded(bool isLoaded);
    void SetGameVersion(uint32_t gameVersion);
    void IndexFile(const std::string& filePath);
    virtual std::shared_ptr<File> LoadFileRaw(const std::string& filePath) = 0;
    virtual std::shared_ptr<File> LoadFileRaw(uint64_t hash) = 0;

  private:
    static std::shared_ptr<ResourceInitData> CreateDefaultResourceInitData();
    std::shared_ptr<ResourceInitData> ReadResourceInitData(const std::string& filePath,
                                                           std::shared_ptr<File> metaFileToLoad);
    std::shared_ptr<ResourceInitData> ReadResourceInitDataLegacy(const std::string& filePath,
                                                                 std::shared_ptr<File> fileToLoad);
    static std::shared_ptr<ResourceInitData> ReadResourceInitDataBinary(const std::string& filePath,
                                                                        std::shared_ptr<BinaryReader> headerReader);
    static std::shared_ptr<ResourceInitData> ReadResourceInitDataXml(const std::string& filePath,
                                                                     std::shared_ptr<tinyxml2::XMLDocument> document);
    std::shared_ptr<BinaryReader> CreateBinaryReader(std::shared_ptr<File> fileToLoad);
    std::shared_ptr<tinyxml2::XMLDocument> CreateXMLReader(std::shared_ptr<File> fileToLoad);

    bool mIsLoaded;
    bool mHasGameVersion;
    uint32_t mGameVersion;
    std::string mPath;
    std::shared_ptr<std::unordered_map<uint64_t, std::string>> mHashes;
};
} // namespace Ship
