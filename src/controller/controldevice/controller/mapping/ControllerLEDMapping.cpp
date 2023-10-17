#include "ControllerLEDMapping.h"

namespace LUS {
ControllerLEDMapping::ControllerLEDMapping(LUSDeviceIndex lusDeviceIndex, uint8_t portIndex, uint8_t colorSource,
                                           Color_RGB8 savedColor)
    : ControllerMapping(lusDeviceIndex), mPortIndex(portIndex), mColorSource(colorSource), mSavedColor(savedColor) {
}

ControllerLEDMapping::~ControllerLEDMapping() {
}

void ControllerLEDMapping::SetColorSource(uint8_t colorSource) {
    mColorSource = colorSource;
    if (mColorSource == LED_COLOR_SOURCE_OFF) {
        SetLEDColor(Color_RGB8({ 0, 0, 0 }));
    }
    if (mColorSource == LED_COLOR_SOURCE_SET) {
        SetLEDColor(mSavedColor);
    }
    SaveToConfig();
}

uint8_t ControllerLEDMapping::GetColorSource() {
    return mColorSource;
}

void ControllerLEDMapping::SetSavedColor(Color_RGB8 colorToSave) {
    mSavedColor = colorToSave;
    if (mColorSource == LED_COLOR_SOURCE_SET) {
        SetLEDColor(mSavedColor);
    }
    SaveToConfig();
}

Color_RGB8 ControllerLEDMapping::GetSavedColor() {
    return mSavedColor;
}

void ControllerLEDMapping::SetPortIndex(uint8_t portIndex) {
    mPortIndex = portIndex;
}
} // namespace LUS
