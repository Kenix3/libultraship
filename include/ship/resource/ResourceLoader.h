#pragma once

#include <memory>
#include <unordered_map>
#include "ResourceType.h"
#include <ship/resource/ResourceFactory.h>
#include <ship/resource/Resource.h>

namespace Ship {
struct File;

/**
 * @brief Cache key that uniquely identifies a ResourceFactory by format, type, and version.
 *
 * The three-field key allows multiple factories to be registered for the same resource
 * type (e.g. different binary versions or an XML variant of the same type).
 */
struct ResourceFactoryKey {
    /** @brief Format tag (RESOURCE_FORMAT_BINARY or RESOURCE_FORMAT_XML). */
    uint32_t resourceFormat;
    /** @brief Numeric resource type identifier (see ResourceType). */
    uint32_t resourceType;
    /** @brief Version of the resource format this factory handles. */
    uint32_t resourceVersion;

    bool operator==(const ResourceFactoryKey& o) const {
        return (resourceFormat == o.resourceFormat) && (resourceType == o.resourceType) &&
               (resourceVersion == o.resourceVersion);
    }
};

/** @brief std::hash specialization for ResourceFactoryKey, used by the factory map. */
struct ResourceFactoryKeyHash {
    std::size_t operator()(const ResourceFactoryKey& key) const {
        return std::hash<int>()(key.resourceFormat) ^ std::hash<int>()(key.resourceType) ^
               std::hash<int>()(key.resourceVersion);
    }
};

/**
 * @brief Parses raw file data into IResource objects using registered ResourceFactory instances.
 *
 * ResourceLoader sits between the archive layer and the ResourceManager. Given a File and
 * its associated ResourceInitData, it selects the appropriate ResourceFactory (matched by
 * format, type, and version) and delegates deserialization to that factory.
 *
 * New resource types are registered at startup via RegisterResourceFactory(). Built-in
 * factories (Blob, JSON, Shader) are registered automatically in RegisterGlobalResourceFactories().
 */
class ResourceLoader {
  public:
    ResourceLoader();
    virtual ~ResourceLoader();

    /**
     * @brief Deserializes a File into a fully constructed IResource.
     *
     * Reads the resource header from @p fileToLoad, selects the matching factory,
     * and calls ResourceFactory::ReadResource().
     *
     * @param filePath   Virtual path (used for logging and to populate ResourceInitData).
     * @param fileToLoad Raw file buffer as returned by the archive layer.
     * @param initData   Optional metadata overrides; pass nullptr to infer from the header.
     * @return Deserialized IResource, or nullptr if no matching factory was found or parsing failed.
     */
    std::shared_ptr<IResource> LoadResource(std::string filePath, std::shared_ptr<File> fileToLoad,
                                            std::shared_ptr<ResourceInitData> initData = nullptr);

    /**
     * @brief Registers a factory so that ResourceLoader can handle a new resource type.
     *
     * @param factory  The factory implementation to register.
     * @param format   Format tag (RESOURCE_FORMAT_BINARY / RESOURCE_FORMAT_XML).
     * @param typeName Human-readable type name string (e.g. "Texture").
     * @param type     Numeric type identifier.
     * @param version  Resource format version this factory handles.
     * @return true if registration succeeded; false if a factory with the same key already exists.
     */
    bool RegisterResourceFactory(std::shared_ptr<ResourceFactory> factory, uint32_t format, std::string typeName,
                                 uint32_t type, uint32_t version);

    /**
     * @brief Looks up the numeric type ID for a given type name string.
     * @param type Human-readable type name (e.g. "Texture").
     * @return Numeric type ID, or 0 if the type name is not registered.
     */
    uint32_t GetResourceType(const std::string& type);

  protected:
    /** @brief Registers the built-in factories (Blob, JSON, Shader). Called during construction. */
    void RegisterGlobalResourceFactories();

    /**
     * @brief Returns the factory registered for the given (format, type, version) triple.
     * @return Matching factory, or nullptr if none is registered.
     */
    std::shared_ptr<ResourceFactory> GetFactory(uint32_t format, uint32_t type, uint32_t version);

    /**
     * @brief Returns the factory registered for the given (format, typeName, version) triple.
     * @return Matching factory, or nullptr if none is registered.
     */
    std::shared_ptr<ResourceFactory> GetFactory(uint32_t format, std::string typeName, uint32_t version);

    /**
     * @brief Reads and parses the resource header from the meta-file to produce ResourceInitData.
     * @param filePath       Virtual path (for error messages).
     * @param metaFileToLoad File containing the header (may be a separate ".meta" sidecar).
     * @return Populated ResourceInitData, or nullptr if the header is invalid.
     */
    std::shared_ptr<ResourceInitData> ReadResourceInitData(const std::string& filePath,
                                                           std::shared_ptr<File> metaFileToLoad);

    /** @brief Creates a ResourceInitData with default/zeroed fields. */
    static std::shared_ptr<ResourceInitData> CreateDefaultResourceInitData();

    /**
     * @brief Reads a legacy (pre-versioned) binary resource header.
     * @param filePath   Virtual path (for error messages).
     * @param fileToLoad File containing the legacy header.
     * @return Populated ResourceInitData, or nullptr on failure.
     */
    std::shared_ptr<ResourceInitData> ReadResourceInitDataLegacy(const std::string& filePath,
                                                                 std::shared_ptr<File> fileToLoad);

    /**
     * @brief Reads a standard binary resource header from a BinaryReader.
     * @param filePath     Virtual path (for error messages).
     * @param headerReader Reader positioned at the start of the binary header.
     * @return Populated ResourceInitData, or nullptr on failure.
     */
    static std::shared_ptr<ResourceInitData> ReadResourceInitDataBinary(const std::string& filePath,
                                                                        std::shared_ptr<BinaryReader> headerReader);

    /**
     * @brief Reads an XML resource header from an XMLDocument.
     * @param filePath Virtual path (for error messages).
     * @param document Parsed XML document containing the header.
     * @return Populated ResourceInitData, or nullptr on failure.
     */
    static std::shared_ptr<ResourceInitData> ReadResourceInitDataXml(const std::string& filePath,
                                                                     std::shared_ptr<tinyxml2::XMLDocument> document);

    /**
     * @brief Reads a PNG image header to produce ResourceInitData.
     * @param filePath     Virtual path (for error messages).
     * @param headerReader Reader positioned at the start of the PNG file.
     * @return Populated ResourceInitData, or nullptr on failure.
     */
    static std::shared_ptr<ResourceInitData> ReadResourceInitDataPng(const std::string& filePath,
                                                                     std::shared_ptr<BinaryReader> headerReader);

    /**
     * @brief Creates a BinaryReader wrapping the buffer of a loaded File.
     * @param fileToLoad File whose buffer should be wrapped.
     * @param initData   Init data used to configure byte-order.
     * @return Configured BinaryReader.
     */
    std::shared_ptr<BinaryReader> CreateBinaryReader(std::shared_ptr<File> fileToLoad,
                                                     std::shared_ptr<ResourceInitData> initData);

    /**
     * @brief Parses the buffer of a loaded File as XML and returns the resulting document.
     * @param fileToLoad File whose buffer should be parsed.
     * @param initData   Init data (unused currently, reserved for future use).
     * @return Parsed XMLDocument, or nullptr if parsing failed.
     */
    std::shared_ptr<tinyxml2::XMLDocument> CreateXMLReader(std::shared_ptr<File> fileToLoad,
                                                           std::shared_ptr<ResourceInitData> initData);

  private:
    std::string DecodeASCII(uint32_t value);
    std::unordered_map<std::string, uint32_t> mResourceTypes;
    std::unordered_map<ResourceFactoryKey, std::shared_ptr<ResourceFactory>, ResourceFactoryKeyHash> mFactories;
};
} // namespace Ship
