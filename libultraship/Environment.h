#pragma once

#include <string>

namespace SohUtils {
    void saveEnvironmentVar(std::string key, std::string value);
    std::string getEnvironmentVar(std::string key);
}