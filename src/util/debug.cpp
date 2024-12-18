#include "debug.h"
#include "logger.h"

#include <stdexcept>

void util::break_here() {}

std::string util::get_error_str(
    std::string const &file,
    int line,
    std::string const &function_name,
    std::string const &error_message) {
    return util::logger.get_color_code(util::Color::alt_black) + file + ":" + util::str(line) + ", " + function_name + ": " + util::logger.get_color_code(util::Color::fg) + error_message;
}

void util::assert_equal_fail(
    std::string const &file,
    int line,
    std::string const &function,
    std::string const &first_arg,
    std::string const &second_arg,
    std::string const &first_val,
    std::string const &second_val) {
    std::string error_msg = util::get_error_str(
        file, line, function,
        std::string("expected ") + first_arg + " == " + second_arg + ", got " + first_arg + " = " + first_val + " and " + second_arg + " = " + second_val);

    throw std::logic_error(error_msg);
}

void util::assert_true_fail(
    std::string const &file,
    int line,
    std::string const &function,
    std::string const &arg) {
    std::string error_msg = util::get_error_str(
        file, line, function,
        std::string("expected ") + arg + " to be true");

    throw std::logic_error(error_msg);
}

void util::assert_lt_fail(
    std::string const &file,
    int line,
    std::string const &function,
    std::string const &first_arg,
    std::string const &second_arg,
    std::string const &first_val,
    std::string const &second_val) {
    std::string error_msg = util::get_error_str(
        file, line, function,
        std::string("expected ") + first_arg + " < " + second_arg + ", got " + first_arg + " = " + first_val + " and " + second_arg + " = " + second_val);

    throw std::logic_error(error_msg);
}

void util::assert_lte_fail(
    std::string const &file,
    int line,
    std::string const &function,
    std::string const &first_arg,
    std::string const &second_arg,
    std::string const &first_val,
    std::string const &second_val) {
    std::string error_msg = util::get_error_str(
        file, line, function,
        std::string("expected ") + first_arg + " <= " + second_arg + ", got " + first_arg + " = " + first_val + " and " + second_arg + " = " + second_val);

    throw std::logic_error(error_msg);
}

void util::assert_map_contains_fail(
    std::string const &file,
    int line,
    std::string const &function,
    std::string const &map_name,
    std::string const &element_name,
    std::string const &element_val) {
    std::string error_msg = util::get_error_str(
        file, line, function,
        std::string("the map ") + map_name + " does not contain " + element_name + " = " + element_val);

    throw std::logic_error(error_msg);
}
