#include "../../types/Text.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class TextFactory {
  public:
    static Text* ReadText(BinaryReader* reader);
};
} // namespace Ship