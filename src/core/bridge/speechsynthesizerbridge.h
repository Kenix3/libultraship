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
void SpeechSynthesizerSpeak(const char* text);

#ifdef __cplusplus
};
#endif

#endif /* speechsynthesizerbridge_h */
