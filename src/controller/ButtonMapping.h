#include <cstdint>

namespace LUS {
class ButtonMapping {
  public:
    ButtonMapping(int32_t bitmask);
    ~ButtonMapping();

    virtual void UpdatePad(int32_t& padButtons) = 0;

  protected:
    int32_t mBitmask;
};
} // namespace LUS