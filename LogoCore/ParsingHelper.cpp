#include "pch.h"
#include "ParsingHelper.h"
#include <stdexcept>

namespace ParsingHelper {
    double ParseValue(const std::string& value, const std::map<std::string, double>& variables) {
        try {
            return std::stod(value);
        }
        catch (const std::exception&) {
            auto it = variables.find(value);
            if (it != variables.end()) {
                return it->second;
            }
            return 0.0;
        }
    }
}
