#pragma once

#include "../types.h"
#include "loggable.h"

/*! \file */

namespace util {

/*! \ingroup maths
    \brief A class to represent rational numbers
    \details This class is test in the application user::frac.
    \note Make sure all fractions comparisons in the code are made using the util::Frac comparison operators for consistency.

    Usage:
    \code{.cpp}
    // 3 (= 3/1)
    util::Frac x(3);
    // 2/3
    util::Frac y(2,3);
    // Prints false:
    util::logger << (y > x) << util::cr;
    \endcode */
class Frac : public util::Loggable {
  public:
    /*! \brief Constructor from numerator and denominator */
    Frac(Num num, Num denom = 1);
    Frac(Frac const &other) = default;
    Frac &operator=(Frac const &other) = default;
    Frac &operator=(Frac &&other) = default;
    Frac(Frac &&other) = default;

    /*! \brief Returns the numerator */
    Num get_num() const;
    /*! \brief Returns the numerator, allows to set the value of the numerator
     *
     * This allows to write:
     * \code{.cpp}
     * util::Frac x(2,5);
     * x.get_num() = 3;
     * // x is now 3/5
     * \endcode */
    Num &get_num();
    /*! \brief Returns the denominator */
    Num get_denom() const;

    /*! \brief Comparison between fractions, returns `*this < other`
        \note Hard assertion for overflows */
    bool operator<(util::Frac const &other) const;
    /*! \brief Comparison between fractions, returns `*this > other`
        \note Hard assertion for overflows */
    bool operator>(util::Frac const &other) const;
    /*! \brief Comparison between fractions, returns `*this <= other`
        \note Hard assertion for overflows */
    bool operator<=(util::Frac const &other) const;
    /*! \brief Comparison between fractions, returns `*this >= other`
        \note Hard assertion for overflows */
    bool operator>=(util::Frac const &other) const;
    /*! \brief Comparison between fractions, returns `*this != other`
        \note Hard assertion for overflows */
    bool operator!=(util::Frac const &other) const;
    /*! \brief Comparison between fractions, returns `*this == other`
        \note Hard assertion for overflows */
    bool operator==(util::Frac const &other) const;

    /*! \brief To print fractions as `2/3 (0.333...)` */
    void log() const override;

  private:
    /*! \brief The numerator */
    Num m_num;
    /*! \brief The denominator */
    Num m_denom;
};

} // namespace util
