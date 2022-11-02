#include "../../types/Skeleton.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class SkeletonFactory {
  public:
    static Skeleton* ReadSkeleton(BinaryReader* reader);
};
} // namespace Ship