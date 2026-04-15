#include "ship/utils/Utils.h"
#include <algorithm>

namespace Ship {
namespace Math {
float clamp(float d, float min, float max) {
    const float t = d < min ? min : d;
    return t > max ? max : t;
}

size_t HashCombine(size_t lhs, size_t rhs) {
    if constexpr (sizeof(size_t) >= 8) {
        lhs ^= rhs + 0x517cc1b727220a95 + (lhs << 6) + (lhs >> 2);
    } else {
        lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    }
    return lhs;
}
} // namespace Math

std::vector<std::string> splitText(const std::string& text, char separator, bool keepQuotes) {
    std::vector<std::string> args;
    const size_t length = text.length();

    bool inQuotes = false;
    size_t from = 0;

    for (size_t i = 0; i < length; i++) {
        if (text[i] == '"') {
            inQuotes = !inQuotes;
        } else if (text[i] == separator && !inQuotes) {
            const size_t tokenLength = i - from;
            if (tokenLength > 0) {
                size_t start = from;
                size_t end = i;
                if (!keepQuotes && (end - start) >= 2 && text[start] == '"' && text[end - 1] == '"') {
                    start++;
                    end--;
                }

                if (end > start) {
                    args.emplace_back(text.substr(start, end - start));
                }
            }

            from = i + 1;
        }
    }

    if (from < length) {
        size_t start = from;
        size_t end = length;

        if (!keepQuotes && (end - start) >= 2 && text[start] == '"' && text[end - 1] == '"') {
            start++;
            end--;
        }

        if (end > start) {
            args.emplace_back(text.substr(start, end - start));
        }
    }

    return args;
}

std::string toLowerCase(std::string in) {
    std::string cpy(in);
    std::transform(cpy.begin(), cpy.end(), cpy.begin(), ::tolower);
    return cpy;
}
} // namespace Ship
