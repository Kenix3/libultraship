#include "../../types/CollisionHeader.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class CollisionHeaderFactory {
  public:
    static CollisionHeader* ReadCollisionHeader(BinaryReader* reader);
};
} // namespace Ship