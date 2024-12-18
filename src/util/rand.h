#pragma once

#include <random>

/*! \file */

namespace util {

/*! \ingroup misc

    \brief A uniform random integer generator over a specified range.

    \warning Template instances of this class must be forward-declared in its source file!

    Usage:
    \code{.cpp}
    RNG<Index> rng(0,1);
    Index val = rng.get_rand();
    // A good Bayesian ought to believe that val is 0 or 1 with 50% probability:
    \endcode
    */
template <typename T>
class RNG {
  public:
    /*! \brief Constructor with min and max values
        \param min_value The minimal value that may be sampled
        \param max_value The maximal value that may be sampled */
    RNG(T min_value, T max_value);
    //! \cond
    RNG(RNG const &other) = delete;
    RNG(RNG &&other) = delete;
    RNG &operator=(RNG const &other) = delete;
    RNG &operator=(RNG &&other) = delete;
    //! \endcond

    /*! \brief Query the random number generator
        \return Uniformly sampled value between `min_value` and `max_value` (both included). */
    T get_rand();

  private:
    /*! \brief The source of randomness */
    std::default_random_engine m_generator;
    /*! \brief The distribution to sample from */
    std::uniform_int_distribution<T> m_distribution;
};

} // namespace util
