#include "ship/Part.h"

namespace Ship {
std::atomic<uint64_t> Part::sNextPartId = 0;

Part::Part() : mId(sNextPartId++) {
}

uint64_t Part::GetId() const {
    return mId;
}

bool Part::operator==(const Part& other) const {
    return GetId() == other.GetId();
}
} // namespace Ship