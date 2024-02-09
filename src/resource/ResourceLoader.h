#pragma once

#include <memory>
#include <unordered_map>
#include "ResourceType.h"
#include "ResourceFactory.h"
#include "Resource.h"

namespace LUS {
struct File;

struct ResourceFactoryKey {
  uint32_t resourceFormat;
  uint32_t resourceType;
  uint32_t resourceVersion;
};

class ResourceLoader {
  public:
    ResourceLoader();
    ~ResourceLoader();

    std::shared_ptr<IResource> LoadResource(std::shared_ptr<File> fileToLoad);
    bool RegisterResourceFactory(std::shared_ptr<ResourceFactory> factory, uint32_t format, std::string typeName, uint32_t type, uint32_t version);

  protected:
    void RegisterGlobalResourceFactories();

  private:
    uint32_t GetResourceType(const std::string& type);
    std::unordered_map<std::string, uint32_t> mResourceTypes;
    std::unordered_map<ResourceFactoryKey, std::shared_ptr<ResourceFactory>> mFactories;
};
} // namespace LUS
