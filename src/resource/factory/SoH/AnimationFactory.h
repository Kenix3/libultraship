#include "../../types/Animation.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class AnimationFactory {
  public:
    static Animation* ReadAnimation(BinaryReader* reader);
};
} // namespace Ship