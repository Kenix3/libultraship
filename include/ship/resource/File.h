#pragma once

#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <stdint.h>
#include "ship/resource/ResourceType.h"
#include "ship/utils/binarytools/BinaryReader.h"

namespace tinyxml2 {
class XMLDocument;
class XMLElement;
} // namespace tinyxml2

namespace Ship {
class Archive;

/** @brief Format tag indicating the resource data is stored as raw binary. */
#define RESOURCE_FORMAT_BINARY 0
/** @brief Format tag indicating the resource data is stored as XML. */
#define RESOURCE_FORMAT_XML 1

/**
 * @brief Metadata that describes how a resource was (or should be) loaded.
 *
 * ResourceInitData is populated by ResourceLoader during the header-parsing phase
 * and is subsequently attached to every IResource instance. It is also used to
 * supply override metadata when loading a resource with non-default options.
 */
struct ResourceInitData {
    /** @brief The archive this resource was loaded from (may be null for loose files). */
    std::shared_ptr<Archive> Parent;
    /** @brief Virtual path of the resource within its archive or filesystem. */
    std::string Path;
    /** @brief Byte order of the raw data. */
    Endianness ByteOrder;
    /** @brief Resource type identifier (see ResourceType). */
    uint32_t Type;
    /** @brief Version of the resource format within its type family. */
    int32_t ResourceVersion;
    /** @brief Content-hash / unique identifier for this resource. */
    uint64_t Id;
    /** @brief True when this resource originated from an "alt assets" override path. */
    bool IsCustom;
    /** @brief Binary or XML format tag (RESOURCE_FORMAT_BINARY / RESOURCE_FORMAT_XML). */
    uint32_t Format;
};

/**
 * @brief Raw file buffer as read from an archive or the filesystem.
 *
 * A File object is the intermediate representation between the raw bytes stored
 * in an archive and the fully deserialized IResource. ResourceLoader reads the
 * File's buffer and forwards it to the appropriate ResourceFactory.
 */
struct File {
    /** @brief Raw byte buffer of the file contents. */
    std::shared_ptr<std::vector<char>> Buffer;
    /** @brief Byte offset into Buffer where the resource payload starts (after any header). */
    size_t BufferOffset = 0;
    /** @brief Parsed reader; either a BinaryReader or an XMLDocument, depending on the format. */
    std::variant<std::shared_ptr<tinyxml2::XMLDocument>, std::shared_ptr<BinaryReader>> Reader;
    /** @brief True once the file has been fully loaded from its backing store. */
    bool IsLoaded = false;
};
} // namespace Ship
