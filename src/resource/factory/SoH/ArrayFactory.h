#include "../../types/Array.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class ArrayFactory {
  public:
    static Array* ReadArray(BinaryReader* reader);
};
} // namespace Ship