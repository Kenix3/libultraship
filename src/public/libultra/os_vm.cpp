#include "libultraship/libultraship.h"

extern "C" {

uintptr_t osVirtualToPhysical(void* addr) {
    return (uintptr_t)addr;
}

void osMapTLB(int32_t a, uint32_t b, void* c, uint32_t d, uint32_t e, uint32_t f) {
}
}