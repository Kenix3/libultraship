#include "resource/type/Matrix.h"

namespace LUS {
void* Matrix::GetPointer() {
    return &Matrx;
}

size_t Matrix::GetPointerSize() {
    return sizeof(Mtx);
}
} // namespace LUS
