#pragma once
#include <string>
#include <unordered_map>

namespace ParsingHelper {
    inline double ParseValue(const std::string& value, const std::unordered_map<std::string, double>& variables) {
        if (value.empty()) return 0.0;
        
        char first = value[0];
        if (first == '-' || first == '.' || (first >= '0' && first <= '9')) {
            // Fast manual double parse for simple integers (most common case)
            if (first >= '0' && first <= '9') {
                bool isSimple = true;
                for (size_t i = 1; i < value.size(); ++i) {
                    char c = value[i];
                    if (c < '0' || c > '9') {
                        if (c == '.') { isSimple = false; break; }
                        else { isSimple = false; break; }
                    }
                }
                if (isSimple) {
                    double result = 0;
                    for (char c : value) result = result * 10.0 + (c - '0');
                    return result;
                }
            }
            // Fallback to stod
            try { return std::stod(value); } catch (...) {}
        }

        auto it = variables.find(value);
        if (it != variables.end()) {
            return it->second;
        }
        return 0.0;
    }
}
