#include "resource/type/Matrix.h"

namespace Fast {
Matrix::Matrix() : Resource(std::shared_ptr<Ship::ResourceInitData>()) {
}

Mtx* Matrix::GetPointer() {
    return &Matrx;
}

size_t Matrix::GetPointerSize() {
    return sizeof(Mtx);
}
} // namespace Fast
