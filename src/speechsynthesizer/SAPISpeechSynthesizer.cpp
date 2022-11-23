//
//  SAPISpeechSynthesizer.cpp
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#include "SAPISpeechSynthesizer.hpp"
#include <sapi.h>
#include <thread>

namespace Ship {
SAPISpeechSynthesizer::SAPISpeechSynthesizer() {
}

bool SAPISpeechSynthesizer::DoInit() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HRESULT CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
    CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&mVoice);
    return true;
}

void SpeakThreadTask(const char* textt) {
    const int w = 512;
    int* wp = const_cast<int*>(&w);
    *wp = strlen(text);

    wchar_t wtext[w];
    mbstowcs(wtext, text, strlen(text) + 1);

    (ISpVoice*)mVoice->Speak(wtext, SPF_IS_XML | SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL);
}

void SAPISpeechSynthesizer::Speak(const char* text) {
    std::thread t1(SpeakThreadTask, text);
    t1.detach();
}
} // namespace Ship
