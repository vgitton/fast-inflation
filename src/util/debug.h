#pragma once

#include "to_string.h"
#include <stdexcept>

/*! \file */

/*! \defgroup debug Debugging
    \ingroup util
    \brief Support for `gdb` and soft/hard assertions.

    ### Assertions

    The hard assertions look like #HARD_ASSERT_TRUE(X), #HARD_ASSERT_EQUAL(X,Y), etc.
    Such assertions are always tripped, and throw an error indicating the file, line, and expected statement.

    The soft assertions look like #ASSERT_TRUE(X), #ASSERT_EQUAL(X,Y), etc.
    Such assertions are only tripped in debug mode, which is internally controlled by the `NDEBUG` macro only defined in released mode.
    For instance:
    \code{.cpp}
    #ifndef NDEBUG
        // Here code only executed in debug mode
    #else
        // Here code only executed in release mode
    #endif
    \endcode
    However, the point of the debugging macros is to *not* have to use such `#ifndef NDEBUG` statements in applications.
    For those cases where this is actually convenient, one can use
    \code{.cpp}
    DEBUG_RUN(
        // Here code only executed in debug mode
        int x = 0;
        ++x;
        util::logger << x << util::cr;
    )
    \endcode

    ### Support for gdb

    Gdb support is currently turned off (to speed up compilation, mostly).
    It can be brought back in the makefile by uncommenting the `-g` flag in the `DEBUGFLAGS` variable.

    */

namespace util {

/*! \ingroup debug
    \brief Dummy function, use #BREAK_HERE instead */
void break_here();

#ifndef NDEBUG
/*! \ingroup debug
    \brief Use to set a breakpoint for gdb

    In `~/.gdbinit`, place
    ```
    set breakpoint pending on
    break break_here
    ```
    to be able to use the #BREAK_HERE macro, which evaluates to a call to the
    dummy function `void break_here() {}` in debug mode, and to nothing in release mode.
    Then, gdb will break on the line where this macro was called. */
#define BREAK_HERE util::break_here();
#else
#define BREAK_HERE
#endif

#ifndef NDEBUG
/*! \ingroup debug
    \brief To run an instruction only in debug mode */
#define DEBUG_RUN(X) X
/*! \ingroup debug
 *  \brief To print a statement only in debug mode */
#define DEBUG_LOG(X) util::logger << #X << " = " << X << util::cr;
#else
#define DEBUG_RUN(X)
#define DEBUG_LOG(X)
#endif

/*! \ingroup debug
    \brief This internal function combines the file, line, function name and error message in a nice error string. */
std::string get_error_str(
    std::string const &file,
    int line,
    std::string const &function_name,
    std::string const &error_message);

/*! \ingroup debug
    \brief This macro throws a std::logic_error with an error message indicating the file, line and function where the macro was called. */
#define THROW_ERROR(error_message) throw std::logic_error(util::get_error_str(__FILE__, __LINE__, __PRETTY_FUNCTION__, error_message));

/*! \ingroup debug
    \brief Hard assertion, tests for `X == Y` and calls THROW_ERROR else.

    Tests for `X == Y`: if false, throws exception indicating the values that `X` and `Y` had, together with the location of the assertion.
    */
#define HARD_ASSERT_EQUAL(X, Y)                                                                               \
    if (not((X) == (Y))) {                                                                                    \
        util::assert_equal_fail(__FILE__, __LINE__, __PRETTY_FUNCTION__, #X, #Y, util::str(X), util::str(Y)); \
    }
/*! \ingroup debug
    \brief Hard assertion, tests for `X` being `true` (so `X` should be a `bool`) and calls THROW_ERROR else. */
#define HARD_ASSERT_TRUE(X)                                                  \
    if (not(X)) {                                                            \
        util::assert_true_fail(__FILE__, __LINE__, __PRETTY_FUNCTION__, #X); \
    }
/*! \ingroup debug
    \brief Hard assertion, tests for `X < Y` and calls THROW_ERROR else. */
#define HARD_ASSERT_LT(X, Y)                                                                               \
    if (not((X) < (Y))) {                                                                                  \
        util::assert_lt_fail(__FILE__, __LINE__, __PRETTY_FUNCTION__, #X, #Y, util::str(X), util::str(Y)); \
    }
/*! \ingroup debug
    \brief Hard assertion, tests for `X <= Y` and calls THROW_ERROR else. */
#define HARD_ASSERT_LTE(X, Y)                                                                               \
    if (not((X) <= (Y))) {                                                                                  \
        util::assert_lte_fail(__FILE__, __LINE__, __PRETTY_FUNCTION__, #X, #Y, util::str(X), util::str(Y)); \
    }
/*! \ingroup debug
    \brief Hard assertion to check that a map indeed contains an element

    This macro has proven useful. The compilation flag `-D_GLIBCXX_DEBUG` equips std::vector with bound checks for `operator[]` but does not do the same for std::map. Hence, using this assertion fills that gap. */
#define HARD_ASSERT_MAP_CONTAINS(map, element)                                                                       \
    if (not map.contains(element)) {                                                                                 \
        util::assert_map_contains_fail(__FILE__, __LINE__, __PRETTY_FUNCTION__, #map, #element, util::str(element)); \
    }

#ifndef NDEBUG
/*! \ingroup debug
    \brief Soft assertion, tests for `X == Y`

    This soft assertion is only triggered in debug mode: the code
    \code{.cpp}
    ASSERT_EQUAL(0,1)
    \endcode
    only throws an error when compiled in debug mode, but not in release mode. */
#define ASSERT_EQUAL(X, Y) HARD_ASSERT_EQUAL(X, Y)
/*! \ingroup debug
    \brief Soft assertion, tests for `X` being true */
#define ASSERT_TRUE(X) HARD_ASSERT_TRUE(X)
/*! \ingroup debug
    \brief Soft assertion, tests for `X < Y` */
#define ASSERT_LT(X, Y) HARD_ASSERT_LT(X, Y)
/*! \ingroup debug
    \brief Soft assertion, tests for `X <= Y` */
#define ASSERT_LTE(X, Y) HARD_ASSERT_LTE(X, Y)
/*! \ingroup debug
    \brief Soft assertion to check that a map indeed contains an element

    This macro has proven useful. The compilation flag `-D_GLIBCXX_DEBUG` equips std::vector with bound checks for `operator[]` but does not do the same for std::map. Hence, using this soft assertion fills that gap. */
#define ASSERT_MAP_CONTAINS(map, element) HARD_ASSERT_MAP_CONTAINS(map, element)
#else
#define ASSERT_EQUAL(X, Y)
#define ASSERT_TRUE(X)
#define ASSERT_LT(X, Y)
#define ASSERT_LTE(X, Y)
#define ASSERT_MAP_CONTAINS(map, element)
#endif

/*! \ingroup debug
    \brief Internal function to pretty-print #ASSERT_EQUAL fail statements */
void assert_equal_fail(
    std::string const &file,
    int line,
    std::string const &function,
    std::string const &first_arg,
    std::string const &second_arg,
    std::string const &first_val,
    std::string const &second_val);

/*! \ingroup debug
    \brief Internal function to pretty-print #ASSERT_TRUE fail statements */
void assert_true_fail(
    std::string const &file,
    int line,
    std::string const &function,
    std::string const &arg);

/*! \ingroup debug
    \brief Internal function to pretty-print #ASSERT_LT fail statements */
void assert_lt_fail(
    std::string const &file,
    int line,
    std::string const &function,
    std::string const &first_arg,
    std::string const &second_arg,
    std::string const &first_val,
    std::string const &second_val);

/*! \ingroup debug
    \brief Internal function to pretty-print #ASSERT_LTE fail statements */
void assert_lte_fail(
    std::string const &file,
    int line,
    std::string const &function,
    std::string const &first_arg,
    std::string const &second_arg,
    std::string const &first_val,
    std::string const &second_val);

/*! \ingroup debug
    \brief Internal function to pretty-print #ASSERT_MAP_CONTAINS fail statements */
void assert_map_contains_fail(
    std::string const &file,
    int line,
    std::string const &function,
    std::string const &map_name,
    std::string const &element_name,
    std::string const &element_val);
} // namespace util
