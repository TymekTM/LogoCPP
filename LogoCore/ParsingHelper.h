#pragma once
#include <string>
#include <unordered_map>

namespace ParsingHelper {
    double ParseValue(const std::string& value, const std::unordered_map<std::string, double>& variables);
}
