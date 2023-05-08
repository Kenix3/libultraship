#pragma once

#include <memory>
#include <unordered_map>
#include "ResourceType.h"
#include "ResourceFactory.h"
#include "Resource.h"

namespace LUS {
class Context;
struct File;

class ResourceLoader {
  public:
    ResourceLoader(std::shared_ptr<Context> context);

    std::shared_ptr<Context> GetContext();
    std::shared_ptr<Resource> LoadResource(std::shared_ptr<File> fileToLoad);
    bool RegisterResourceFactory(ResourceType resourceType, std::string resourceTypeXML,
                                 std::shared_ptr<ResourceFactory> factory);

  protected:
    void RegisterGlobalResourceFactories();

  private:
    std::shared_ptr<Context> mContext;
    std::unordered_map<ResourceType, std::shared_ptr<ResourceFactory>> mFactories;
    std::unordered_map<std::string, std::shared_ptr<ResourceFactory>> mFactoriesStr;
    std::unordered_map<std::string, ResourceType> mFactoriesTypes;
};
} // namespace LUS
