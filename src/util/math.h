#pragma once

#include <limits>
// For std::gcd()
#include <numeric>

#include "../types.h"
#include "debug.h"
#include "range.h"

/*! \file */

/*! \defgroup util Utilities
    \brief Useful bits of code that are unrelated to inflation

    If this was Python, we would just import the necessary packages, but hey, welcome to C++.
*/

/*! \defgroup maths Maths utilities
    \ingroup util
    \brief Fractions, factorials, sums, powers, etc. */

namespace util {

/*! \ingroup maths
    \brief Factorial of integer-like types
    \details The output is required to fit in `Index = unsigned long int`.
    In debug mode, an overflow will trigger an exception.
    \param n \f$\in\{0,1,...\}\f$
    \return \f$ n! \f$ */
template <typename T>
Index factorial(T n) {
    Index ret = 1;
    for (Index i : Range(T(2), T(n + 1))) {
        ASSERT_LT(ret, (std::numeric_limits<Index>::max() / i));
        ret *= i;
    }
    return ret;
}

/*! \ingroup maths
    \brief Integer powers of arithmetic types
    \details The output should fit in the input type.
    In debug mode, an overflow will trigger an exception.
    \param x base
    \param n integer exponent
    \return \f$ x^n \f$ */
template <typename T>
T pow(T x, T n) {
    T ret = 1;
    for (Index i : util::Range(n)) {
        (void)i;
        ASSERT_LT(ret, (std::numeric_limits<T>::max() / x));
        ret *= x;
    }
    return ret;
}

/*! \ingroup maths
    \brief Sums the element of a std::vector
    \details The output should fit in the element type.
    `T` should be a numeric type.
    \warning `T` should be cheap to copy
    \param vec The list of elements to be added together */
template <typename T>
T sum(std::vector<T> const &vec) {
    T ret = 0;
    for (T element : vec) {
        ret += element;
    }
    return ret;
}

/*! \ingroup maths
 *  \brief Sums the squares of the elements of a list
 *  \warning `T` should be cheap to copy */
template <typename T>
T norm_squared(std::vector<T> const &vec) {
    T ret = 0;
    for (T element : vec) {
        ret += element * element;
    }
    return ret;
}

/*! \ingroup maths
    \brief Dot product
    \details This function has a soft assertion to check that the size of \p v1 and \p v2 match.
    \param v1
    \param v2
    \return \f$ v_1 \cdot v_2 = \sum_i v_{1,i}v_{2,i}\f$ */
template <typename T>
T inner_product(std::vector<T> const &v1, std::vector<T> const &v2) {
    T ret = 0;
    ASSERT_EQUAL(v1.size(), v2.size());
    Index const size = v1.size();
    for (Index i(0); i < size; ++i) {
        ret += v1[i] * v2[i];
    }
    return ret;
}

/*! \ingroup maths
    \brief Divides each entry by the greatest common divisor of all of the entries */
template <typename T>
void simplify_by_gcd(std::vector<T> &vec) {
    T the_gcd = vec[0];
    for (Index i : util::Range(Index(1), vec.size())) {
        the_gcd = std::gcd(the_gcd, vec[i]);
        if (the_gcd == 1)
            return;
    }

    for (T &vec_element : vec) {
        vec_element /= the_gcd;
    }
}

} // namespace util
