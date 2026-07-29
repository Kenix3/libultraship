#include "ship/resource/ResourceLoader.h"
#include "ship/resource/ResourceFactory.h"
#include "ship/resource/ResourceManager.h"
#include "ship/resource/Resource.h"
#include "ship/resource/File.h"
#include "ship/Context.h"
#include "ship/utils/binarytools/MemoryStream.h"
#include "ship/utils/binarytools/BinaryReader.h"
#include "ship/resource/factory/JsonFactory.h"
#include "ship/resource/factory/ShaderFactory.h"
#include <spdlog/spdlog.h>
#include <tinyxml2.h>
#include "nlohmann/json.hpp"

namespace Ship {
ResourceLoader::ResourceLoader() {
    RegisterGlobalResourceFactories();
}

ResourceLoader::~ResourceLoader() {
    SPDLOG_TRACE("destruct resource loader");
}

void ResourceLoader::RegisterGlobalResourceFactories() {
    RegisterResourceFactory(std::make_shared<ResourceFactoryBinaryJsonV0>(), RESOURCE_FORMAT_BINARY, "Json",
                            static_cast<uint32_t>(ResourceType::Json), 0);
    RegisterResourceFactory(std::make_shared<ResourceFactoryBinaryShaderV0>(), RESOURCE_FORMAT_BINARY, "Shader",
                            static_cast<uint32_t>(ResourceType::Shader), 0);
}

bool ResourceLoader::RegisterResourceFactory(std::shared_ptr<ResourceFactory> factory, uint32_t format,
                                             std::string typeName, uint32_t type, uint32_t version) {
    if (mResourceTypes.contains(typeName)) {
        if (mResourceTypes[typeName] != type) {
            SPDLOG_ERROR("Failed to register resource factory: conflicting types for name {}", typeName);
            return false;
        }
    } else {
        mResourceTypes[typeName] = type;
    }

    ResourceFactoryKey key{ .resourceFormat = format, .resourceType = type, .resourceVersion = version };
    if (mFactories.contains(key)) {
        SPDLOG_ERROR("Failed to register resource factory: factory with key {}{}{} already exists", format, type,
                     version);
        return false;
    }
    mFactories[key] = factory;

    return true;
}

std::string ResourceLoader::DecodeASCII(uint32_t value) {
    std::string str(4, '\0'); // Initialize the string with 4 null characters

    str[0] = (value >> 24) & 0xFF;
    str[1] = (value >> 16) & 0xFF;
    str[2] = (value >> 8) & 0xFF;
    str[3] = value & 0xFF;

    return str;
}

std::shared_ptr<ResourceFactory> ResourceLoader::GetFactory(uint32_t format, uint32_t type, uint32_t version) {
    ResourceFactoryKey key{ .resourceFormat = format, .resourceType = type, .resourceVersion = version };
    if (!mFactories.contains(key)) {
        SPDLOG_ERROR("GetFactory failed to find an import factory for resource of type {} as hex: 0x{:X}. Format: {}, "
                     "Version: {}",
                     DecodeASCII(type), type, format, version);
        return nullptr;
    }

    return mFactories[key];
}

std::shared_ptr<ResourceFactory> ResourceLoader::GetFactory(uint32_t format, std::string typeName, uint32_t version) {
    if (!mResourceTypes.contains(typeName)) {
        SPDLOG_ERROR("Could not find resource type for name {}", typeName);
        return nullptr;
    }

    return GetFactory(format, mResourceTypes[typeName], version);
}

std::shared_ptr<ResourceInitData> ResourceLoader::ReadResourceInitDataLegacy(const std::string& filePath,
                                                                             std::shared_ptr<File> fileToLoad) {
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
        if (fileToLoad->Buffer->size() < OTR_HEADER_SIZE) {
            SPDLOG_ERROR("Failed to parse ResourceInitData, buffer size too small. File: {}. Got {} bytes and "
                         "needed {} bytes.",
                         filePath, fileToLoad->Buffer->size(), OTR_HEADER_SIZE);
            return nullptr;
        }

        // Record where the body starts so CreateBinaryReader can skip the header
        // without copying the buffer.
        fileToLoad->BufferOffset = OTR_HEADER_SIZE;

        // Read the header from the start of the buffer (no copy needed).
        auto headerStream = std::make_shared<MemoryStream>(fileToLoad->Buffer);
        auto headerReader = std::make_shared<BinaryReader>(headerStream);
        return ReadResourceInitDataBinary(filePath, headerReader);
    }
}

std::shared_ptr<BinaryReader> ResourceLoader::CreateBinaryReader(std::shared_ptr<File> fileToLoad,
                                                                 std::shared_ptr<ResourceInitData> initData) {
    auto stream = std::make_shared<MemoryStream>(fileToLoad->Buffer, fileToLoad->BufferOffset);
    auto reader = std::make_shared<BinaryReader>(stream);
    reader->SetEndianness(initData->ByteOrder);
    return reader;
}

std::shared_ptr<tinyxml2::XMLDocument> ResourceLoader::CreateXMLReader(std::shared_ptr<File> fileToLoad,
                                                                       std::shared_ptr<ResourceInitData> initData) {
    auto stream = std::make_shared<MemoryStream>(fileToLoad->Buffer);
    auto binaryReader = std::make_shared<BinaryReader>(stream);

    auto xmlReader = std::make_shared<tinyxml2::XMLDocument>();

    xmlReader->Parse(binaryReader->ReadCString().data());
    if (xmlReader->Error()) {
        SPDLOG_ERROR("Failed to parse XML file {}. Error: {}", initData->Path, xmlReader->ErrorStr());
        return nullptr;
    }

    return xmlReader;
}

std::shared_ptr<ResourceInitData> ResourceLoader::CreateDefaultResourceInitData() {
    auto resourceInitData = std::make_shared<ResourceInitData>();
    resourceInitData->Id = 0xDEADBEEFDEADBEEF;
    resourceInitData->Type = static_cast<uint32_t>(ResourceType::None);
    resourceInitData->ResourceVersion = -1;
    resourceInitData->IsCustom = false;
    resourceInitData->Format = RESOURCE_FORMAT_BINARY;
    resourceInitData->ByteOrder = Endianness::Native;
    return resourceInitData;
}

std::shared_ptr<ResourceInitData> ResourceLoader::ReadResourceInitData(const std::string& filePath,
                                                                       std::shared_ptr<File> metaFileToLoad) {
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

    initData->Type =
        Context::GetRawInstance()->GetResourceManager()->GetResourceLoader()->GetResourceType(parsed["type"]);
    initData->ResourceVersion = parsed["version"];

    return initData;
}

// Position File::BufferOffset at the resource body. A currently-headed binary resource keeps an OTR
// header ahead of its body; when init data was supplied externally (a caller or a `.meta`) rather
// than parsed from that header, skip past it. Headerless buffers (a shader, or any asset once
// Kenix3/libultraship#1160 removes the header) have no valid byte order / matching type and are left
// at offset 0. Remove this and its call sites along with the header under #1160.
static void SetBufferOffset(const std::shared_ptr<File>& file, const std::shared_ptr<ResourceInitData>& initData) {
    if (file == nullptr || initData->Format != RESOURCE_FORMAT_BINARY || file->Buffer->size() < OTR_HEADER_SIZE) {
        return;
    }
    auto reader = std::make_shared<BinaryReader>(std::make_shared<MemoryStream>(file->Buffer));
    auto byteOrder = (Endianness)reader->ReadInt8();
    if (byteOrder != Endianness::Little && byteOrder != Endianness::Big) {
        return;
    }
    reader->SetEndianness(byteOrder);
    reader->ReadInt8(); // isCustom
    reader->ReadInt8(); // reserved
    reader->ReadInt8(); // reserved
    if (reader->ReadUInt32() == initData->Type) {
        file->BufferOffset = OTR_HEADER_SIZE;
    }
}

std::shared_ptr<ResourceInitData> ResourceLoader::ResolveMetaAlias(const std::string& filePath,
                                                                   std::shared_ptr<File>& fileToLoad) {
    auto resourceManager = Context::GetRawInstance()->GetResourceManager();
    auto metaFileToLoad = resourceManager->LoadFileProcess(filePath + ".meta");
    if (metaFileToLoad == nullptr) {
        return nullptr;
    }

    auto metaInitData = ReadResourceInitData(filePath, metaFileToLoad);
    auto aliasedFileToLoad = resourceManager->LoadFileProcess(metaInitData->Path);
    if (aliasedFileToLoad == nullptr) {
        return nullptr;
    }

    // The alias wins only if its target lives in an equal-or-higher priority archive than the
    // real asset at filePath (ties go to the alias; a missing real asset reports priority -1).
    auto archiveManager = resourceManager->GetArchiveManager();
    int32_t realPriority = archiveManager->GetFilePriority(filePath);
    int32_t aliasPriority = archiveManager->GetFilePriority(metaInitData->Path);
    if (aliasPriority < realPriority) {
        return nullptr;
    }

    fileToLoad = aliasedFileToLoad;
    return metaInitData;
}

std::shared_ptr<IResource> ResourceLoader::LoadResource(std::string filePath, std::shared_ptr<File> fileToLoad,
                                                        std::shared_ptr<ResourceInitData> initData) {
    // fileToLoad is the highest-priority real asset at filePath, or null when the resource
    // exists only as a `.meta` alias. Prefer a winning alias, else read the real asset's header.
    bool legacyInitData = false;
    if (initData == nullptr) {
        initData = ResolveMetaAlias(filePath, fileToLoad);
    }
    if (initData == nullptr && fileToLoad != nullptr) {
        initData = ReadResourceInitDataLegacy(filePath, fileToLoad);
        legacyInitData = true;
    }

    if (fileToLoad == nullptr) {
        SPDLOG_ERROR("Failed to load file at path {}.", filePath);
        return nullptr;
    }

    switch (initData->Format) {
        case RESOURCE_FORMAT_BINARY:
            if (!legacyInitData) {
                SetBufferOffset(fileToLoad, initData);
            }
            fileToLoad->Reader = CreateBinaryReader(fileToLoad, initData);
            break;
        case RESOURCE_FORMAT_XML:
            fileToLoad->Reader = CreateXMLReader(fileToLoad, initData);
            break;
    }

    // initData->Parent = shared_from_this();

    auto factory = GetFactory(initData->Format, initData->Type, initData->ResourceVersion);
    if (factory == nullptr) {
        SPDLOG_ERROR("LoadResource failed to find factory for the resource at path: {}", initData->Path);
        return nullptr;
    }

    return factory->ReadResource(fileToLoad, initData);
}

uint32_t ResourceLoader::GetResourceType(const std::string& type) {
    return mResourceTypes.contains(type) ? mResourceTypes[type] : static_cast<uint32_t>(ResourceType::None);
}

std::shared_ptr<ResourceInitData>
ResourceLoader::ReadResourceInitDataBinary(const std::string& filePath, std::shared_ptr<BinaryReader> headerReader) {
    auto resourceInitData = CreateDefaultResourceInitData();
    resourceInitData->Path = filePath;

    if (headerReader == nullptr) {
        SPDLOG_ERROR("Error reading OTR header from Binary: No header buffer document for file {}", filePath);
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

std::shared_ptr<ResourceInitData>
ResourceLoader::ReadResourceInitDataXml(const std::string& filePath, std::shared_ptr<tinyxml2::XMLDocument> document) {
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
        Context::GetRawInstance()->GetResourceManager()->GetResourceLoader()->GetResourceType(root->Name());
    resourceInitData->ResourceVersion = root->IntAttribute("Version");

    return resourceInitData;
}
} // namespace Ship
