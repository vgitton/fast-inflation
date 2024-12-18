#pragma once

#include <vector>

/*! \file */

namespace util {

/*! \ingroup iteratorranges
    \brief To iterate over permutations, see util::Permutations
    \details This class should not be used directly, it is used when using a util::Permutations object in a range-based for loop. */
template <typename T>
class PermIterator {
  public:
    /*! \brief Intializes to the identity permutation
        \param size The number of elements on which the permutation acts */
    PermIterator(T size);
    //! \cond
    PermIterator(PermIterator const &other) = delete;
    PermIterator(PermIterator &&other) = delete;
    PermIterator &operator=(PermIterator const &other) = delete;
    PermIterator &operator=(PermIterator &&other) = delete;
    //! \endcond

    /*! \brief Goes to the next permutation, and stores a flag that tells whether or not there are other permutations coming up.
        \return `*this` */
    util::PermIterator<T> &operator++();
    /*! \brief Yields the current permutation
        \return the current permutation */
    std::vector<T> const &operator*() const;
    /*! \brief Used to stop the iteration
        \return `true` if more iterations are available */
    bool is_not_done() const { return m_is_not_done; }

  private:
    /*! \brief The current permutation held by the iterator */
    std::vector<T> m_perm;
    /*! \brief used by util::PermIterator::is_not_done() */
    bool m_is_not_done;
};

/*! \ingroup ranges
    \brief To iterate over permutations
    \details Example use:
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
template <typename T>
class Permutations {
  public:
    /*! \brief To iterate over permutation of a given size
        \param size The number of elements on which the permutation acts */
    Permutations(T size) : m_size(size) {}
    //! \cond
    Permutations(Permutations const &other) = delete;
    Permutations(Permutations &&other) = delete;
    Permutations &operator=(Permutations const &other) = delete;
    Permutations &operator=(Permutations &&other) = delete;
    //! \endcond

    /*! \brief The return iterator begins to the identity permutation */
    util::PermIterator<T> begin() const;

    /*! \brief Trivial iterator end, could be anything
        \details This is in fact a little hack, works in tandem with `operator!=(util::PermIterator const& it, bool unused)`.
        \return This value is actually never looked at */
    bool end() const { return true; }

  private:
    /*! \brief The number of elements on which the permutation acts */
    const T m_size;
};

} // namespace util

/*! \relates util::PermIterator
 * \ingroup iteratorranges
    \brief To stop iterating over permutations
    \param it The iterator whose status util::PermIterator::is_not_done() is returned
    \param unused
    \return `true` unless we're back to the identity permutation */
template <typename T>
bool operator!=(util::PermIterator<T> const &it, bool unused) {
    (void)unused;
    return it.is_not_done();
}
