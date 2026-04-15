#pragma once

#include <atomic>
#include <stdint.h>

namespace Ship {

#define INVALID_PART_ID UINT64_MAX

class Part {
  public:
    Part();
    virtual ~Part() = default;

    uint64_t GetId() const;
    bool operator==(const Part& other) const;

  private:
    static std::atomic<uint64_t> NextPartId;
    uint64_t mId;
};

} // namespace Ship
