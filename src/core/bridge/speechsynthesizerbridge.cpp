//
//  speechsynthesizerbridge.cpp
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#include "speechsynthesizerbridge.h"
#include "core/Window.h"

extern "C" {

bool SpeechSynthesizerInit(void) {
    auto speechSynthesizer = Ship::Window::GetInstance()->SpeechSynthesizer();
    if (speechSynthesizer == nullptr) {
        return false;
    }

    if (speechSynthesizer->Init()) {
        return true;
    }

    return false
}
}
