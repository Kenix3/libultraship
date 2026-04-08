#pragma once

#include <vector>
#include <memory>
#include <type_traits>
#include <stdint.h>

#include "ship/Part.h"

namespace Ship {
#define INCLUDE_MUTEX 1
#ifdef INCLUDE_MUTEX
#define MUTEX_CONST const
#else
#define MUTEX_CONST 
#endif

class Component;

template <typename T>
concept DerivedFromPart = std::derived_from<T, Part>;

// >= 0 means that the Part is guaranteed to be in the list after running.
// < 0 is an error scenario.
// > 0 means that a Part was actually added to the list.
// == 0 means that the list is unchanged.
enum class ListReturnCode : int32_t {
    ForcedSuccess = 2,
    Success = 1,
    Duplicate = 0,
    Unknown = 0,
    NotFound = 0,
    NoItemsProvided = -1,
    NotPermitted = -2,
    Failed = -3
};

template <DerivedFromPart C = Part> class PartList : public Part {
  public:
    PartList(std::shared_ptr<Component> parent, const size_t initialAllocation);

    std::shared_ptr<Component> GetParent() const;

    bool Has(std::shared_ptr<C> part) MUTEX_CONST;
    template <typename T> bool Has() MUTEX_CONST;
    bool Has(const uint64_t id) MUTEX_CONST;
    bool Has() MUTEX_CONST;
    size_t GetCount() MUTEX_CONST;

    std::shared_ptr<C> Get(const uint64_t id) MUTEX_CONST;
    std::shared_ptr<std::vector<std::shared_ptr<C>>> Get() MUTEX_CONST;
    template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> Get() MUTEX_CONST;
    std::shared_ptr<std::vector<std::shared_ptr<C>>> Get(const std::vector<uint64_t>& ids) MUTEX_CONST;

    ListReturnCode Add(std::shared_ptr<C> part);
    ListReturnCode Add(const std::vector<std::shared_ptr<C>>& parts);
    ListReturnCode Remove(std::shared_ptr<C> part);
    ListReturnCode Remove(const uint64_t id);
    ListReturnCode Remove();
    ListReturnCode Remove(const std::vector<std::shared_ptr<C>>& parts);
    template <typename T> ListReturnCode Remove();
    ListReturnCode Remove(const std::vector<uint64_t>& ids);

  protected:
    std::vector<std::shared_ptr<C>>& GetList() const;

  private:
    std::shared_ptr<Component> mParent;
    std::vector<std::shared_ptr<C>> mList;
}
} // namespace Ship