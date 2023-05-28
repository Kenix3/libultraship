#include "resource/type/Matrix.h"

namespace LUS {
Matrix::Matrix() : Resource(std::shared_ptr<ResourceInitData>()) {
}

void* Matrix::GetRawPointer() {
    return &Matrx;
}

size_t Matrix::GetPointerSize() {
    return sizeof(Mtx);
}
} // namespace LUS
