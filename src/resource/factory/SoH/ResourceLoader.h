#include "../../Resource.h"
#include "../../OtrFile.h"

namespace Ship {
class ResourceLoader {
  public:
    static Resource* LoadResource(std::shared_ptr<OtrFile> fileToLoad);
};
} // namespace Ship