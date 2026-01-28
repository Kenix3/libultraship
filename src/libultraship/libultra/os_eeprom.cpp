#include "libultraship/libultraship.h"
#include <string>

#ifdef __ANDROID__
#include <jni.h>
extern "C" {
    const char* Android_GetSaveDir();
}
#endif

static std::string GetSaveFilePath() {
#ifdef __ANDROID__
    const char* saveDir = Android_GetSaveDir();
    if (saveDir != nullptr) {
        return std::string(saveDir) + "/default.sav";
    }
#endif
    return Ship::Context::GetPathRelativeToAppDirectory("default.sav");
}

extern "C" {

int32_t osEepromProbe(OSMesgQueue* mq) {
    return EEPROM_TYPE_4K;
}

int32_t osEepromLongRead(OSMesgQueue* mq, uint8_t address, uint8_t* buffer, int32_t length) {
    u8 content[512];
    s32 ret = -1;

    const std::string save_file = GetSaveFilePath();
    FILE* fp = fopen(save_file.c_str(), "rb");
    if (fp == NULL) {
        return -1;
    }
    if (fread(content, 1, 512, fp) == 512) {
        memcpy(buffer, content + address * 8, length);
        ret = 0;
    }
    fclose(fp);

    return ret;
}

int32_t osEepromRead(OSMesgQueue* mq, u8 address, u8* buffer) {
    return osEepromLongRead(mq, address, buffer, 8);
}

int32_t osEepromLongWrite(OSMesgQueue* mq, uint8_t address, uint8_t* buffer, int32_t length) {
    u8 content[512] = { 0 };
    if (address != 0 || length != 512) {
        osEepromLongRead(mq, 0, content, 512);
    }
    memcpy(content + address * 8, buffer, length);

    const std::string save_file = GetSaveFilePath();
    FILE* fp = fopen(save_file.c_str(), "wb");
    if (fp == NULL) {
        return -1;
    }
    s32 ret = fwrite(content, 1, 512, fp) == 512 ? 0 : -1;
    fclose(fp);
    return ret;
}

int32_t osEepromWrite(OSMesgQueue* mq, uint8_t address, uint8_t* buffer) {
    return osEepromLongWrite(mq, address, buffer, 8);
}
}
