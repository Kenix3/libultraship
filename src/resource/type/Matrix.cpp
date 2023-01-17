#include "resource/type/Matrix.h"

namespace Ship {
void* Matrix::GetPointer() {
    return &Matrx;
}

size_t Matrix::GetPointerSize() {
    return sizeof(Mtx);
}
} // namespace Ship
