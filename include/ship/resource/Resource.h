#pragma once

#include "ship/resource/File.h"

namespace Ship {
class ResourceManager;

/**
 * @brief Non-templated base interface for all resources managed by the ResourceManager.
 *
 * IResource stores the initialization metadata that was used to load the underlying data
 * and exposes a type-erased pointer to that data. The dirty flag is used to signal that
 * the resource needs to be reloaded or regenerated (e.g. when the source archive changes).
 */
class IResource {
  public:
    /** @brief Path prefix that identifies an "alt asset" override. */
    inline static const std::string gAltAssetPrefix = "alt/";

    /**
     * @brief Constructs an IResource with the given initialization data.
     * @param initData Metadata describing how this resource was loaded (path, type, version, …).
     */
    IResource(std::shared_ptr<ResourceInitData> initData);
    virtual ~IResource();

    /**
     * @brief Returns a type-erased raw pointer to the underlying resource data.
     * @return Void pointer to the resource payload; never null for a successfully loaded resource.
     */
    virtual void* GetRawPointer() = 0;

    /**
     * @brief Returns the size (in bytes) of the type pointed to by GetRawPointer().
     * @return Size in bytes.
     */
    virtual size_t GetPointerSize() = 0;

    /**
     * @brief Returns true if the resource has been marked dirty.
     *
     * A dirty resource should be considered stale and may need to be reloaded.
     */
    bool IsDirty();

    /**
     * @brief Marks the resource as dirty.
     *
     * Callers (e.g. the ResourceManager) use this to signal that the cached data
     * is no longer up-to-date and should be reloaded before next use.
     */
    void Dirty();

    /**
     * @brief Returns the initialization data used to load this resource.
     * @return Shared pointer to the ResourceInitData.
     */
    std::shared_ptr<ResourceInitData> GetInitData();

  private:
    std::shared_ptr<ResourceInitData> mInitData;
    bool mIsDirty = false;
};

/**
 * @brief Typed resource base class that provides a strongly-typed pointer accessor.
 *
 * Extend this class (rather than IResource directly) when implementing a concrete
 * resource type so that consumers can retrieve the payload without casting.
 *
 * @tparam T The concrete payload type exposed by GetPointer().
 */
template <class T> class Resource : public IResource {
  public:
    using IResource::IResource;

    /**
     * @brief Returns a typed pointer to the resource payload.
     * @return Pointer to T; never null for a successfully loaded resource.
     */
    virtual T* GetPointer() = 0;

    void* GetRawPointer() override {
        return static_cast<void*>(GetPointer());
    }
};

} // namespace Ship
