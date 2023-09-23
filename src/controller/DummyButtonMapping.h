#include "ButtonMapping.h"

namespace LUS {
class DummyButtonMapping final : public ButtonMapping {
  public:
    using ButtonMapping::ButtonMapping;
    void UpdatePad(uint16_t& padButtons) override;
};
} // namespace LUS
