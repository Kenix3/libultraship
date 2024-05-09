#include "resource/type/DisplayList.h"
#include <memory>

namespace LUS {
DisplayList::DisplayList() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {
}

DisplayList::~DisplayList() {
    for (char* string : Strings) {
        free(string);
    }
}

Gfx* DisplayList::GetPointer() {
    return (Gfx*)Instructions.data();
}

size_t DisplayList::GetPointerSize() {
    return Instructions.size() * sizeof(Gfx);
}
} // namespace LUS
