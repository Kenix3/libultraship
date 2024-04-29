#include "resource/type/DisplayList.h"

namespace LUS {
DisplayList::DisplayList() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {
}

Gfx* DisplayList::GetPointer() {
    return Instructions.data();
}

size_t DisplayList::GetPointerSize() {
    return Instructions.size() * sizeof(Gfx);
}
} // namespace LUS
