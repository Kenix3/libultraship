#include "../../types/DisplayList.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class DisplayListFactory {
  public:
    static DisplayList* ReadDisplayList(BinaryReader* reader);
};
} // namespace Ship