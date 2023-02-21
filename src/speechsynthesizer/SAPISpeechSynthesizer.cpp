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
#include <spdlog/fmt/xchar.h>

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
    const size_t languageSize = strlen(language) + 1;
    wchar_t* wLanguage = new wchar_t[languageSize];
    mbstowcs(wLanguage, language, languageSize);

    std::string locale = fmt::format("{}.UTF-8", language);
    locale.replace(locale.find("-"), 1, "_");
    std::setlocale(LC_ALL, locale.c_str());

    const size_t textSize = strlen(text) + 1;
    wchar_t* wText = new wchar_t[textSize];
    mbstowcs(wText, text, textSize);

    auto speakText = fmt::format(
        L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='{}'>{}</speak>", wLanguage, wText);
    mVoice->Speak(speakText.c_str(), SPF_IS_XML | SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL);

    delete[] wLanguage;
    delete[] wText;
}

void SAPISpeechSynthesizer::Speak(const char* text, const char* language) {
    std::thread t1(SpeakThreadTask, text, language);
    t1.detach();
}
} // namespace Ship
