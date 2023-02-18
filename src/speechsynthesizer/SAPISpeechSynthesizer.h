//
//  SAPISpeechSynthesizer.h
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#ifndef SAPISpeechSynthesizer_h
#define SAPISpeechSynthesizer_h

#include "SpeechSynthesizer.h"
#include <stdio.h>

namespace Ship {
class SAPISpeechSynthesizer : public SpeechSynthesizer {
  public:
    SAPISpeechSynthesizer();

    void Speak(const char* text, const char* language);

  protected:
    bool DoInit(void);
    void DoUninitialize(void);
};
} // namespace Ship

#endif /* SAPISpeechSynthesizer_h */
