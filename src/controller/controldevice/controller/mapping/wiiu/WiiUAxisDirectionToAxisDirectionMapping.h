#ifdef __WIIU__
#include "controller/controldevice/controller/mapping/ControllerAxisDirectionMapping.h"
#include "WiiUMapping.h"

#define WII_U_AXIS_LEFT_STICK_X 0
#define WII_U_AXIS_LEFT_STICK_Y 1
#define WII_U_AXIS_RIGHT_STICK_X 2
#define WII_U_AXIS_RIGHT_STICK_Y 3
#define WII_U_AXIS_NUNCHUK_STICK_X 4
#define WII_U_AXIS_NUNCHUK_STICK_Y 5

namespace LUS {
class WiiUAxisDirectionToAxisDirectionMapping final : public ControllerAxisDirectionMapping, public WiiUMapping {
  public:
    WiiUAxisDirectionToAxisDirectionMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, Stick stick,
                                            Direction direction, int32_t wiiuControllerAxis, int32_t axisDirection);
    float GetNormalizedAxisDirectionValue() override;
    std::string GetAxisDirectionMappingId() override;
    uint8_t GetMappingType() override;
    void SaveToConfig() override;
    void EraseFromConfig() override;
    std::string GetPhysicalInputName() override;
    std::string GetPhysicalDeviceName() override;
    bool PhysicalDeviceIsConnected() override;

  private:
    int32_t mControllerAxis;
    AxisDirection mAxisDirection;
};
} // namespace LUS
#endif
