#include "resource/type/DisplayList.h"

namespace Ship {
void* DisplayList::GetPointer() {
    return Instructions.data();
}

size_t DisplayList::GetPointerSize() {
    return Instructions.size() * sizeof(Gfx);
}
} // namespace Ship
