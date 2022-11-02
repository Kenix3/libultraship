#include "../../types/Path.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class PathFactory {
  public:
    static Path* ReadPath(BinaryReader* reader);
};
} // namespace Ship