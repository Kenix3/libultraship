#pragma once
#include <string>

namespace OtrLib {
	bool stob(std::string s, bool defaultVal = false);
	int32_t stoi(std::string s, int32_t defaultVal = 0);
	int64_t stoll(std::string s, int64_t defaultVal = 0);
}