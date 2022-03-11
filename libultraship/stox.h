#pragma once
#include <string>

namespace Ship {
	bool stob(const std::string& s, bool defaultVal = false);
	int32_t stoi(const std::string& s, int32_t defaultVal = 0);
	int64_t stoll(const std::string& s, int64_t defaultVal = 0);
}