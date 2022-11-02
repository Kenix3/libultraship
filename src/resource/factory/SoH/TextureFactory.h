#include "../../types/Texture.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class TextureFactory {
  public:
    static Texture* ReadTexture(BinaryReader* reader);
};
} // namespace Ship