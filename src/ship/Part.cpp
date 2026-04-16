#include "ship/Part.h"

namespace Ship {
std::atomic<uint64_t> Part::NextPartId = 0;

Part::Part() : mId(NextPartId++) {
}

uint64_t Part::GetId() const {
    return mId;
}

bool Part::operator==(const Part& other) const {
    return GetId() == other.GetId();
}
} // namespace Ship