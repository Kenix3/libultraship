#ifdef __WIIU__
#include "WiiUImpl.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/iosupport.h>

#include <whb/log.h>
#include <whb/log_udp.h>
#include <coreinit/debug.h>

#include "Context.h"

namespace LUS {
namespace WiiU {

static bool hasVpad = false;
static VPADReadError vpadError;
static VPADStatus vpadStatus;

static bool hasKpad[4] = { false };
static KPADError kpadError[4] = { KPAD_ERROR_OK };
static KPADStatus kpadStatus[4];
static uint8_t kpadExtensions[4];

static bool controllersInitialized = false;

#ifdef _DEBUG
extern "C" {
void __wrap_abort() {
    printf("Abort called.\n");
    // force a stack trace
    *(uint32_t*)0xdeadc0de = 0xcafebabe;
    while (1)
        ;
}

static ssize_t wiiu_log_write(struct _reent* r, void* fd, const char* ptr, size_t len) {
    char buf[1024];
    snprintf(buf, sizeof(buf), "%*.*s", len, len, ptr);
    OSReport(buf);
    WHBLogWritef("%*.*s", len, len, ptr);
    return len;
}

static const devoptab_t dotab_stdout = {
    .name = "stdout_whb",
    .write_r = wiiu_log_write,
};
};
#endif

void Init(const std::string& shortName) {
#ifdef _DEBUG
    WHBLogUdpInit();
    WHBLogPrint("Hello World!");

    devoptab_list[STD_OUT] = &dotab_stdout;
    devoptab_list[STD_ERR] = &dotab_stdout;
#endif

    // make sure the required folders exist
    mkdir("/vol/external01/wiiu/", 0755);
    mkdir("/vol/external01/wiiu/apps/", 0755);
    mkdir(("/vol/external01/wiiu/apps/" + shortName + "/").c_str(), 0755);

    chdir(("/vol/external01/wiiu/apps/" + shortName + "/").c_str());

    KPADInit();
    WPADEnableURCC(true);
}

void Exit() {
    KPADShutdown();

    WHBLogUdpDeinit();
}

void ThrowMissingOTR(const char* otrPath) {
    // TODO handle this better in the future
    OSFatal("Main OTR file not found!");
}

void ThrowInvalidOTR() {
    OSFatal("Invalid OTR files! Try regenerating them!");
}

void Update() {
    bool rescan = false;

    VPADRead(VPAD_CHAN_0, &vpadStatus, 1, &vpadError);
    if (vpadError == VPAD_READ_SUCCESS) {
        if (!hasVpad) {
            rescan = true;
        }

        hasVpad = true;
    } else if (vpadError != VPAD_READ_NO_SAMPLES) {
        if (hasVpad) {
            rescan = true;
        }

        hasVpad = false;
    }

    for (int i = 0; i < 4; i++) {
        KPADReadEx((KPADChan)i, &kpadStatus[i], 1, &kpadError[i]);
        if (kpadError[i] == KPAD_ERROR_OK && kpadStatus[i].extensionType != 255) {
            if (!hasKpad[i]) {
                rescan = true;
            }

            if (kpadStatus[i].extensionType != kpadExtensions[i]) {
                kpadExtensions[i] = kpadStatus[i].extensionType;
                rescan = true;
            }

            // set emulated stick button bits so all controllers behave like the gamepad does
            switch (kpadStatus[i].extensionType) {
                case WPAD_EXT_PRO_CONTROLLER:
                    if (kpadStatus[i].pro.leftStick.x > 0.7f) {
                        kpadStatus[i].pro.hold |= WPAD_PRO_STICK_L_EMULATION_RIGHT;
                    }
                    if (kpadStatus[i].pro.leftStick.x < -0.7f) {
                        kpadStatus[i].pro.hold |= WPAD_PRO_STICK_L_EMULATION_LEFT;
                    }
                    if (kpadStatus[i].pro.leftStick.y > 0.7f) {
                        kpadStatus[i].pro.hold |= WPAD_PRO_STICK_L_EMULATION_UP;
                    }
                    if (kpadStatus[i].pro.leftStick.y < -0.7f) {
                        kpadStatus[i].pro.hold |= WPAD_PRO_STICK_L_EMULATION_DOWN;
                    }

                    if (kpadStatus[i].pro.rightStick.x > 0.7f) {
                        kpadStatus[i].pro.hold |= WPAD_PRO_STICK_R_EMULATION_RIGHT;
                    }
                    if (kpadStatus[i].pro.rightStick.x < -0.7f) {
                        kpadStatus[i].pro.hold |= WPAD_PRO_STICK_R_EMULATION_LEFT;
                    }
                    if (kpadStatus[i].pro.rightStick.y > 0.7f) {
                        kpadStatus[i].pro.hold |= WPAD_PRO_STICK_R_EMULATION_UP;
                    }
                    if (kpadStatus[i].pro.rightStick.y < -0.7f) {
                        kpadStatus[i].pro.hold |= WPAD_PRO_STICK_R_EMULATION_DOWN;
                    }
                    break;
                case WPAD_EXT_CLASSIC:
                case WPAD_EXT_MPLUS_CLASSIC:
                    if (kpadStatus[i].classic.leftStick.x > 0.7f) {
                        kpadStatus[i].classic.hold |= WPAD_CLASSIC_STICK_L_EMULATION_RIGHT;
                    }
                    if (kpadStatus[i].classic.leftStick.x < -0.7f) {
                        kpadStatus[i].classic.hold |= WPAD_CLASSIC_STICK_L_EMULATION_LEFT;
                    }
                    if (kpadStatus[i].classic.leftStick.y < -0.7f) {
                        kpadStatus[i].classic.hold |= WPAD_CLASSIC_STICK_L_EMULATION_UP;
                    }
                    if (kpadStatus[i].classic.leftStick.y > 0.7f) {
                        kpadStatus[i].classic.hold |= WPAD_CLASSIC_STICK_L_EMULATION_DOWN;
                    }

                    if (kpadStatus[i].classic.rightStick.x > 0.7f) {
                        kpadStatus[i].classic.hold |= WPAD_CLASSIC_STICK_R_EMULATION_RIGHT;
                    }
                    if (kpadStatus[i].classic.rightStick.x < -0.7f) {
                        kpadStatus[i].classic.hold |= WPAD_CLASSIC_STICK_R_EMULATION_LEFT;
                    }
                    if (kpadStatus[i].classic.rightStick.y > 0.7f) {
                        kpadStatus[i].classic.hold |= WPAD_CLASSIC_STICK_R_EMULATION_UP;
                    }
                    if (kpadStatus[i].classic.rightStick.y < -0.7f) {
                        kpadStatus[i].classic.hold |= WPAD_CLASSIC_STICK_R_EMULATION_DOWN;
                    }
                    break;
                case WPAD_EXT_NUNCHUK:
                case WPAD_EXT_MPLUS_NUNCHUK:
                    if (kpadStatus[i].nunchuck.stick.x < -0.7f) {
                        kpadStatus[i].nunchuck.hold |= WPAD_NUNCHUK_STICK_EMULATION_LEFT;
                    }
                    if (kpadStatus[i].nunchuck.stick.x > 0.7f) {
                        kpadStatus[i].nunchuck.hold |= WPAD_NUNCHUK_STICK_EMULATION_RIGHT;
                    }
                    if (kpadStatus[i].nunchuck.stick.y > 0.7f) {
                        kpadStatus[i].nunchuck.hold |= WPAD_NUNCHUK_STICK_EMULATION_UP;
                    }
                    if (kpadStatus[i].nunchuck.stick.y < -0.7f) {
                        kpadStatus[i].nunchuck.hold |= WPAD_NUNCHUK_STICK_EMULATION_DOWN;
                    }
                    break;
            }

            hasKpad[i] = true;
        } else if (kpadError[i] != KPAD_ERROR_NO_SAMPLES) {
            if (hasKpad[i]) {
                rescan = true;
            }

            hasKpad[i] = false;
        }
    }

    // rescan devices if connection state changed
    if (controllersInitialized && rescan) {
        Context::GetInstance()->GetControlDeck()->GetDeviceIndexMappingManager()->HandlePhysicalDevicesChanged();
    }
}

void SetControllersInitialized() {
    controllersInitialized = true;
}

VPADStatus* GetVPADStatus(VPADReadError* error) {
    *error = vpadError;
    return hasVpad ? &vpadStatus : nullptr;
}

KPADStatus* GetKPADStatus(WPADChan chan, KPADError* error) {
    *error = kpadError[chan];
    return hasKpad[chan] ? &kpadStatus[chan] : nullptr;
}

}; // namespace WiiU
}; // namespace LUS

#endif
