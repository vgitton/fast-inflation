#pragma once

#include "../../util/loggable.h"
#include "../events/event.h"

#include <memory>

/*! \file */

namespace inf {

/*! \ingroup symmetry
    \brief Outcome symmetry
    \details
    Recall that we work with a fixed number of outcomes per party labeled \f$\nouts\f$.
    An outcome symmetry is simply a permutation \f$\sigma \in \shortsymgroup\nouts\f$,
    which implies a global simultaneous outcome symmetry on events.
    In the future, this could be replaced with party-dependent outcome symmetries for
    enhanced performance wiht distributions having this more general kind of symmetries.
*/
class OutcomeSym : public util::Loggable {
  public:
    /*! \brief The image of the symmetry \f$(\sigma(0),\dots,\sigma(\nouts-1))\f$, where \f$\nouts\f$ is the number of outcomes */
    typedef std::vector<inf::Outcome> Bare;
    /*! \brief Constructor from image of the permutation
        \param sym Vector representation of image of outcomes under permutation, i.e., \f$(\sigma(0),\dots,\sigma(\nouts-1))\f$. */
    explicit OutcomeSym(inf::OutcomeSym::Bare const &sym);
    OutcomeSym(inf::OutcomeSym const &other) = default;
    OutcomeSym(inf::OutcomeSym &&other) = default;
    inf::OutcomeSym &operator=(inf::OutcomeSym const &other) = default;
    inf::OutcomeSym &operator=(inf::OutcomeSym &&other) = default;

    /*! \brief %Symmetry action on inflation event
     * \details The \p input event should be thought of as a function \f$f : \set A \to \outset\f$,
     * where \f$\set A\f$ is a set of parties (represented as `(0,1,\dots)`).
     * The action of outcome symmetries then reads \f$\sigma(f) = \sigma \circ f\f$.
     * \note \p input and \p output must have the same size (this is soft asserted), and are allowed to point to the same memory location.
        \param input The input event
        \param output The event after modification */
    void act_on_event(inf::Event const &input, inf::Event &output) const;

    /*! \brief The composition \f$\sigma_\text{this} \circ \sigma_\text{other}\f$
        \param other Applied before `*this` */
    inf::OutcomeSym get_composition_after(inf::OutcomeSym const &other) const;

    /*! \brief The inverse permutation \f$\sigma^{-1}\f$ */
    inf::OutcomeSym get_inverse() const;

    /*! \brief Get access to the underlying raw permutation */
    inf::OutcomeSym::Bare const &get_bare_sym() const { return m_sym; }
    /*! \brief Returns the inverse permutation's image (computed once upon construction), i.e., \f$(\sigma^{-1}(0),\sigma^{-1}(1),\dots)\f$. */
    inf::OutcomeSym::Bare const &get_inverse_bare_sym() const { return m_inverse_sym; }

    void log() const override;

    // Operator overloads

    bool operator==(inf::OutcomeSym const &other) const;

    /*! \brief Lexicographical comparison
        \details This is used in particular to be able to put permutations in a std::set */
    bool operator<(inf::OutcomeSym const &other) const;

    /*! \brief Returns true if the permutation is the identity (over its respective outcome set) */
    bool is_trivial() const;

  private:
    /*! \brief The image of the symmetry \f$(\sigma(0),\dots,\sigma(\nouts-1))\f$, where \f$\nouts\f$ is the number of outcomes */
    inf::OutcomeSym::Bare m_sym;
    /*! \brief The image of the inverse of the permutation, i.e., \f$(\sigma^{-1}(0),\sigma^{-1}(1),\dots)\f$ */
    inf::OutcomeSym::Bare m_inverse_sym;
};

} // namespace inf
