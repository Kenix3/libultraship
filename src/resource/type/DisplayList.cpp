#include "resource/type/DisplayList.h"

namespace LUS {
void* DisplayList::GetPointer() {
    return Instructions.data();
}

size_t DisplayList::GetPointerSize() {
    return Instructions.size() * sizeof(Gfx);
}
} // namespace LUS
