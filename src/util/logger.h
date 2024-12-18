#pragma once

#include <iostream>
#include <string>
#include <type_traits>

#include "color.h"
#include "to_string.h"

/*! \file */

/*! \defgroup printing Terminal output system
 * \ingroup util
 * \brief This module describes the logger system that we use to create indented and colored output in the terminal.
 *
 * This module defines a global object `util::logger` (available after including logger.h), which should be used in place of `std::cout`.
 * The user can then use `util::logger << loggable` to print a `loggable`
 * of type util::Loggable, or of any type that can be fed into
 * util::str().
 * Furthermore, the `util::logger` supports various additional commands, such as
 * `util::logger << util::begin_section` to begin an indentend section of terminal output.
 *
 * ### Making loggable classes
 *
 * New classes should inherit from util::Loggable and override
 * util::Loggable::log() so that they can be printed as `util::logger << loggable`.
 * The util::Loggable::log() should log an object by using the globally accessible `util::logger`
 * to ensure a sensible output (with respect to color and indentation settings, in particular).
 *
 * ### The log/verbosity level
 *
 * Controlling the log level is achieved by having a notion of section, subsection, subsubsection, etc.
 * For instance:
 * \code{.cpp}
 * util::logger << util::begin_section
 *     << "Welcome!" << util::cr
 *     << "This text can be seen at logLevel at least 1" << util::cr;
 *
 * // This macro is used instead of `util::logger << util::begin_section` for creating a (sub-)section with a label
 * LOG_BEGIN_SECTION("My subsection")
 * util::logger << "This text can be seen at logLevel at least 2" << util::cr;
 * // This macro is equivalent to util::logger << util::end_section;
 * LOG_END_SECTION
 *
 * util::logger << "Again, this text can be seen at logLevel at least 1" << util::cr
 *     << util::end_section;
 * \endcode
 *
 */

namespace util {

/*! \ingroup printing
 *  \brief This custom logger providing additional functionalities over `std::cout` */
class Logger {
  public:
    /*! \brief The coloring scheme */
    enum class ColorMode {
        ansi, ///< for color in the terminal
        none  ///< No color
    };

    /*! \brief The (full names of the) available actions on the logger
        \details These can be abbreviated, e.g., `util::cr` instead of `util::Logger::Action::cr`, etc. */
    enum class Action {
        flush,         ///< flush
        endl,          ///< flush and newline
        cr,            ///< newline ("Carriage Return")
        begin_section, ///< to increase indentation
        end_section,   ///< to decrease indentation
        left,          ///< align left
        right,         ///< align right (works in tandem with util::setw())
        reset_indent,  ///< reset indentation
        wait,          ///< pause the execution of the program
    };

    /*! \brief Internal class, use util::setw() instead */
    class WidthManip {
      public:
        WidthManip(int the_width) : width(the_width) {}
        int width;
    };

    /*! \brief Constructor setting the color mode
        \details To turn off support of colored output in the terminal (e.g., if the terminal at hand does not support the ANSI color encoding), then the globally accessible `util::logger` can be initialized without color in the file logger.cpp. */
    Logger(util::Logger::ColorMode color_mode);
    //! \cond
    Logger(Logger const &other) = delete;
    Logger(Logger &&other) = delete;
    Logger &operator=(Logger const &other) = delete;
    Logger &operator=(Logger &&other) = delete;
    //! \endcond

    /*! \brief To control the max level of indentation to be displayed (the rest is truncated from the output) */
    void set_log_level(unsigned int log_level);

    /*! \brief By default, hidden sections above the max log level get printed as "...", but this method disables this behavior. */
    void disable_dots_for_hidden_sections();

    /*! \brief Prints a string, taking care of indentation */
    void echo(std::string const &str);

    // Action methods

    /*! \brief Flushes the output to stdout, used upon calling `util::logger << util::flush;` */
    void flush();
    /*! \brief Creates an indented newline, used upon calling `util::logger << util::cr;` */
    void new_line();
    /*! \brief Creates a new section (more indentation), used upon calling `util::logger << util::begin_section;` */
    void begin_section();
    /*! \brief Ends a section (less indentation), used upon calling `util::logger << util::end_section;` */
    void end_section();
    /*! \brief for `util::logger << util::setw(n);` manipulation
        \param n The desired width for the next print call */
    void set_next_width(int n);
    /*! \brief To change the next alignment in combination with util::setw(), e.g., `util::logger << util::left << util::setw(5) << "a";` */
    void set_alignment(std::ios_base::fmtflags alignment);
    /*! \brief Resets the indentation to the left */
    void reset_indent();

    /*! \brief Format for main title (green) */
    void echo_title(std::string const &name);
    /*! \brief Format for secondary title (blue) */
    void echo_header(std::string const &name);
    /*! \brief Returns either an ANSI color code or an empty string, depending on the util::Logger::ColorMode */
    std::string get_color_code(util::Color c) const;
    /*! \brief To color numbers depending on the value of the number */
    std::string get_colored_number_str(int number) const;
    /*! \brief To log colored numbers */
    void echo_colored_number(int number);
    /*! \brief To prepend an error message by a red `Error:` */
    void echo_error(std::string const &what);

    inline util::Logger::ColorMode get_color_mode() const { return m_color_mode; }

    /*! \brief Returns a string with a single-char representation of the number \p i.
     * \details Currently, sends 0...9 to '0'...'9' and then 10 to 'a', 11 to 'b', etc */
    std::string get_single_char(unsigned long int i) const;

    /*! \brief Places a "mark" at the current line position to be able to rewind using util::Logger::go_back_to_mark() */
    void set_mark();
    /*! \brief Moves the cursor up to the line where util::Logger::set_mark() was called */
    void go_back_to_mark();
    /*! \brief Moves the cursor down to the place util::Logger::go_back_to_mark() was last called (unless we're already farther down) */
    void cancel_go_back_to_mark();

    /*! \brief Moves the cursor up by i+1 and enters a util::cr */
    void move_up(int i);
    /*! \brief Moves the cursor down by i-1 lines and enters a util::cr */
    void move_down(int i);

    /*! \brief `true` if no output should be printed because of the log level setting */
    inline bool is_disabled() const { return not m_depth_enabled; }

  private:
    // Getters

    /*! \brief Internal method used for indentation */
    void process_pending_indent();

    /*! \brief Controls whether or not to log things: only `true` if `m_section_index <= m_log_level` */
    bool m_depth_enabled;
    /*! \brief Whether or not to use color in the console */
    util::Logger::ColorMode m_color_mode;
    /*! \brief The log level controlling the amount of output */
    unsigned int m_log_level;

    // Indentation related things

    /*! \brief The current section (larger = deeper in sub-sub-sections) */
    unsigned int m_section_index;
    /*! \brief Indicates whether it is still required to print out the indentation space characters
        \details It is not desirable to indent directly after receiving a newline instruction,
        there might be a `util::logger << util::end_section` instruction coming up */
    bool m_indent_pending;
    /*! \brief Whether or not to issue dots to indicate that logs are omitted. */
    bool m_disable_dots_for_hidden_sections;

    /*! \brief Counts the number of newlines printed since the last mark was set */
    int m_n_cr_since_mark;
    /*! \brief Counts the number of newlines printed before going back to a mark, to be able to leave the mark section */
    int m_max_n_cr_since_mark;
};

/*! \ingroup printing
 *  \brief The globally accessible logger replacing `std::cout` */
extern util::Logger logger;

/*! \ingroup printing
    \brief Flush */
constexpr util::Logger::Action flush = util::Logger::Action::flush;
/*! \ingroup printing
    \brief Flush and newline */
constexpr util::Logger::Action endl = util::Logger::Action::endl;
/*! \ingroup printing
    \brief Newline */
constexpr util::Logger::Action cr = util::Logger::Action::cr;
/*! \ingroup printing
    \brief Increase indentation */
constexpr util::Logger::Action begin_section = util::Logger::Action::begin_section;
/*! \ingroup printing
    \brief Decrease indentation */
constexpr util::Logger::Action end_section = util::Logger::Action::end_section;
/*! \ingroup printing
    \brief Align the next call to util::setw() to the left */
constexpr util::Logger::Action left = util::Logger::Action::left;
/*! \ingroup printing
    \brief Align the next call to util::setw() to the right */
constexpr util::Logger::Action right = util::Logger::Action::right;
/*! \ingroup printing
    \brief Reset indentation to zero */
constexpr util::Logger::Action reset_indent = util::Logger::Action::reset_indent;
/*! \ingroup printing
    \brief Pause the execution of the program, internally calling `std::cin` */
constexpr util::Logger::Action wait = util::Logger::Action::wait;

/*! \ingroup printing
    \brief Dimmed color
    \details Custom colors - not constexpr because might want to update them without recompiling */
extern const util::Color begin_comment;
/*! \ingroup printing
    \brief End dimmed color */
extern const util::Color end_comment;

/*! \ingroup printing
    \brief Used as `util::logger << util::setw(5) << "a"` to print `"____a"` (four spaces before "a") */
util::Logger::WidthManip setw(int width);
/*! \ingroup printing
    \brief Used as `util::logger << util::phantom("bonjour")` to print 7 spaces
    \param str The string whose length will set the desired width of space
    \return a string of whitespaces of adequate length */
std::string phantom(std::string const &str);

} // namespace util

/*! \ingroup printing
 * \brief Prints util::Loggable instances using their util::Loggable::log() method
 * \details ... unless the util::logger is disabled */
template <typename T>
typename std::enable_if<std::is_base_of<util::Loggable, T>::value, util::Logger &>::type
operator<<(util::Logger &logger, T const &loggable) {
    if (logger.is_disabled())
        return logger;

    // This prints to util::logger
    loggable.log();
    return logger;
}

/*! \ingroup printing
 *  \brief (Attempts to) print non util::Loggable things by trying to convert them to a `std::string` via some `util::str()` overload.
 */
template <typename T>
typename std::enable_if<not std::is_base_of<util::Loggable, T>::value, util::Logger &>::type
operator<<(util::Logger &logger, T const &loggable) {
    if (logger.is_disabled())
        return logger;

    logger.echo(util::str(loggable));
    return logger;
}

/*! \ingroup printing
    \brief To support, e.g., `util::logger << util::cr` (new line) */
util::Logger &operator<<(util::Logger &logger, util::Logger::Action action);
/*! \ingroup printing
    \brief To support, e.g., `util::logger << util::setw(5)` (see util::Logger::WidthManip)
    \details The parameter \p width_manip is meant to be created using util::setw(). */
util::Logger &operator<<(util::Logger &logger, util::Logger::WidthManip width_manip);
/*! \ingroup printing
    \brief To support, e.g., `util::logger << util::Color::blue` */
util::Logger &operator<<(util::Logger &logger, util::Color c);

/*! \ingroup printing
    \brief Shorthand to create a section with header \p X
    \param X The header name */
#define LOG_BEGIN_SECTION(X)                        \
    util::logger.echo_header(X + std::string(":")); \
    util::logger << util::begin_section;
/*! \ingroup printing
    \brief Shorthand to create a section with the current function name as header */
#define LOG_BEGIN_SECTION_FUNC                               \
    util::logger.echo_header(__func__ + std::string("():")); \
    util::logger << util::begin_section;
/*! \ingroup printing
    \brief Shorthand for `util::logger << util::end_section`, to match the LOG_BEGIN_SECTION(X) syntax */
#define LOG_END_SECTION util::logger << util::end_section;
/*! \ingroup printing
    \brief To quickly print `variable_name = 7`
    \param X the variable whose name and value will be printed. Thus, `X` must be either a util::Loggable or a util::str() compatible entity to have its value printed. */
#define LOG_VARIABLE(X) util::logger << util::begin_comment << #X << " = " << util::end_comment << X << util::cr;
