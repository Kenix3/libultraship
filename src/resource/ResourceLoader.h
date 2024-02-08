#pragma once

#include <memory>
#include <unordered_map>
#include "ResourceType.h"
#include "ResourceFactory.h"
#include "Resource.h"

namespace LUS {
struct File;

class ResourceLoader {
  public:
    ResourceLoader();
    ~ResourceLoader();

    std::shared_ptr<IResource> LoadResource(std::shared_ptr<File> fileToLoad);
    bool RegisterResourceFactory(uint32_t resourceType, std::shared_ptr<ResourceFactory> factory);
    uint32_t GetVersionFromString(const std::string& version);

  protected:
    void RegisterGlobalResourceFactories();

  private:
    std::unordered_map<uint32_t, std::shared_ptr<ResourceFactory>> mFactories;
};
} // namespace LUS
