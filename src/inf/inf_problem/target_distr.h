#pragma once

#include "../symmetry/symmetry.h"
#include "../symmetry/tensor_with_orbits.h"
#include "network.h"

#include <set>

/*! \file */

namespace inf {

/*! \ingroup infpb
 * \brief A distribution over an inf::Network
 * \details
 * In the notation of the paper, class instances represent a distribution \f$\targetp \in \targetps\f$.
 * This class allows the user to specify a target distribution over the triangle network,
 * i.e., a distribution with support on a triangle network (represented as an inf::Network).
 * It is then possible to test whether or not the distribution is causally compatible with
 * the classical triangle network using an inf::FeasProblem instance.
 *
 * ### A note on exact arithmetic
 *
 * This class uses exact arithmetic for the probabilities.
 * This requires the user to first choose a distribution with rational components,
 * i.e., such that \f$\targetp(\infevent) \in \mathbb{Q}\f$ for all \f$\infevent\in\netevents\f$.
 * and then to find an adequate common denominator for all probabilities.
 * It is thus not possible to test exactly the nonlocality of a distribution that has irrational
 * components with the present code. (In principle, this could be achieved, but would require
 * fancier symbolic arithmetic capabilities.) */
class TargetDistr : public inf::TensorWithOrbits {
  public:
    /*! \brief A marginal name is a list of the form `{"A", "B"}` */
    typedef std::vector<std::string> MarginalName;
    typedef std::shared_ptr<const inf::TargetDistr> ConstPtr;

    /*! \brief Checks nonnegativity and normalization of the tensor as if it was meant to be a probability distribution
        \return `true` if the event tensor's numerators num up to its denominator and all components are nonnegative */
    static bool is_probability_distribution(inf::EventTensor const &event_tensor);

    /*! \brief Construct a distribution over an inf::Network, specifying its name and its components
        \details
        This constructor requires fixing the denominator in advance,
        and specifying the numerators of the various probabilities.
        \param name Used for display purposes, e.g., "Elegant Joint Measurement".
        \param short_name Use for display purposes. It is recommended to keep this a few characters long only, e.g., "EJM"
        \param net The underlying inf::Network, which currently mostly specifies the number of outcomes per party.
        \param data This inf::EventTensor specifies the probabilities as rational numbers with a common denominator.
        The number of outcomes and number of parties of \p net and \p data are asserted to match. */
    TargetDistr(std::string const &name,
                std::string const &short_name,
                inf::Network::ConstPtr const &net,
                inf::EventTensor const &data);
    //! \cond ignore deleted
    TargetDistr(TargetDistr const &other) = delete;
    TargetDistr(TargetDistr &&other) = delete;
    TargetDistr &operator=(TargetDistr const &other) = delete;
    TargetDistr &operator=(TargetDistr &&other) = delete;
    //! \endcond

    /*! \brief Returns the underlying inf::Network */
    inf::Network::ConstPtr const &get_network() const;

    /*! \brief Get the marginal distribution corresponding to the parties specified in the \p name.
     *  \details The marginals are computed the first time they're required, then cached.
     *
     *  This functionality is tested in the application user::distribution_marginal.
     *  \param name This needs to be a subset of the network party names, ordered in the same way that the network party names are ordered.
     *  E.g., if the network names are `{"A","B","C"}`, a valid marginal name is `{"A","C"}` but not `{"C","A"}`.
     *  This is asserted.
     *  \return Reference to an inf::EventTensor --- the reference is valid until the inf::TargetDistr is. */
    inf::EventTensor const &get_marginal(inf::TargetDistr::MarginalName const &name) const;

    /*! \brief The name of the distribution */
    std::string const &get_name() const;
    /*! \brief The short name of the distribution */
    std::string const &get_short_name() const;

    // Overrides from inf::TensorWithOrbits

    std::string get_math_name() const override;
    void log_header() const override;

    // Overrides from inf::Orbitable

    void log_event(inf::Event const &event) const override;

  private:
    /*! \brief Name of the distribution, for logging purposes */
    const std::string m_name;
    /*! \brief Short name of the distribution, for logging purposes */
    std::string const m_short_name;
    /*! \brief Underlying inf::Network */
    const inf::Network::ConstPtr m_network;

    /*! \brief Check whether a network symmetry is a distribution symmetry
        \param network_sym A symmetry that ought to be compatible with the network topology,
        but may or may not be a symmetry of the inf::TargetDistr
        \return `true` if \p network_sym is an actual symmetry of the inf::TargetDistr, i.e., if applying the symmetry
        on the distribution leaves it invariant */
    bool is_symmetrized_by(inf::Symmetry const &network_sym) const;

    /*! \brief Initialize the underlying symmetry group, and calls inf::Orbitable::init_orbits() */
    void init_symmetries();

    // Marginals related things

    /*! \brief Stores the marginals. Dictionary is initially empty.
     *  \details This is the cache used by inf::TargetDistr::get_marginal().
     *  It is `mutable` because computing a marginal of a distribution does not modify the distribution */
    mutable std::map<inf::TargetDistr::MarginalName, inf::EventTensor::UniqueConstPtr> m_marginals;
    /*! \brief This function computes the marginals and returns them, without looking at the cache `m_marginals`.
     *  \param name This identifies the marginal */
    inf::EventTensor::UniqueConstPtr compute_marginal(inf::TargetDistr::MarginalName const &name) const;
};

} // namespace inf
