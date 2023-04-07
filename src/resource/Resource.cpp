#include "Resource.h"
#include "resource/type/DisplayList.h"
#include "resource/ResourceMgr.h"
#include <spdlog/spdlog.h>
#include "libultraship/libultra/gbi.h"

namespace Ship {
Resource::Resource(std::shared_ptr<ResourceMgr> resourceManager, std::shared_ptr<ResourceInitData> initData)
    : ResourceManager(resourceManager), InitData(initData) {
}

Resource::~Resource() {
    SPDLOG_TRACE("Resource Unloaded: {}\n", InitData->Path);
}
} // namespace Ship
