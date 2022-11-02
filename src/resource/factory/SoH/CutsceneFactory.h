#include "../../types/Cutscene.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class CutsceneFactory {
  public:
    static Cutscene* ReadCutscene(BinaryReader* reader);
};
} // namespace Ship