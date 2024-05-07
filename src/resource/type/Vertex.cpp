#include "resource/type/Vertex.h"
#include "libultraship/libultra/gbi.h"

namespace LUS {
Vertex::Vertex() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {
}

Vtx* Vertex::GetPointer() {
    return VertexList.data();
}

size_t Vertex::GetPointerSize() {
    return VertexList.size() * sizeof(Vtx);
}
} // namespace LUS
