#include "ship/Part.h"
#include "ship/Context.h"

namespace Ship {
std::atomic<uint64_t> Part::sNextPartId = 0;

Part::Part() : mId(sNextPartId++) {
}

Part::Part(std::shared_ptr<Context> context) : mId(sNextPartId++), mContext(std::move(context)) {
}

uint64_t Part::GetId() const {
    return mId;
}

bool Part::operator==(const Part& other) const {
    return GetId() == other.GetId();
}

std::shared_ptr<Context> Part::GetContext() const {
    return mContext.lock();
}

void Part::SetContext(std::shared_ptr<Context> ctx) {
    mContext = ctx;
}

void Part::OnAdded(bool /*forced*/) {
}

void Part::OnRemoved(bool /*forced*/) {
}
} // namespace Ship
