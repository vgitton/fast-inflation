#pragma once

#include "outcome_sym.h"
#include "party_sym.h"
#include <exception>
#include <set>

/*! \file */

/*! \defgroup symmetry Event symmetry
    \ingroup event

    \brief Symmetry actions and related data structures

*/

namespace inf {

/*! \ingroup symmetry
    \brief A pair of party symmetry and outcome symmetry
    \details This class allows to encode the network and inflation symmetries.
    The inflation symmetries are described in the paper as the group \f$\fullgroup\f$ with elements \f$\fullsymexpl\f$
    where \f$\sourcesym \in \sourcegroup\f$, \f$\partysym \in \partygroup\f$, \f$\outsym \in \outgroup\f$.
    The inf::Symmetry class allows to store the inflation party permutation \f$\act{\sourcesym}{\infparties} \circ \act{\partysym}{\infparties}\f$
    together with the outcome permutation \f$\outsym\f$.
*/
class Symmetry : public util::Loggable {
  public:
    /*! \brief Used to indicate that a symmetry group has changed */
    class SymmetriesHaveChanged : public std::exception {
      public:
        /*! \brief Indicates that symmetries have changed */
        virtual char const *what() const noexcept { return "Symmetries have changed!"; }
    };

    /*! \brief A set of symmetries, typically a group */
    typedef std::set<inf::Symmetry> Group;

    /*! \brief Default constructor, can then use the copy or move operators to set up the inf::Symmetry  */
    explicit Symmetry();
    /*! \brief Constructor that copies \p party_sym and \p outcome_sym */
    explicit Symmetry(inf::PartySym const &party_sym, inf::OutcomeSym const &outcome_sym);
    Symmetry(Symmetry &&other) = default;
    Symmetry &operator=(Symmetry &&other) = default;
    Symmetry(Symmetry const &other) = default;
    Symmetry &operator=(Symmetry const &other) = default;

    /*! \brief %Action on symmetry on event, essentially \f$\sigma_\text{outcome} \circ \infevent \circ \sigma^{-1}_\text{party}\f$.
        \details Checking the parameters is delayed to the underlying inf::PartySym::act_on_list() and
        inf::OutcomeSym::act_on_event() that perform their own tests.
        \param input Input of symmetry action
        \param output Output of symmetry action. */
    void act_on_event(inf::Event const &input, inf::Event &output) const;

    /*! \brief getter \return Underlying inf::PartySym */
    inf::PartySym const &get_party_sym() const { return m_party_sym; }
    /*! \brief getter \return Underlying inf::OutcomeSym */
    inf::OutcomeSym const &get_outcome_sym() const { return m_outcome_sym; }

    void log() const override;

    /*! \brief Composition \f$ \sigma_\text{this} \circ \sigma_\text{other} \f$
        \param other Applied before `*this` */
    inf::Symmetry get_composition_after(inf::Symmetry const &other) const;

    /*! \brief Checks for equality of both the underlying inf::PartySym and the inf::OutcomeSym
        \details This one is needed to be able to check equality between symmetry groups */
    bool operator==(inf::Symmetry const &other) const;

    /*! \brief Lexicographical comparison of the inf::OutcomeSym first, and then of the inf::PartySym */
    bool operator<(inf::Symmetry const &other) const;

  private:
    /*! \brief The party symmetry */
    inf::PartySym m_party_sym;
    /*! \brief The outcome symmetry */
    inf::OutcomeSym m_outcome_sym;
};

} // namespace inf

// For some obscure reason, the non-member version doesn't work with the stl algo for comparing vectors...
// this is actually discussed a bit in https://stackoverflow.com/questions/51447860/comparison-operator-for-stdvectort-fails-to-find-comparison-operator-for-t
// bool operator==(inf::Symmetry const& sym_1, inf::Symmetry const& sym_2);
