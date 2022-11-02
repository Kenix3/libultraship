#pragma once
#include "../../types/Vertex.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class VertexFactory {
  public:
    static Vertex* ReadVtx(BinaryReader* reader);
};
} // namespace Ship