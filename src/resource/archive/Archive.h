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

class Archive : public std::enable_shared_from_this<Archive> {
    friend class ArchiveManager;

  public:
    Archive(const std::string& path);
    ~Archive();

    bool operator==(const Archive& rhs) const;

    void Load();
    void Unload();

    virtual std::shared_ptr<File> LoadFile(const std::string& filePath) = 0;
    virtual std::shared_ptr<File> LoadFile(uint64_t hash) = 0;
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
    virtual bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data) = 0;

  protected:
    void SetLoaded(bool isLoaded);
    void SetGameVersion(uint32_t gameVersion);
    void IndexFile(const std::string& filePath);

  private:
    bool mIsLoaded;
    bool mHasGameVersion;
    uint32_t mGameVersion;
    std::string mPath;
    std::shared_ptr<std::unordered_map<uint64_t, std::string>> mHashes;
};
} // namespace Ship
