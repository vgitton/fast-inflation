#pragma once

#include "../types.h"

#include <vector>

/*! \file */

/*! \defgroup ranges Ranges
    \ingroup util
    \brief Provides support for range-based for loops, mimicking Python syntax

    \warning Template instances of these classes must be forward-declared in their source files!

    ### Iterating over a finite integer range

    Use as
    \code{.cpp}
    for(Index i : util::Range(n))
    {
        std::cout << i << " ";
    }
    \endcode
    which prints `0 1 2 ... n-1`.
    We also provide functionality to iterate over the size of a `std::vector`, e.g.,
    \code{.cpp}
    std::vector<double> vec{1.1,2.2,3.3};
    for(Index i : util::Range(vec))
    {
        std::cout << vec[i] << " ";
    }
    \endcode
    which prints `1.1 2.2 3.3`.

    ### Product ranges

    Use as
    \code{.cpp}
    for(std::vector<size_t> const& event : util::ProductRange(3,4))
    {
        // event = {0,0,0}
        // event = {0,0,1}
        // ...
        // event = {3,3,3}
    }
    \endcode
    or more generally as
    \code{.cpp}
    for(std::vector<size_t> const& event : util::ProductRange({2,3,4}))
    {
        // event = {0,0,0}
        // event = {0,0,1}
        // ...
        // event = {1,2,3}
    }
    \endcode

    ### Permutation ranges

    Example use:
    \code{.cpp}
    for(std::vector<int> const& v : util::Permutations(3))
    {
        // v is successively
        // {0,1,2}
        // {0,2,1}
        // {1,0,2}
        // {1,2,0}
        // {2,0,1}
        // {2,1,0}
    }
    \endcode

    */

/*! \defgroup iteratorranges Iterators
 * \ingroup ranges
 * \brief The iterators supporting the ranges we define */

/*! \brief The namespace for general purpose code, not specifically related to inflation */
namespace util {

/*! \ingroup iteratorranges
 * \brief %Iterator over incrementable type `T`, see util::Range
 * \details This class should not be used directly, use range-based for loops with util::Range instead. */
template <typename T>
class Iterator {
  public:
    /*! \brief Iterator storing \p val as its current value */
    Iterator(T val);
    //! \cond
    Iterator(Iterator const &other) = delete;
    Iterator(Iterator &&other) = delete;
    Iterator &operator=(Iterator const &other) = delete;
    Iterator &operator=(Iterator &&other) = delete;
    //! \endcond

    /*! \brief Increments the internal value
        \return `*this` */
    util::Iterator<T> &operator++();
    /*! \brief Yields the internal value \return `m_val` */
    T operator*() const;

  private:
    /*! \brief The current value held by the iterator */
    T m_val;
};

/*! \ingroup ranges
    \brief Iterating over a finite integer range

    \details
    Use as
    \code{.cpp}
    for(Index i : util::Range(n))
    {
        std::cout << i << " ";
    }
    \endcode
    which prints `0 1 2 ... n-1`.
    We also provide functionality to iterate over the size of a `std::vector`, e.g.,
    \code{.cpp}
    std::vector<double> vec{1.1,2.2,3.3};
    for(Index i : util::Range(vec))
    {
        std::cout << vec[i] << " ";
    }
    \endcode
    which prints `1.1 2.2 3.3`. */
template <typename T>
class Range {
  public:
    /*! \brief Creates the range `(0,1,\dots,bound-1)`
        \param bound Excluded! */
    Range(T bound);
    /*! \brief Creates the range `(first,first+1,\dots,bound-1)`
        \param first Included
        \param bound Excluded! */
    Range(T first, T bound);
    //! \cond
    Range(Range const &other) = delete;
    Range(Range &&other) = delete;
    Range &operator=(Range const &other) = delete;
    Range &operator=(Range &&other) = delete;
    //! \endcond

    /*! \brief Beginning at the initial value, typically 0
        \return `util::Iterator<T>(first)` */
    util::Iterator<T> begin() const;
    /*! \brief Ending at the final value \p bound which will be excluded
        \return `util::Iterator<T>(bound)` */
    util::Iterator<T> end() const;

  private:
    /*! \brief The initial value, included in the range */
    T m_first;
    /*! \brief The final value, excluded from the range */
    T m_bound;
};

} // namespace util

/*! \ingroup iteratorranges
    \relates util::Iterator
    \brief Checks for inequality between the values held by `first` and `second`
    \param first
    \param second
    \return `first.m_val != second.m_val` */
template <typename T>
bool operator!=(util::Iterator<T> const &first, util::Iterator<T> const &second) {
    return *first != *second;
}
