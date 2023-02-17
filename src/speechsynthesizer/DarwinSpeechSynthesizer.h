//
//  DarwinSpeechSynthesizer.h
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#ifndef DarwinSpeechSynthesizer_h
#define DarwinSpeechSynthesizer_h

#include "SpeechSynthesizer.h"
#include <stdio.h>

namespace Ship {
class DarwinSpeechSynthesizer : public SpeechSynthesizer {
  public:
    DarwinSpeechSynthesizer();

    void Speak(const char* text, const char* language);

  protected:
    bool DoInit(void);

  private:
    void* mSynthesizer;
};
} // namespace Ship

#endif /* DarwinSpeechSynthesizer_h */
