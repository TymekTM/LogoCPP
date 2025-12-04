#include "pch.h"
#include "ParsingHelper.h"
#include <charconv>

namespace ParsingHelper {
    double ParseValue(const std::string& value, const std::unordered_map<std::string, double>& variables) {
        if (value.empty()) return 0.0;
        
        // Szybkie sprawdzenie czy to liczba (zaczyna sie od cyfry, '-' lub '.')
        char first = value[0];
        if (first == '-' || first == '.' || (first >= '0' && first <= '9')) {
            double result;
            auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
            if (ec == std::errc{} && ptr == value.data() + value.size()) {
                return result;
            }
        }

        auto it = variables.find(value);
        if (it != variables.end()) {
            return it->second;
        }
        return 0.0;
    }
}
