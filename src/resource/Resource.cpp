#include "Resource.h"
#include "resource/ResourceManager.h"
#include <spdlog/spdlog.h>
#include "libultraship/libultra/gbi.h"

namespace LUS {
Resource::Resource(std::shared_ptr<ResourceInitData> initData) : InitData(initData) {
}

Resource::~Resource() {
    SPDLOG_TRACE("Resource Unloaded: {}\n", InitData->Path);
}
} // namespace LUS
