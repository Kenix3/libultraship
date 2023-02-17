//
//  SAPISpeechSynthesizer.cpp
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#include "SAPISpeechSynthesizer.h"
#include <sapi.h>
#include <thread>

ISpVoice* mVoice = NULL;

namespace Ship {
SAPISpeechSynthesizer::SAPISpeechSynthesizer() {
}

bool SAPISpeechSynthesizer::DoInit() {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HRESULT CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
    CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&mVoice);
    return true;
}

void SpeakThreadTask(const char* text, const char* language) {
    std::wstring wtext = L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='";
    wtext += std::wstring(language, language + strlen(language));
    wtext += L"'>";
    wtext += std::wstring(text, text + strlen(text));
    wtext += L"</speak>";

    mVoice->Speak(wtext, SPF_IS_XML | SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL);
}

void SAPISpeechSynthesizer::Speak(const char* text, const char* language) {
    std::thread t1(SpeakThreadTask, text, language);
    t1.detach();
}
} // namespace Ship
