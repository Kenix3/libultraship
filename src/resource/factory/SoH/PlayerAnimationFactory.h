#include "../../types/PlayerAnimation.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class PlayerAnimationFactory {
  public:
    static PlayerAnimation* ReadPlayerAnimation(BinaryReader* reader);
};
} // namespace Ship