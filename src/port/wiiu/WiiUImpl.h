#pragma once

#ifdef __WIIU__

#include <string>

namespace Ship::WiiU {

void Init(const std::string& shortName);
void Exit();
[[noreturn]] void ThrowInvalidOTR();

} // namespace Ship::WiiU

#endif
