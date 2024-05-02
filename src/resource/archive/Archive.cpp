#include "Archive.h"

#include "spdlog/spdlog.h"

#include "Context.h"
#include "resource/GameVersions.h"
#include "resource/File.h"
#include "resource/ResourceLoader.h"
#include "utils/binarytools/MemoryStream.h"
#include "utils/glob.h"
#include "utils/StrHash64.h"
#include <nlohmann/json.hpp>

namespace Ship {
Archive::Archive(const std::string& path)
    : mHasGameVersion(false), mGameVersion(UNKNOWN_GAME_VERSION), mPath(path), mIsLoaded(false) {
    mHashes = std::make_shared<std::unordered_map<uint64_t, std::string>>();
}

Archive::~Archive() {
    SPDLOG_TRACE("destruct archive: {}", GetPath());
}

void Archive::Load() {
    bool opened = Open();

    auto t = LoadFileRaw("version");
    bool isGameVersionValid = false;
    if (t != nullptr && t->IsLoaded) {
        mHasGameVersion = true;
        auto stream = std::make_shared<MemoryStream>(t->Buffer->data(), t->Buffer->size());
        auto reader = std::make_shared<BinaryReader>(stream);
        Ship::Endianness endianness = (Endianness)reader->ReadUByte();
        reader->SetEndianness(endianness);
        SetGameVersion(reader->ReadUInt32());
        isGameVersionValid =
            Context::GetInstance()->GetResourceManager()->GetArchiveManager()->IsGameVersionValid(GetGameVersion());

        if (!isGameVersionValid) {
            SPDLOG_WARN("Attempting to load Archive \"{}\" with invalid version {}", GetPath(), GetGameVersion());
        }
    }

    SetLoaded(opened && (!mHasGameVersion || isGameVersionValid));

    if (!IsLoaded()) {
        Unload();
    }
}

void Archive::Unload() {
    Close();
    SetLoaded(false);
}

std::shared_ptr<std::unordered_map<uint64_t, std::string>> Archive::ListFiles() {
    return mHashes;
}

std::shared_ptr<std::unordered_map<uint64_t, std::string>> Archive::ListFiles(const std::string& filter) {
    auto result = std::make_shared<std::unordered_map<uint64_t, std::string>>();

    std::copy_if(mHashes->begin(), mHashes->end(), std::inserter(*result, result->begin()),
                 [filter](const std::pair<const int64_t, const std::string&> entry) {
                     return glob_match(filter.c_str(), entry.second.c_str());
                 });

    return result;
}

bool Archive::HasFile(const std::string& filePath) {
    return HasFile(CRC64(filePath.c_str()));
}

bool Archive::HasFile(uint64_t hash) {
    return mHashes->count(hash) > 0;
}

bool Archive::HasGameVersion() {
    return mHasGameVersion;
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

void Archive::IndexFile(const std::string& filePath) {
    if (filePath.length() > 5 && filePath.substr(filePath.length() - 5) == ".meta") {
        IndexFile(filePath.substr(0, filePath.length() - 5));
        return;
    }

    (*mHashes)[CRC64(filePath.c_str())] = filePath;
}

std::shared_ptr<Ship::ResourceInitData> Archive::ReadResourceInitData(const std::string& filePath,
                                                                      std::shared_ptr<Ship::File> metaFileToLoad) {
    auto initData = CreateDefaultResourceInitData();

    // just using metaFileToLoad->Buffer->data() leads to garbage at the end
    // that causes nlohmann to fail parsing, following the pattern used for
    // xml resolves that issue
    auto stream = std::make_shared<MemoryStream>(metaFileToLoad->Buffer);
    auto binaryReader = std::make_shared<BinaryReader>(stream);
    auto parsed = nlohmann::json::parse(binaryReader->ReadCString());

    if (parsed.contains("path")) {
        initData->Path = parsed["path"];
    } else {
        initData->Path = filePath;
    }

    auto formatString = parsed["format"];
    if (formatString == "XML") {
        initData->Format = RESOURCE_FORMAT_XML;
    }

    initData->Type = Context::GetInstance()->GetResourceManager()->GetResourceLoader()->GetResourceType(parsed["type"]);
    initData->ResourceVersion = parsed["version"];

    return initData;
}

std::shared_ptr<Ship::ResourceInitData> Archive::ReadResourceInitDataLegacy(const std::string& filePath,
                                                                            std::shared_ptr<Ship::File> fileToLoad) {
    // Determine if file is binary or XML...
    if (fileToLoad->Buffer->at(0) == '<') {
        // File is XML
        // Read the xml document
        auto stream = std::make_shared<MemoryStream>(fileToLoad->Buffer);
        auto binaryReader = std::make_shared<BinaryReader>(stream);

        auto xmlReader = std::make_shared<tinyxml2::XMLDocument>();

        xmlReader->Parse(binaryReader->ReadCString().data());
        if (xmlReader->Error()) {
            SPDLOG_ERROR("Failed to parse XML file {}. Error: {}", filePath, xmlReader->ErrorStr());
            return nullptr;
        }
        return ReadResourceInitDataXml(filePath, xmlReader);
    } else {
        auto headerBuffer = std::make_shared<std::vector<char>>(fileToLoad->Buffer->begin(),
                                                                fileToLoad->Buffer->begin() + OTR_HEADER_SIZE);

        if (headerBuffer->size() < OTR_HEADER_SIZE) {
            SPDLOG_ERROR("Failed to parse ResourceInitData, buffer size too small. File: {}. Got {} bytes and "
                         "needed {} bytes.",
                         filePath, headerBuffer->size(), OTR_HEADER_SIZE);
            return nullptr;
        }

        // Factories expect the buffer to not include the header,
        // so we need to remove it from the buffer on the file
        fileToLoad->Buffer = std::make_shared<std::vector<char>>(fileToLoad->Buffer->begin() + OTR_HEADER_SIZE,
                                                                 fileToLoad->Buffer->end());

        // Create a reader for the header buffer
        auto headerStream = std::make_shared<MemoryStream>(headerBuffer);
        auto headerReader = std::make_shared<BinaryReader>(headerStream);
        return ReadResourceInitDataBinary(filePath, headerReader);
    }
}

std::shared_ptr<Ship::BinaryReader> Archive::CreateBinaryReader(std::shared_ptr<Ship::File> fileToLoad) {
    auto stream = std::make_shared<MemoryStream>(fileToLoad->Buffer);
    auto reader = std::make_shared<BinaryReader>(stream);
    reader->SetEndianness(fileToLoad->InitData->ByteOrder);
    return reader;
}

std::shared_ptr<tinyxml2::XMLDocument> Archive::CreateXMLReader(std::shared_ptr<Ship::File> fileToLoad) {
    auto stream = std::make_shared<MemoryStream>(fileToLoad->Buffer);
    auto binaryReader = std::make_shared<BinaryReader>(stream);

    auto xmlReader = std::make_shared<tinyxml2::XMLDocument>();

    xmlReader->Parse(binaryReader->ReadCString().data());
    if (xmlReader->Error()) {
        SPDLOG_ERROR("Failed to parse XML file {}. Error: {}", fileToLoad->InitData->Path, xmlReader->ErrorStr());
        return nullptr;
    }

    return xmlReader;
}

std::shared_ptr<Ship::File> Archive::LoadFile(const std::string& filePath,
                                              std::shared_ptr<Ship::ResourceInitData> initData) {
    std::shared_ptr<Ship::File> fileToLoad = nullptr;

    if (initData != nullptr) {
        fileToLoad = LoadFileRaw(filePath);
        fileToLoad->InitData = initData;
    } else {
        auto metaFilePath = filePath + ".meta";
        auto metaFileToLoad = LoadFileRaw(metaFilePath);

        if (metaFileToLoad != nullptr) {
            auto initDataFromMetaFile = ReadResourceInitData(filePath, metaFileToLoad);
            fileToLoad = LoadFileRaw(initDataFromMetaFile->Path);
            fileToLoad->InitData = initDataFromMetaFile;
        } else {
            fileToLoad = LoadFileRaw(filePath);
            fileToLoad->InitData = ReadResourceInitDataLegacy(filePath, fileToLoad);
        }
    }

    if (fileToLoad == nullptr) {
        SPDLOG_ERROR("Failed to load file at path {}.", filePath);
        return nullptr;
    }

    switch (fileToLoad->InitData->Format) {
        case RESOURCE_FORMAT_BINARY:
            fileToLoad->Reader = CreateBinaryReader(fileToLoad);
            break;
        case RESOURCE_FORMAT_XML:
            fileToLoad->Reader = CreateXMLReader(fileToLoad);
            break;
    }

    return fileToLoad;
}

std::shared_ptr<Ship::File> Archive::LoadFile(uint64_t hash, std::shared_ptr<Ship::ResourceInitData> initData) {
    const std::string& filePath =
        *Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToString(hash);
    return LoadFile(filePath, initData);
}

std::shared_ptr<Ship::ResourceInitData> Archive::CreateDefaultResourceInitData() {
    auto resourceInitData = std::make_shared<ResourceInitData>();
    resourceInitData->Id = 0xDEADBEEFDEADBEEF;
    resourceInitData->Type = static_cast<uint32_t>(ResourceType::None);
    resourceInitData->ResourceVersion = -1;
    resourceInitData->IsCustom = false;
    resourceInitData->Format = RESOURCE_FORMAT_BINARY;
    resourceInitData->ByteOrder = Endianness::Native;
    return resourceInitData;
}

std::shared_ptr<Ship::ResourceInitData>
Archive::ReadResourceInitDataBinary(const std::string& filePath, std::shared_ptr<Ship::BinaryReader> headerReader) {
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
    // reader->ReadUInt32();
    // ROM CRC
    // reader->ReadUInt64();
    // ROM Enum
    // reader->ReadUInt32();
    // Reserved for future file format versions...
    // reader->Seek(OTR_HEADER_SIZE, SeekOffsetType::Start);
    // OTR HEADER END

    headerReader->Seek(0, SeekOffsetType::Start);
    return resourceInitData;
}

std::shared_ptr<Ship::ResourceInitData>
Archive::ReadResourceInitDataXml(const std::string& filePath, std::shared_ptr<tinyxml2::XMLDocument> document) {
    auto resourceInitData = CreateDefaultResourceInitData();
    resourceInitData->Path = filePath;

    if (document == nullptr) {
        SPDLOG_ERROR("Error reading OTR header from XML: No XML document for file {}", filePath);
        return resourceInitData;
    }

    // XML
    resourceInitData->IsCustom = true;
    resourceInitData->Format = RESOURCE_FORMAT_XML;

    auto root = document->FirstChildElement();
    resourceInitData->Type =
        Context::GetInstance()->GetResourceManager()->GetResourceLoader()->GetResourceType(root->Name());
    resourceInitData->ResourceVersion = root->IntAttribute("Version");

    return resourceInitData;
}

} // namespace Ship
