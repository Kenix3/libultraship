#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdint.h>
#include <tinyxml2.h>
#include "resource/ResourceType.h"
#include "utils/binarytools/BinaryReader.h"

namespace LUS {
class Archive;

#define RESOURCE_FORMAT_BINARY 0
#define RESOURCE_FORMAT_XML 1

struct ResourceInitData {
    std::string Path;
    Endianness ByteOrder;
    uint32_t Type;
    int32_t ResourceVersion;
    uint64_t Id;
    bool IsCustom;
    uint32_t Format;
};

struct File {
    std::shared_ptr<Archive> Parent;
    std::shared_ptr<ResourceInitData> InitData;
    std::shared_ptr<std::vector<char>> Buffer;
    std::shared_ptr<tinyxml2::XMLDocument> XmlDocument;
    std::shared_ptr<BinaryReader> Reader;
    bool IsLoaded = false;
};
} // namespace LUS
