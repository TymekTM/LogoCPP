#pragma once
#include <string>
#include <map>

namespace ParsingHelper {
    double ParseValue(const std::string& value, const std::map<std::string, double>& variables);
}
