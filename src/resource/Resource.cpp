#include "Resource.h"
#include "resource/ResourceManager.h"
#include <spdlog/spdlog.h>
#include "libultraship/libultra/gbi.h"

namespace LUS {
Resource::Resource(std::shared_ptr<ResourceInitData> initData) : mInitData(initData) {
}

Resource::~Resource() {
    SPDLOG_TRACE("Resource Unloaded: {}\n", GetInitData()->Path);
}

bool Resource::IsDirty() {
    return mIsDirty;
}

void Resource::Dirty() {
    mIsDirty = true;
}

std::shared_ptr<ResourceInitData> Resource::GetInitData() {
    return mInitData;
}
} // namespace LUS
