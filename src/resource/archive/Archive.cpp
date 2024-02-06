#include "Archive.h"

#include "spdlog/spdlog.h"

#include "Context.h"
#include "resource/GameVersions.h"
#include "resource/File.h"
#include "resource/ResourceLoader.h"
#include "utils/binarytools/MemoryStream.h"
#include <StrHash64.h>

// TODO: Delete me and find an implementation
// Comes from stormlib. May not be the most efficient, but it's also important to be consistent.
// NOLINTNEXTLINE
extern bool SFileCheckWildCard(const char* szString, const char* szWildCard);

namespace LUS {
Archive::Archive(const std::string& path) : mGameVersion(UNKNOWN_GAME_VERSION), mPath(path), mIsLoaded(false) {
    mHashes = std::make_shared<std::unordered_map<uint64_t, std::string>>();
}

Archive::~Archive() {
    SPDLOG_TRACE("destruct archive: {}", GetPath());
}

void Archive::Load() {
    bool opened = LoadRaw();

    auto t = LoadFileRaw("version");
    bool isGameVersionValid = false;
    if (t != nullptr && t->IsLoaded) {
        auto stream = std::make_shared<MemoryStream>(t->Buffer->data(), t->Buffer->size());
        auto reader = std::make_shared<BinaryReader>(stream);
        LUS::Endianness endianness = (Endianness)reader->ReadUByte();
        reader->SetEndianness(endianness);
        SetGameVersion(reader->ReadUInt32());
        isGameVersionValid =
            Context::GetInstance()->GetResourceManager()->GetArchiveManager()->IsGameVersionValid(GetGameVersion());

        if (!isGameVersionValid) {
            SPDLOG_WARN("Attempting to load Archive \"{}\" with invalid version {}", GetPath(), GetGameVersion());
        }
    }

    SetLoaded(opened && isGameVersionValid);

    if (!IsLoaded()) {
        Unload();
    }
}

void Archive::Unload() {
    SetLoaded(false);
}

std::shared_ptr<std::unordered_map<uint64_t, std::string>> Archive::ListFiles() {
    return mHashes;
}

std::shared_ptr<std::unordered_map<uint64_t, std::string>> Archive::ListFiles(const std::string& filter) {
    auto result = std::make_shared<std::unordered_map<uint64_t, std::string>>();

    std::copy_if(mHashes->begin(), mHashes->end(), std::inserter(*result, result->begin()), 
    [filter](const std::pair<const int64_t, const std::string&> entry) {
        return SFileCheckWildCard(entry.second.c_str(), filter.c_str());
    });

    return result;
}

bool Archive::HasFile(const std::string& filePath) {
    return HasFile(CRC64(filePath.c_str()));
}

bool Archive::HasFile(uint64_t hash) {
    return mHashes->count(hash) > 0;
}

uint32_t Archive::GetGameVersion() {
    return mGameVersion;
}

const std::string& Archive::GetPath() {
    return mPath;
}

bool Archive::IsLoaded() {
    return mIsLoaded;
}

void Archive::SetLoaded(bool isLoaded) {
    mIsLoaded = isLoaded;
}

void Archive::SetGameVersion(uint32_t gameVersion) {
    mGameVersion = gameVersion;
}

void Archive::AddFile(const std::string& filePath) {
    (*mHashes)[CRC64(filePath.c_str())] = filePath;
}

std::shared_ptr<File> Archive::LoadFile(const std::string& filePath) {
    auto fileToLoad = LoadFileRaw(filePath);

    // Determine if file is binary or XML...
    if (fileToLoad->Buffer->at(0) == '<') {
        // File is XML
        // Read the xml document
        auto stream = std::make_shared<MemoryStream>(fileToLoad->Buffer);
        auto reader = fileToLoad->Reader = std::make_shared<BinaryReader>(stream);
        fileToLoad->XmlDocument = std::make_shared<tinyxml2::XMLDocument>();
        fileToLoad->XmlDocument->Parse(reader->ReadCString().data());
        if (fileToLoad->XmlDocument->Error()) {
            SPDLOG_ERROR("Failed to parse XML file {}. Error: {}", filePath, fileToLoad->XmlDocument->ErrorStr());
            return nullptr;
        }
        fileToLoad->InitData = ReadResourceInitDataXml(filePath, fileToLoad->XmlDocument);
    } else {
        // File is Binary
        auto fileToLoadMeta = LoadFileMeta(filePath);

        // Split out the header for reading.
        if (fileToLoadMeta != nullptr) {
            fileToLoad->Buffer = std::make_shared<std::vector<char>>(fileToLoad->Buffer->begin(),
                                                                     fileToLoad->Buffer->end());
        } else {
            auto headerBuffer = std::make_shared<std::vector<char>>(
                fileToLoad->Buffer->begin(), fileToLoad->Buffer->begin() + OTR_HEADER_SIZE);

            if (headerBuffer->size() < OTR_HEADER_SIZE) {
                SPDLOG_ERROR("Failed to parse ResourceInitData, buffer size too small. File: {}. Got {} bytes and "
                             "needed {} bytes.",
                             filePath, headerBuffer->size(), OTR_HEADER_SIZE);
                return nullptr;
            }

            fileToLoad->Buffer = std::make_shared<std::vector<char>>(fileToLoad->Buffer->begin() + OTR_HEADER_SIZE,
                                                                     fileToLoad->Buffer->end());

            // Create a reader for the header buffer
            auto headerStream = std::make_shared<MemoryStream>(headerBuffer);
            auto headerReader = std::make_shared<BinaryReader>(headerStream);
            fileToLoadMeta = ReadResourceInitDataBinary(filePath, headerReader);
        }

        // Create a reader for the data buffer
        auto stream = std::make_shared<MemoryStream>(fileToLoad->Buffer);
        fileToLoad->Reader = std::make_shared<BinaryReader>(stream);

        fileToLoad->InitData = fileToLoadMeta;
        fileToLoad->Reader->SetEndianness(fileToLoad->InitData->ByteOrder);
    }

    return fileToLoad;
}

std::shared_ptr<File> Archive::LoadFile(uint64_t hash) {
    return LoadFile(Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToString(hash));
}

std::shared_ptr<ResourceInitData> Archive::CreateDefaultResourceInitData() {
    auto resourceInitData = std::make_shared<ResourceInitData>();
    resourceInitData->Id = 0xDEADBEEFDEADBEEF;
    resourceInitData->Type = static_cast<uint32_t>(ResourceType::None);
    resourceInitData->ResourceVersion = -1;
    resourceInitData->IsCustom = false;
    resourceInitData->IsXml = false;
    resourceInitData->ByteOrder = Endianness::Native;
    return resourceInitData;
}

std::shared_ptr<ResourceInitData> Archive::ReadResourceInitDataBinary(const std::string& filePath,
                                                                      std::shared_ptr<BinaryReader> headerReader) {
    auto resourceInitData = CreateDefaultResourceInitData();
    resourceInitData->Path = filePath;

    if (headerReader == nullptr) {
        SPDLOG_ERROR("Error reading OTR header from XML: No header buffer document for file {}", filePath);
        return resourceInitData;
    }

    // OTR HEADER BEGIN
    // Byte Order
    resourceInitData->ByteOrder = (Endianness)headerReader->ReadInt8();
    headerReader->SetEndianness(resourceInitData->ByteOrder);
    // Is this asset custom?
    resourceInitData->IsCustom = (bool)headerReader->ReadInt8();
    // Unused two bytes
    for (int i = 0; i < 2; i++) {
        headerReader->ReadInt8();
    }
    // The type of the resource
    resourceInitData->Type = headerReader->ReadUInt32();
    // Resource version
    resourceInitData->ResourceVersion = headerReader->ReadUInt32();
    // Unique asset ID
    resourceInitData->Id = headerReader->ReadUInt64();
    // Reserved
    //reader->ReadUInt32();
    // ROM CRC
    //reader->ReadUInt64();
    // ROM Enum
    //reader->ReadUInt32();
    // Reserved for future file format versions...
    //reader->Seek(OTR_HEADER_SIZE, SeekOffsetType::Start);
    // OTR HEADER END

    headerReader->Seek(0, SeekOffsetType::Start);
    return resourceInitData;
}

std::shared_ptr<ResourceInitData> Archive::ReadResourceInitDataXml(const std::string& filePath,
                                                                   std::shared_ptr<tinyxml2::XMLDocument> document) {
    auto resourceInitData = CreateDefaultResourceInitData();
    resourceInitData->Path = filePath;

    if (document == nullptr) {
        SPDLOG_ERROR("Error reading OTR header from XML: No XML document for file {}", filePath);
        return resourceInitData;
    }

    // XML
    resourceInitData->IsCustom = true;
    resourceInitData->IsXml = true;

    auto root = document->FirstChildElement();
    resourceInitData->Type = mFactoriesTypes[root->Name()];
    resourceInitData->ResourceVersion = root->IntAttribute("Version");

    return resourceInitData;
}

} // namespace LUS
