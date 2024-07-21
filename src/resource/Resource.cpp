#include "Resource.h"
#include <spdlog/spdlog.h>

namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData> initData) : mInitData(initData) {
}

IResource::~IResource() {
    SPDLOG_TRACE("Resource Unloaded: {}\n", GetInitData()->Path);
}

bool IResource::IsDirty() {
    return mIsDirty;
}

void IResource::Dirty() {
    mIsDirty = true;
}

std::shared_ptr<ResourceInitData> IResource::GetInitData() {
    return mInitData;
}
} // namespace Ship
