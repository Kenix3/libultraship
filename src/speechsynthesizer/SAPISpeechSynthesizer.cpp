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

std::wstring c2ws(const char* text) {
    std::string str(text);
    int textSize = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(textSize, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], textSize);
    return wstrTo;
}

void SpeakThreadTask(const char* text, const char* language) {
    auto wLanguage = c2ws(language);
    auto wText = c2ws(text);

    auto speakText = fmt::format(
        L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='{}'>{}</speak>", wLanguage, wText);
    mVoice->Speak(speakText.c_str(), SPF_IS_XML | SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL);
}

void SAPISpeechSynthesizer::Speak(const char* text, const char* language) {
    std::thread t1(SpeakThreadTask, text, language);
    t1.detach();
}
} // namespace Ship
