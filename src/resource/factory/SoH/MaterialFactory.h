#include "../../types/Material.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class MaterialFactory {
  public:
    static Material* ReadMaterial(BinaryReader* reader);
};
} // namespace Ship