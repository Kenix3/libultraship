#pragma once

#include "resource/File.h"

namespace ShipDK {
class ResourceManager;

class IResource {
  public:
    inline static const std::string gAltAssetPrefix = "alt/";

    IResource(std::shared_ptr<ShipDK::ResourceInitData> initData);
    virtual ~IResource();

    virtual void* GetRawPointer() = 0;
    virtual size_t GetPointerSize() = 0;

    bool IsDirty();
    void Dirty();
    std::shared_ptr<ShipDK::ResourceInitData> GetInitData();

  private:
    std::shared_ptr<ShipDK::ResourceInitData> mInitData;
    bool mIsDirty = false;
};

template <class T> class Resource : public IResource {
  public:
    using IResource::IResource;
    virtual T* GetPointer() = 0;
    void* GetRawPointer() override {
        return static_cast<void*>(GetPointer());
    }
};

} // namespace ShipDK
