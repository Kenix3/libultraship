#pragma once

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <tinyxml2.h>
#include "utils/binarytools/BinaryReader.h"

namespace ShipDK {
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

    virtual std::shared_ptr<ShipDK::File> LoadFile(const std::string& filePath,
                                                   std::shared_ptr<ShipDK::ResourceInitData> initData = nullptr);
    virtual std::shared_ptr<ShipDK::File> LoadFile(uint64_t hash,
                                                   std::shared_ptr<ShipDK::ResourceInitData> initData = nullptr);
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
    virtual std::shared_ptr<ShipDK::File> LoadFileRaw(const std::string& filePath) = 0;
    virtual std::shared_ptr<ShipDK::File> LoadFileRaw(uint64_t hash) = 0;

  private:
    static std::shared_ptr<ShipDK::ResourceInitData> CreateDefaultResourceInitData();
    std::shared_ptr<ShipDK::ResourceInitData> ReadResourceInitData(const std::string& filePath,
                                                                   std::shared_ptr<ShipDK::File> metaFileToLoad);
    std::shared_ptr<ShipDK::ResourceInitData> ReadResourceInitDataLegacy(const std::string& filePath,
                                                                         std::shared_ptr<ShipDK::File> fileToLoad);
    static std::shared_ptr<ShipDK::ResourceInitData>
    ReadResourceInitDataBinary(const std::string& filePath, std::shared_ptr<ShipDK::BinaryReader> headerReader);
    static std::shared_ptr<ShipDK::ResourceInitData>
    ReadResourceInitDataXml(const std::string& filePath, std::shared_ptr<tinyxml2::XMLDocument> document);
    std::shared_ptr<ShipDK::BinaryReader> CreateBinaryReader(std::shared_ptr<ShipDK::File> fileToLoad);
    std::shared_ptr<tinyxml2::XMLDocument> CreateXMLReader(std::shared_ptr<ShipDK::File> fileToLoad);

    bool mIsLoaded;
    bool mHasGameVersion;
    uint32_t mGameVersion;
    std::string mPath;
    std::shared_ptr<std::unordered_map<uint64_t, std::string>> mHashes;
};
} // namespace ShipDK
