#pragma once
#include "../../types/Matrix.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class MtxFactory {
  public:
    static Matrix* ReadMtx(BinaryReader* reader);
};
} // namespace Ship