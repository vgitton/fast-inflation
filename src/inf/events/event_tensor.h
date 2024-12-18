#pragma once

#include "../../util/frac.h"
#include "event.h"
#include <memory>

/*! \file */

/*! \defgroup eventstorage Event storage
 * \ingroup event
 * \brief Event trees and event tensors */

namespace inf {

/*! \ingroup eventstorage
    \brief Stores a function from a set of inf::Event of fixed length to ::Num.
    \details Conceptually, this plays the role of a function \f$v \in \vecset{\set A}\f$ for some set \f$\set A\f$,
    i.e., a function \f$v : \events{\set A} \to \R\f$, except that we restrict the images of \f$v\f$ to rational numbers.
    The idea of the inf::EventTensor is that it stores rational numbers expressed with the same denominator.
    Thus, it is mostly a function from inf::Event to ::Num, but additionally provides a denominator (see inf::EventTensor::get_denom())
    and some rational arithmetic capabilities such as inf::EventTensor::simplify().

    Example use:
    \code{.cpp}
    inf::EventTensor T(3,4);

    T.get_num({0,0,0}) = 1;
    T.get_num({0,0,1}) = 2;

    // Now, T contains
    // {0,0,0} -> 1
    // {0,0,1} -> 2
    // {0,0,2} -> 0
    // {0,0,3} -> 0
    // ...
    // {3,3,3} -> 0
    \endcode

    This class:
    - is used over std::map because the latter was not so efficient (accessing an item takes longer).
    - is used internally by inf::TensorWithOrbits (and so by inf::TargetDistr and inf::DualVector).
    - is used by the end-user to specify the components of an inf::TargetDistr.
    - is tested in the user::event_tensor application.
    */
class EventTensor {
  public:
    /*! \brief Internally, the inf::EventTensor assigns a ::Num to an inf::Event by first hashing it.  */
    typedef Index EventHash;
    typedef std::unique_ptr<inf::EventTensor> UniquePtr;
    typedef std::unique_ptr<const inf::EventTensor> UniqueConstPtr;

    /*! \brief Constructor specifying the number of parties and number of outcomes per party
        \param n_parties Number of parties, i.e., length of the ::inf::Event keys. This is allowed to be zero
        to be able to represent a scalar, in which case \p base is irrelevant.
        \param base E.g., if we want to store keys from {0,0,0} to {3,3,3}, we set `base = 4`. */
    EventTensor(Index const n_parties,
                inf::Outcome const base);

    EventTensor(EventTensor const &other) = default;
    EventTensor(EventTensor &&other) = default;
    EventTensor &operator=(EventTensor &&other) = default;
    EventTensor &operator=(EventTensor const &other) = default;

    /*! \brief Returns true if the inf::EventTensor is scalar, i.e., is a function over inf::Events of zero length (recall that we define \f$\vecset{\emptyset} = \R\f$). */
    bool is_scalar() const;

    /*! \brief Logs a string of the form "p(000) = 1"`
        \param math_name "p"
        \param first_element For line breaks
        \param count For line breaks
        \param items_per_line For line breaks
        \param event The event whose value in the inf::EventTensor will be printed */
    void log_single_event(
        std::string const &math_name,
        bool &first_element,
        Index &count,
        Index const items_per_line,
        inf::Event const &event) const;
    /*! \brief Logs the contents of the tensor in the format `math_name(000) = 1, ...` */
    void log(std::string const &math_name) const;

    /*! \brief Const getter from hash
        \details The `hash` should be obtained with inf::EventTensor::get_event_hash().
        However, one can simply use the overload inf::EventTensor::get_num(inf::Event const& event) const instead.
        Computing the hash by hand can occasionnally be a bit more efficient than constructing an intermediate inf::Event.
        \param hash The hash (key) of an inf::Event
        \return The ::Num associated to the key */
    Num get_num(inf::EventTensor::EventHash hash) const;
    /*! \brief Getter from hash, allows for modifying the returned ::Num
        \details The `hash` should be obtained with inf::EventTensor::get_event_hash().
        However, one can simply use the overload inf::EventTensor::get_num(inf::Event const& event) instead.
        Computing the hash by hand can occasionnally be a bit more efficient than constructing an intermediate inf::Event.

        To modify the ::Num of a hash, use the following syntax:
        \code{.cpp}
        inf::EventTensor T(3,4);
        Index const hash = T.get_event_hash({0,0,0});
        T.get_num(hash) = 1;
        \endcode

        \param hash The hash (key) of an inf::Event
        \return The ::Num associated to the key */
    Num &get_num(inf::EventTensor::EventHash const hash);
    /*! \brief Const getter from inf::Event.
     * \details Conceptually, thinking of the inf::EventTensor as some \f$v \in \vecset{\set A}\f$ for some set \f$\set A\f$,
     * this returns \f$v(\infevent)\f$ for some \f$\infevent\in\events{\set A}\f$.
        \param event The key inf::Event
        \return The ::Num associated to `event` */
    Num get_num(inf::Event const &event) const;
    /*! \brief Getter from inf::Event
        \details
        To modify the ::Num associated to an event, use the following syntax:
        \code{.cpp}
        inf::EventTensor T(3,4);
        T.get_num({0,0,0}) = 1;
        \endcode

        \param event The key inf::Event
        \return The ::Num associated to `event` */
    Num &get_num(inf::Event const &event);
    /*! \brief To get the fraction associated to the hash of the event, including the denominator of the inf::EventTensor
        \details The `hash` should be obtained with inf::EventTensor::get_event_hash().
        However, one can simply use the overload inf::EventTensor::get_frac(inf::Event const& event) const instead.
        Computing the hash by hand can occasionnally be a bit more efficient than constructing an intermediate inf::Event. */
    util::Frac get_frac(inf::EventTensor::EventHash hash) const;
    /*! \brief To get the fraction associated to the event, including the denominator of the inf::EventTensor */
    util::Frac get_frac(inf::Event const &event) const;
    /*! \brief Returns `true` if the `*this` and `other` have the same number of parties and number of outcomes per party */
    bool has_same_shape_as(inf::EventTensor const &other) const;
    /*! \brief Divides the common denominator and all of the numerators by the GCD of them all */
    void simplify();

    /*! \brief Used to compute hashes for the inf::EventTensor */
    static std::vector<Index> compute_weights(Index const n_parties, Index const base);
    /*! \brief Returns a view over the party weights used to compute hashes inf::EventTensor */
    std::vector<Index> const &get_party_weights() const;

    /*! \brief Returns the common denominator of all of the entries of the inf::EventTensor */
    Num get_denom() const;
    /*! \brief Set the denominator, soft assertion preventing to set it to zero */
    void set_denom(Num const denom);

    /*! \brief The length of each inf::Event to which the inf::EventTensor assigns values */
    Index get_n_parties() const;
    /*! \brief The number of values/outcomes of each inf::Event to which the inf::EventTensor assigns values */
    inf::Outcome get_base() const;

    /*! \brief To iterate over all hashes of the inf::EventTensor
     * \return `util::Range(m_data.size())` */
    util::Range<inf::EventTensor::EventHash> get_hash_range() const;
    /*! \brief The inf::EventRange over which the inf::EventTensor is valued */
    inf::EventRange get_event_range() const;

    /*! \brief Sets the current tensor to the tensor product of the elements of \p factors, and calls inf::EventTensor::simplify()
     *  \details Before calling this method, make sure that `this` has the right size:
     *  there will be a soft assertion `ASSERT_EQUAL(this->get_n_parties(), sum_factors_n_parties)`, where `sum_factors_n_parties` is the sum over \p factor in \p factors of `factor.get_n_parties()`.
     *  Furthermore, `this` and all \p factor in \p factors should have the same base.
     *  \note This method is not meant to be particularly fast.
     *  \param factors The pointers are soft-asserted to be non-nullptr */
    void set_to_tensor_product(std::vector<inf::EventTensor const *> const &factors);

    /*! \brief Hashes an inf::Event
     * \details Internally, each inf::Event is not direclty associated to a ::Num.
     * Instead, the unique hash of each inf::Event is associated to a ::Num. */
    inf::EventTensor::EventHash get_event_hash(inf::Event const &event) const;

  private:
    /*! \brief The length of the inf::Event keys */
    Index m_n_parties;
    /*! \brief The bound (excluded) on the entries of the inf::Event keys */
    inf::Outcome m_base;
    /*! \brief The data: accessing this data requires hashing the inf::Events keys first.
        \sa inf::EventTensor::get_event_hash() to hash an inf::Event */
    std::vector<Num> m_data;
    /*! \brief Used to quickly compute the hash of an event.
     * \details This stores `{1, base, base^2, ..., base^{n_parties-1}}`. */
    std::vector<Index> m_weights;
    /*! \brief The denominator, meant to be te denominator of all of `m_data`
     * \sa inf::EventTensor::get_denom(), inf::EventTensor::simplify() */
    Num m_denom;
};

} // namespace inf
