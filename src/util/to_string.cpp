#include "to_string.h"
#include <algorithm> // For std::remove
#include <iomanip>
#include <sstream>

template <>
std::string util::str(double const &x) {
    std::ostringstream oss;
    oss << std::scientific << x;
    return oss.str();
}

template <>
std::string util::str(bool const &b) {
    return b ? std::string("true") : std::string("false");
}

template <>
std::string util::str(std::string const &string) {
    return string;
}

std::string util::str(const char *string) {
    return std::string(string);
}

std::string util::remove_spaces(std::string const &s) {
    std::string working = s;

    // Remove spaces
    // The call to std::remove send "q ( )  " to "q() )  "
    //                                               ----
    working.erase(std::remove(working.begin(), working.end(), ' '), working.end());

    return working;
}
