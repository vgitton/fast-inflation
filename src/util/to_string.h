#pragma once

#include <string>
#include <vector>

/*! \file */

/*! \defgroup str To-string conversions
    \ingroup printing

    \brief To convert various types of data to strings
    */

namespace util {

//! \cond
class Loggable;
//! \endcond

// Custom string conversion

/*! \ingroup str
    @{ */

/*! \brief The default for std::to_string()-compatible types, e.g., int, double, etc
    \param x To stringify
    \return `std::to_string(x)` */
template <typename T>
std::string str(T const &x) {
    return std::to_string(x);
}

/*! \brief Specialization for double to get scientific notation */
template <>
std::string str(double const &x);

/*! \brief Specialization for bool to get `true` or `false` */
template <>
std::string str(bool const &b);

/*! \brief Specialize to std::string to trivially return argument */
template <>
std::string str(std::string const &string);

/*! \brief Specialize to `char const*`
    \details This is technically not a specialization because not `const&` */
std::string str(char const *string);

/*! \brief To stringify `std::vector<T>`
    \return `(vec[0],vec[1],...) */
template <typename T>
std::string str(std::vector<T> const &vec) {
    std::string ret = "(";
    bool first = true;
    for (T const &v : vec) {
        if (not first)
            ret += ", ";
        else
            first = false;
        ret += util::str(v);
    }
    ret += ")";
    return ret;
}

/*! \brief To stringify pairs
    \return `(util::str(pair.first),util::str(pair.second))` */
template <typename T, typename U>
std::string str(std::pair<T, U> const &pair) {
    return std::string("(") + util::str(pair.first) + " ; " + util::str(pair.second) + ")";
}

/*! \brief To remove the spaces of a string
 * \param s `"H i "` \return `"Hi"` */
std::string remove_spaces(std::string const &s);

//! @}

} // namespace util
