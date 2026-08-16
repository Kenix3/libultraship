#include "ship/resource/Resource.h"
#include <spdlog/spdlog.h>

namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData> initData) : mInitData(initData) {
}

IResource::~IResource() {
    if (GetInitData() == nullptr) {
        return;
    }

    if (GetInitData()->Identifier.IsPath()) {
        SPDLOG_TRACE("Resource Unloaded: {}\n", GetInitData()->Identifier.GetPath());
    } else {
        SPDLOG_TRACE("Resource Unloaded: {}\n", GetInitData()->Identifier.GetPathHash());
    }
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
