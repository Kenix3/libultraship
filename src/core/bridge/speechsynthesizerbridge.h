//
//  speechsynthesizerbridge.h
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#ifndef speechsynthesizerbridge_h
#define speechsynthesizerbridge_h

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

bool SpeechSynthesizerInit(void);
void SpeechSynthesizerUninitialize(void);

void SpeechSynthesizerSpeakEnglish(const char* text);
void SpeechSynthesizerSpeak(const char* text, const char* language);

#ifdef __cplusplus
};
#endif

#endif /* speechsynthesizerbridge_h */
