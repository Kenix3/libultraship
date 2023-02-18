//
//  SAPISpeechSynthesizer.cpp
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#include "SAPISpeechSynthesizer.h"
#include <sapi.h>
#include <thread>
#include <string>
#include <spdlog/fmt/fmt.h>

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

void SAPISpeechSynthesizer::DoUninitialize() {
    mVoice->Release();
    mVoice = NULL;
    CoUninitialize();
}

void SpeakThreadTask(const char* text, const char* language) {
    auto speak =
        fmt::format("<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='{}'>{}</speak>",
                    language, text)
            .c_str();

    const int w = 512;
    int* wp = const_cast<int*>(&w);
    *wp = strlen(speak));

    wchar_t wtext[w];
    mbstowcs(wtext, speak), strlen(speak)) + 1);

    mVoice->Speak(wtext, SPF_IS_XML | SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL);
}

void SAPISpeechSynthesizer::Speak(const char* text, const char* language) {
    std::thread t1(SpeakThreadTask, text, language);
    t1.detach();
}
} // namespace Ship
