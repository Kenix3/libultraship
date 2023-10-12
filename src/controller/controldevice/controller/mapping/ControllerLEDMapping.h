#pragma once

#include <cstdint>
#include <string>
#include "ControllerMapping.h"
#include "libultraship/color.h"

namespace LUS {

#define LED_COLOR_SOURCE_OFF 0
#define LED_COLOR_SOURCE_SET 1
#define LED_COLOR_SOURCE_GAME 2

class ControllerLEDMapping : public ControllerMapping {
  public:
    ControllerLEDMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, uint8_t colorSource, Color_RGB8 savedColor);
    ~ControllerLEDMapping();

    void SetColorSource(uint8_t colorSource);
    uint8_t GetColorSource();
    virtual void SetLEDColor(Color_RGB8 color) = 0;
    void SetSavedColor(Color_RGB8 colorToSave);
    Color_RGB8 GetSavedColor();
    void SetPortIndex(uint8_t portIndex);

    virtual std::string GetLEDMappingId() = 0;
    virtual void SaveToConfig() = 0;
    virtual void EraseFromConfig() = 0;

  protected:
    uint8_t mPortIndex;
    uint8_t mColorSource;
    Color_RGB8 mSavedColor;
};
} // namespace LUS
