#pragma once

#include <memory>
#include <unordered_map>
#include "ResourceType.h"
#include "ResourceFactory.h"
#include "Resource.h"
#include "OtrFile.h"

namespace Ship {
class Window;

class ResourceLoader {
  public:
    ResourceLoader(std::shared_ptr<Window> context);

    std::shared_ptr<Window> GetContext();
    std::shared_ptr<Resource> LoadResource(std::shared_ptr<OtrFile> fileToLoad);
    bool RegisterResourceFactory(ResourceType resourceType, std::shared_ptr<ResourceFactory> factory);

  protected:
    void RegisterGlobalResourceFactories();

  private:
    std::shared_ptr<Window> mContext;
    std::unordered_map<ResourceType, std::shared_ptr<ResourceFactory>> mFactories;
};
} // namespace Ship
