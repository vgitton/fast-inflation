#pragma once

#include <string>

/*! \file */

/*! \defgroup color Coloring terminal output
    \ingroup printing
    \brief This module allows an easy manipulation of colors, and relies on the ANSI encoding of colors that works with most terminals. */

namespace util {

/*! \ingroup color
    \brief The available colors */
enum class Color {
    fg, ///< foreground, i.e., resets the color

    // Normal colors
    black,   ///< black
    red,     ///< red
    green,   ///< green
    yellow,  ///< yellow
    blue,    ///< blue
    magenta, ///< magenta
    cyan,    ///< cyan
    white,   ///< white

    // Brighter colors
    alt_black,   ///< lighter black
    alt_red,     ///< brighter red
    alt_green,   ///< brighter green
    alt_yellow,  ///< lighter yellow
    alt_blue,    ///< lighter blue
    alt_magenta, ///< lighter magenta
    alt_cyan,    ///< lighter cyan
    alt_white    /// brighter white
};

/*! \ingroup color
    \brief Return the escape code for ANSI coloring

    \sa https://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html

    \param c
    \return The ANSI code for the desired color */
std::string get_ansi_escape_code(util::Color c);

} // namespace util
