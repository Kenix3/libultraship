#include "Environment.h"

#include <map>
#include <string>

std::map<std::string, std::string> environmentVars;

namespace SohUtils {
    void saveEnvironmentVar(std::string key, std::string value) {
        environmentVars[key] = value;
    }
    std::string getEnvironmentVar(std::string key) {
        return environmentVars[key];
    }
}