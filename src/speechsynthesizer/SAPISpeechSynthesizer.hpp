//
//  SAPISpeechSynthesizer.hpp
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#ifndef SAPISpeechSynthesizer_hpp
#define SAPISpeechSynthesizer_hpp

#include "SpeechSynthesizer.hpp"
#include <stdio.h>

namespace Ship {
class SAPISpeechSynthesizer : public SpeechSynthesizer {
public:
    SAPISpeechSynthesizer();

    void Speak(const char* text);

protected:
    bool DoInit(void);
};
}

#endif /* SAPISpeechSynthesizer_hpp */
