#include "../../types/SkeletonLimb.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class SkeletonLimbFactory {
  public:
    static SkeletonLimb* ReadSkeletonLimb(BinaryReader* reader);
};
} // namespace Ship