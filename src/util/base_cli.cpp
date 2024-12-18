#include "base_cli.h"
#include "debug.h"
#include "range.h"
#include <memory>

util::BaseCLI::BaseCLI(int argc, char **argv)
    : m_named_args(),
      m_unnamed_args{} {
    for (int argv_i = 1; argv_i < argc; ++argv_i) {
        std::string argv_s = argv[argv_i];

        if (argv_s[0] == '-' and argv_s[1] == '-') {
            std::string arg_name = argv_s.substr(2, argv_s.size());

            ++argv_i;

            std::string arg_value;
            if (argv_i < argc) {
                arg_value = argv[argv_i];
            } else {
                arg_value = "";
            }

            m_named_args.emplace_back(arg_name, arg_value);
        } else {
            m_unnamed_args.push_back(argv[argv_i]);
        }
    }
}

std::string util::BaseCLI::extract_str_option(std::string const &name, std::string const &default_value) {
    unsigned int count = 0;
    std::string ret = default_value;
    // The position of the value to erase
    std::unique_ptr<unsigned int> pos_to_erase = nullptr;
    for (long unsigned pos : util::Range(m_named_args.size())) {
        std::pair<std::string, std::string> const &option = m_named_args[pos];
        if (option.first == name) {
            ++count;
            if (count > 1) {
                THROW_ERROR("Got multiple specifications for the option --" + name)
            }

            ret = option.second;
            pos_to_erase = std::make_unique<unsigned int>(pos);
        }
    }

    if (pos_to_erase != nullptr) {
        m_named_args.erase(m_named_args.begin() + *pos_to_erase);
    }

    return ret;
}
unsigned int util::BaseCLI::extract_uint_option(std::string const &name, unsigned int default_value) {
    std::string value_str = extract_str_option(name, util::str(default_value));

    if (value_str.size() == 0) {
        THROW_ERROR("Got no value for the unsigned int option --" + name)
    }

    int value_int;
    try {
        value_int = std::stoi(value_str);
    } catch (std::exception const &e) {
        THROW_ERROR("Could not parse the --" + name + " value " + value_str + " to an unsigned int")
    }

    if (value_int < 0) {
        THROW_ERROR("Expected a nonnegative integer for the option --" + name + ", got " + value_str)
    }

    return static_cast<unsigned int>(value_int);
}

std::string util::BaseCLI::extract_str_arg(std::string const &default_value) {
    if (m_unnamed_args.size() > 0) {
        std::string ret = m_unnamed_args[0];
        m_unnamed_args.erase(m_unnamed_args.begin());
        return ret;
    } else {
        return default_value;
    }
}

void util::BaseCLI::hard_assert_empty() const {
    std::string error;
    if (m_unnamed_args.size() > 0) {
        error += "Unrecognized positional argument";
        if (m_unnamed_args.size() > 1)
            error += "s";
        error += ": ";
        bool first = true;
        for (std::string const &arg : m_unnamed_args) {
            if (not first)
                error += ", ";
            else
                first = false;

            error += arg;
        }
        error += ". ";
    }

    if (m_named_args.size() > 0) {
        if (error.size() > 0)
            error += "\n";
        error += "Unrecognized option";
        if (m_named_args.size() > 1)
            error += "s";
        error += ": ";
        bool first = true;
        for (std::pair<std::string, std::string> const &option : m_named_args) {
            if (not first)
                error += ", ";
            else
                first = false;
            error += "--" + option.first;
            if (option.second.size() > 0)
                error += " = " + option.second;
            else
                error += " (empty)";
        }
    }

    if (error.size() > 0) {
        THROW_ERROR(error)
    }
}
