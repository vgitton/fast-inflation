#include "color.h"

std::string
util::get_ansi_escape_code(util::Color c) {
    // Basic colors
    switch (c) {
    case util::Color::fg:
        return "\033[0m";

    case util::Color::black:
        return "\033[30m";
    case util::Color::red:
        return "\033[31m";
    case util::Color::green:
        return "\033[32m";
    case util::Color::yellow:
        return "\033[33m";
    case util::Color::blue:
        return "\033[34m";
    case util::Color::magenta:
        return "\033[35m";
    case util::Color::cyan:
        return "\033[36m";
    case util::Color::white:
        return "\033[37m";

    case util::Color::alt_black:
        return "\033[30;1m";
    case util::Color::alt_red:
        return "\033[31;1m";
    case util::Color::alt_green:
        return "\033[32;1m";
    case util::Color::alt_yellow:
        return "\033[33;1m";
    case util::Color::alt_blue:
        return "\033[34;1m";
    case util::Color::alt_magenta:
        return "\033[35;1m";
    case util::Color::alt_cyan:
        return "\033[36;1m";
    case util::Color::alt_white:
        return "\033[37;1m";

    default:
        return "";
    }
}
