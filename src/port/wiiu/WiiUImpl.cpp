#ifdef __WIIU__

#include "port/wiiu/WiiUImpl.h"

#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>

#include <coreinit/debug.h>
#include <padscore/kpad.h>

namespace Ship::WiiU {

namespace {
void EnsureDirectory(const char* path) {
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        OSFatal("Unable to create the Wii U application directory.");
    }
}
} // namespace

void Init(const std::string& shortName) {
    // Aroma starts WUHBs from the SD volume.  Keep all writable SoH state
    // beside the executable, matching the historical Wii U port layout.
    EnsureDirectory("/vol/external01/wiiu");
    EnsureDirectory("/vol/external01/wiiu/apps");
    const std::string appDirectory = "/vol/external01/wiiu/apps/" + shortName;
    EnsureDirectory(appDirectory.c_str());
    if (chdir(appDirectory.c_str()) != 0) {
        OSFatal("Unable to enter the Wii U application directory.");
    }
    KPADInit();
    WPADEnableURCC(true);
}

void Exit() {
    KPADShutdown();
}

[[noreturn]] void ThrowInvalidOTR() {
    OSFatal("Invalid O2R files. Regenerate the game assets from your supported ROM.");
    __builtin_unreachable();
}

} // namespace Ship::WiiU

#endif
