#include "../../types/Blob.h"
#include "binarytools/BinaryReader.h"

namespace Ship {
class BlobFactory {
  public:
    static Blob* ReadBlob(BinaryReader* reader);
};
} // namespace Ship