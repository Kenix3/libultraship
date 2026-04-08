#pragma once

#include <atomic>
#include <stdint.h>

namespace Ship {
#define INVALID_PART_ID 0xFFFFFFFFFFFFFFFF

class Part {
public:
    Part();

    uint64_t GetId() const;
    bool operator==(const Part& other) const;

protected:
private:
  static std::atomic<uint64_t> NextPartId;
  uint64_t mId;
}
} // namespace Ship