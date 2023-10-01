#include "ControllerLEDMapping.h"

namespace LUS {
ControllerLEDMapping::ControllerLEDMapping(uint8_t portIndex, uint8_t colorSource, Color_RGB8 savedColor)
    : mPortIndex(portIndex), mColorSource(colorSource), mSavedColor(savedColor) {
}

ControllerLEDMapping::~ControllerLEDMapping() {
}

void ControllerLEDMapping::SetColorSource(uint8_t colorSource) {
    mColorSource = colorSource;
    if (mColorSource == LED_COLOR_SOURCE_OFF) {
        SetLEDColor(Color_RGB8({0,0,0}));
    }
    if (mColorSource == LED_COLOR_SOURCE_SET) {
        SetLEDColor(mSavedColor);
    }
}

uint8_t ControllerLEDMapping::GetColorSource() {
    return mColorSource;
}

void ControllerLEDMapping::SaveLEDColor(Color_RGB8 colorToSave) {
    mSavedColor = colorToSave;
    if (mColorSource == LED_COLOR_SOURCE_SET) {
        SetLEDColor(mSavedColor);
    }
}
} // namespace LUS
