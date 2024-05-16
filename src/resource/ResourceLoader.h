#pragma once

#include <memory>
#include <unordered_map>
#include "ResourceType.h"
#include "ResourceFactory.h"
#include "Resource.h"

namespace Ship {
struct File;

struct ResourceFactoryKey {
    uint32_t resourceFormat;
    uint32_t resourceType;
    uint32_t resourceVersion;

    bool operator==(const ResourceFactoryKey& o) const {
        return (resourceFormat == o.resourceFormat) && (resourceType == o.resourceType) &&
               (resourceVersion == o.resourceVersion);
    }
};

struct ResourceFactoryKeyHash {
    std::size_t operator()(const ResourceFactoryKey& key) const {
        return std::hash<int>()(key.resourceFormat) ^ std::hash<int>()(key.resourceType) ^
               std::hash<int>()(key.resourceVersion);
    }
};

class ResourceLoader {
  public:
    ResourceLoader();
    ~ResourceLoader();

    std::shared_ptr<Ship::IResource> LoadResource(std::shared_ptr<Ship::File> fileToLoad);
    bool RegisterResourceFactory(std::shared_ptr<ResourceFactory> factory, uint32_t format, std::string typeName,
                                 uint32_t type, uint32_t version);

    uint32_t GetResourceType(const std::string& type);

  protected:
    void RegisterGlobalResourceFactories();
    std::shared_ptr<ResourceFactory> GetFactory(uint32_t format, uint32_t type, uint32_t version);
    std::shared_ptr<ResourceFactory> GetFactory(uint32_t format, std::string typeName, uint32_t version);

  private:
    std::string DecodeASCII(uint32_t value);
    std::unordered_map<std::string, uint32_t> mResourceTypes;
    std::unordered_map<ResourceFactoryKey, std::shared_ptr<ResourceFactory>, ResourceFactoryKeyHash> mFactories;
};
} // namespace Ship
