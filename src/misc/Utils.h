#pragma once

#include <string>
#include <vector>

namespace Ship {

namespace Math {
float clamp(float d, float min, float max);
bool isNumber(const std::string& s);
} // namespace Math

std::vector<std::string> splitText(const std::string& text, char separator, bool keepQuotes);
std::string toLowerCase(std::string in);
} // namespace Ship
