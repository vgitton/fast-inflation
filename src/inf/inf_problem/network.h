#pragma once

#include "../../types.h"
#include "../../util/loggable.h"
#include "../../util/range.h"
#include "../events/event.h"
#include "../symmetry/symmetry.h"

#include <map>
#include <memory>

/*! \file */

/*! \defgroup infpb Inflation problem specification
 *  \ingroup inflation
 *  \brief The classes relevant for specifying an inflation problem to test the nonlocality of a distribution
 *  \details
    This module is concerned with feasibility problems, and more precisely,
    with trying to prove the classical causal incompatibility of an inf::TargetDistr (target distribution) with the triangle network, represented as an inf::Network, by looking at inflation compatibility (with options such as inflation size and inflation constraints specified in inf::FeasOptions).
    These ingredients can then be passed to an inf::FeasProblem (inflation feasibility problem).
    Alternatively, the class inf::VisProblem provides a simple dichotomic search to find the nonlocality threshold of a one-parameter (typically visibility) family of distributions.
 * */

namespace inf {

/*! \ingroup infpb
    \brief The network with which the classical causal compatibility of a target distribution is to be tested
    \details
    This class should be instantiated first, as it is required to initialize an inf::TargetDistr.
    Currently, the only actual parameter is the number \f$\nouts\f$ of outcomes of each party.
    In the future, we may want to allow for a different number of outcomes for each party.

    ### A note on general network graphs

    This class is currently only meant to represent the triangle network.
    In particular, the method inf::Network::get_all_party_sym() is hard-coded
    for the triangle network (it is hard-asserted that the inf::Network is indeed a triangle network).
    Some of the other methods, such as inf::Network::get_event_range(), are already suitable
    for more general network graphs.
    Thus, this class is written in a way that would make the implementation of more general
    network graphs more natural.
*/
class Network : public util::Loggable {
  public:
    typedef std::shared_ptr<const inf::Network> ConstPtr;

    /*! \brief The constructor for the triangle network
        \param n_outcomes The number of outcome of each party, denoted \f$\nouts\f$ in the paper.
        The total number of outcome is thus `n_outcomes^3`. */
    static inf::Network::ConstPtr create_triangle(inf::Outcome n_outcomes);

    /*! \brief For now, use inf::Network::create_triangle() instead
        \param name The name of the network
        \param n_parties The number of parties
        \param party_names A list of names for the parties, represented as \f$\netparties\f$ in the paper. These must be
        one-character wide.
        \param n_outcomes The number of outcomes per party, denoted \f$\nouts\f$ in the paper */
    Network(std::string const &name,
            Index n_parties,
            std::vector<std::string> const &party_names,
            inf::Outcome n_outcomes);

    //! \cond ignore these
    Network(Network const &other) = delete;
    Network(Network &&other) = delete;
    Network &operator=(Network const &other) = delete;
    Network &operator=(Network &&other) = delete;
    //! \endcond

    void log() const override;

    /*! \brief Utility to print a network event, something like `0,0,2` */
    void log_event(inf::Event const &e) const;
    /*! \brief See inf::Network::log_outcome() */
    std::string outcome_to_str(inf::Outcome outcome) const;
    /*! \brief Log a colored outcome or as `?` if outcome is "unknown"
        \param outcome Can be in the range `0,...,inf::Network::get_n_outcomes()`, where the case `inf::Network::get_n_outcomes()` corresponds to the "unknown" outcome. */
    void log_outcome(inf::Outcome outcome) const;

    /*! \brief The total number `n_outcomes^3` of network events \f$\netevents\f$ */
    Index get_n_events() const;

    /*! \brief To iterate over all `n_outcomes^3` network events \f$\netevents\f$ */
    inf::EventRange get_event_range() const;

    /*! \brief The list of party names \f$\netparties\f$ (Alice, Bob and Charlie) */
    inline std::vector<std::string> const &get_party_names() const { return m_party_names; }
    /*! \brief Maps party names (Alice, Bob and Charlie) to their index (0,1 and 2)
     *  \details This is convenient for an inf::TargetDistr to be able to compute its marginals */
    inline std::map<std::string, Index> get_party_name_map() const { return m_party_name_map; }
    /*! \brief Number of parties, 3 for now */
    inline Index get_n_parties() const { return m_n_parties; }
    /*! \brief Range over the 3 parties */
    inline util::Range<Index> get_party_range() const { return util::Range(m_n_parties); }
    /*! \brief Number of outcome per party, typically 2, 3 or 4 */
    inline inf::Outcome get_n_outcomes() const { return m_n_outcomes; }
    /*! \brief The last outcome, `get_n_outcomes() - 1` */
    inline inf::Outcome get_outcome_last() const { return m_outcome_last; }
    /*! \brief The outcome labelling the "unknown" outcome.
        \return This is currently equivalent to `get_n_outcomes()`. */
    inline inf::Outcome get_outcome_unknown() const { return m_outcome_unknown; }
    /*! \brief The network name */
    inline std::string get_name() const { return m_name; }

    /*! \brief All party permutations that correspond to network symmetries
     * \details Currently manually gives the \f$3!\f$ permutations of \f$3\f$ parties,
            with parity specified.
     * \warning This is the inf::Network method that only makes sense for the triangle network,
     * and this would thus need to be adapted for more general networks.
     * For instance, for the square network, only cyclic permutations of parties correspond
     * to network graph automorphisms. */
    std::vector<inf::PartySym> get_all_party_sym() const;
    /*! \brief All joint outcome permutations, representing \f$\shortsymgroup{\nouts}\f$ */
    std::vector<inf::OutcomeSym> get_all_outcome_sym() const;
    /*! \brief All joint party and outcome symmetries of the network
        \return Set of composition of the elements of inf::Network::get_all_party_sym()
        and inf::Network::get_all_outcome_sym() */
    inf::Symmetry::Group get_all_sym() const;
    /*! \brief The trivial symmetry group of the right size
        \return The trivial group whose single element is the identity over the network parties and identity over the outcomes */
    inf::Symmetry::Group get_trivial_symmetry_group() const;

  private:
    /*! \brief The name of the network */
    const std::string m_name;
    /*! \brief The number of parties in the network */
    const Index m_n_parties;
    /*! \brief The list of party names, corresponds to the keys of `m_party_name_map` */
    std::vector<std::string> m_party_names;
    /*! \brief Maps the names (Alice, Bob, Charlie) of the parties to their indices (0, 1, 2) */
    std::map<std::string, Index> m_party_name_map;
    /*! \brief Number of outcomes per party
        \details In the future, this may become `const std::vector<inf::Outcome> m_n_outcomes` instead,
        to accomodate different number of outcomes per party */
    const inf::Outcome m_n_outcomes;
    /*! \brief The last outcome, so currently `m_n_outcomes-1` */
    const inf::Outcome m_outcome_last;
    /*! \brief The number labelling the unknown outcome, currently `m_n_outcomes` */
    const inf::Outcome m_outcome_unknown;
};

} // namespace inf
