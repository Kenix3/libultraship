#include "../../types/Scene.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class SceneFactory {
  public:
    static Scene* ReadScene(BinaryReader* reader);
};
} // namespace Ship