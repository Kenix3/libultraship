#include "ButtonMapping.h"

namespace LUS {
class DummyButtonMapping final : public ButtonMapping {
  public:
    using ButtonMapping::ButtonMapping;
    void UpdatePad(int32_t& padButtons) override;
};
} // namespace LUS
