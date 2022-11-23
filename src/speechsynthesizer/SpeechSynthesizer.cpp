//
//  SpeechSynthesizer.cpp
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#include "SpeechSynthesizer.h"

namespace Ship {
SpeechSynthesizer::SpeechSynthesizer() : mInitialized(false) {
}

bool SpeechSynthesizer::Init(void) {
    mInitialized = DoInit();
    return IsInitialized();
}

bool SpeechSynthesizer::IsInitialized(void) {
    return mInitialized;
}
} // namespace Ship
