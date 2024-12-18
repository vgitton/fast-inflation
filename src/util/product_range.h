#pragma once

#include "../types.h"

#include <vector>

/*! \file */

namespace util {

/*! \ingroup iteratorranges
 * \brief %Iterator over `std::vector<T>`, see util::ProductRange
 * \details This class should not be used directly, use util::ProductRange and range-based for loops instead */
template <typename T>
class ProductIterator {
  public:
    /*! \brief Initializes the internal vector to all zeros
        \param bases The idea is that each vector entry counts up in a separate base. */
    ProductIterator(std::vector<T> const &bases);
    //! \cond
    ProductIterator(ProductIterator const &other) = delete;
    ProductIterator(ProductIterator &&other) = delete;
    ProductIterator &operator=(ProductIterator const &other) = delete;
    ProductIterator &operator=(ProductIterator &&other) = delete;
    //! \endcond

    /*! \brief Increments the internal `std::vector<T>`.
        \return `*this` */
    util::ProductIterator<T> &operator++();
    /*! \brief To get the internal `std::vector<T>`. */
    std::vector<T> const &operator*() const;
    /*! \brief Returns `true` is more iterations are available */
    bool is_not_done() const { return m_is_not_done; }

  private:
    /*! \brief It is convenient to store the index of the last party, i.e., `bases.size()-1`, where `bases` was passed to the constructor */
    size_t m_last_party;
    /*! \brief The current list of numbers */
    std::vector<T> m_event;
    /*! \brief We want to allow for different number of outcomes for each party
     * \details This is the same as the `bases` parameter of the constructor, but
     * with each entry being one less than in `bases`. */
    std::vector<T> m_last_outcomes;
    /*! \brief Used to signal that the iteration is over (i.e., has reached back to `(0,0,...0)`) */
    bool m_is_not_done;
};

/*! \ingroup ranges
    \brief This class implements a range over `std::vector<T>`, akin to Python product ranges.

    \details Equivalently, this class allows to iterate over the range of a Cartesian product of finite sets.
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
    \endcode */
template <typename T>
class ProductRange {
  public:
    /*! \brief Product range with the same base for each entry of the list.
    \details This allows to iterate over the Cartesian product \f$\{0, \dots, \mathtt{base-1}\}^{\times \mathtt{n\_parties}} \f$.
    It is equivalent to `util::ProductRange({base,...,base})`.
    */
    ProductRange(size_t n_parties, T base);
    //! \cond
    ProductRange(ProductRange const &other) = delete;
    ProductRange(ProductRange &&other) = delete;
    ProductRange &operator=(ProductRange const &other) = delete;
    ProductRange &operator=(ProductRange &&other) = delete;
    //! \endcond

    /*! \brief To iterate over a Cartesian product with varying bases
        \param bases Local bases for counting up */
    ProductRange(std::vector<T> const &bases);

    /*! \brief Returns the beginning iterator in the state `(0,0,...,0)`. */
    util::ProductIterator<T> begin() const;
    /*! \brief End iterator, could be anything
        \details This is in fact a little hack that works in tandem with `operator!=(util::ProductIterator const& it, bool unused)`.
        \return The return value is ignored. */
    inline bool end() const { return true; }

  private:
    /*! \brief Stores the number of values that each element of the list will take. */
    const std::vector<T> m_bases;
};

} // namespace util

/*! \ingroup iteratorranges
 * \relates util::ProductIterator
   \brief Returns `true` if the iteration should continue
   \param it The iterator
   \param unused
   \return `true`, unless the internal event of `it` has gotten back to `(0,0,...,0)`. */
template <typename T>
bool operator!=(util::ProductIterator<T> const &it, bool unused) {
    (void)unused;
    return it.is_not_done();
}
