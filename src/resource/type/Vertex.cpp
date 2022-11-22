#include "resource/type/Vertex.h"

namespace Ship {
void* Vertex::GetPointer() {
    return VertexList.data();
}

size_t Vertex::GetPointerSize() {
    return VertexList.size() * sizeof(Vtx);
}
} // namespace Ship
