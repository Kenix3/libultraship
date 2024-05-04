#include "resource/type/Vertex.h"
#include "graphic/Fast3D/lus_gbi.h"

namespace LUS {
Vertex::Vertex() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {
}

F3DVtx* Vertex::GetPointer() {
    return VertexList.data();
}

size_t Vertex::GetPointerSize() {
    return VertexList.size() * sizeof(F3DVtx);
}
} // namespace LUS
