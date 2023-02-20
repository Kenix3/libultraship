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
    auto speechSynthesizer = Ship::Window::GetInstance()->GetSpeechSynthesizer();
    if (speechSynthesizer == nullptr) {
        return false;
    }

    if (speechSynthesizer->Init()) {
        return true;
    }

    return false;
}

void SpeechSynthesizerUninitialize(void) {
    auto speechSynthesizer = Ship::Window::GetInstance()->GetSpeechSynthesizer();
    if (speechSynthesizer == nullptr) {
        return;
    }

    speechSynthesizer->Uninitialize();
}

void SpeechSynthesizerSpeak(const char* text, const char* language) {
    auto speechSynthesizer = Ship::Window::GetInstance()->GetSpeechSynthesizer();
    if (speechSynthesizer == nullptr) {
        return;
    }

    speechSynthesizer->Speak(text, language);
}
}
