#pragma once

#include "../../util/loggable.h"
#include "../events/event.h"

/*! \file */

namespace inf {

/*! \ingroup symmetry
    \brief Party symmetry

    \details
    Consider a set of parties \f$\set A\f$, such as \f$\{0,1,2\}\f$ (contrarily to the paper, we will mostly label
    parties with indices for computational efficiency reasons).
    A party symmetry is a permutation \f$\sigma \in \symgroup{\set A}\f$. */
class PartySym : public util::Loggable {
  public:
    /*! \brief The image of the permutation, \f$(\sigma(0),\sigma(1),\dots)\f$ */
    typedef std::vector<Index> Bare;

    /*! \brief Constructor, can specify parity of the permutation
        \details The parity of the permutation is required to extend the action of the network party symmetries
        to inflation party symmetries.
        \param sym Vector representation of the image of the party indices under permutation
        \param even `true` if even parity (i.e., if the signature of the permutation is one) */
    explicit PartySym(inf::PartySym::Bare const &sym, bool even = true);
    PartySym(PartySym const &other) = default;
    PartySym(PartySym &&other) = default;
    PartySym &operator=(PartySym const &other) = default;
    PartySym &operator=(PartySym &&other) = default;

    /*! \brief %Symmetry action, acts on each party entrywise
     * \details The idea is that \p input should be thought of a list of parties (indices) from the set \f$\set A\f$
     * on which the inf::PartySym \f$\sigma \in \symgroup{\set A}\f$ acts.
     * Then, \f$\sigma\f$ is applied entrywise on the \p input, and the result is stored in \p output.
     * \note \p input and \p output are allowed to be pointing at the same memory location.
     * There is a soft assertion checking that the provided \p input and \p output vectors have the same size. */
    void act_entrywise(std::vector<Index> const &input, std::vector<Index> &output) const;

    /*! \brief %Symmetry action, essentially the action on events.
        \details The parameter \p input, being a list, should be thought of as a function \f$f : \set A \to T\f$,
        where \f$T\f$ is the set of objects of type `T` (the template parameter).
        Then, the symmetry action reads \f$\sigma(f) = f \circ \sigma^{-1}\f$.
        This describes for instance the action of party symmetries on events, denoted \f$\act{\sigma}{\events{\set A}}\f$ in the paper,
        but also the action of constraint symmetries on marginal permutations, denoted \f$\actt{\sigma}{\permutedmargs{A}}\f$ in the paper.
        \note `input` and `output` have to refer to different memory locations,
        and their size should match \f$|\set A|\f$, i.e., the size of the `sym` parameter passed to the constructor inf::PartySym::PartySym().
        This is soft-asserted. */
    template <typename T>
    void act_on_list(std::vector<T> const &input, std::vector<T> &output) const;

    /*! \brief The composed permutation \f$\sigma_\text{this} \circ \sigma_\text{other}\f$.
        \param other Applied before `*this` */
    PartySym get_composition_after(PartySym const &other) const;

    /*! \brief To distinguish even and odd permutations, needed for to extend network symmtries to inflation symmetries
        \return `true` if even permutation (signature is one) */
    bool is_even() const { return m_even; }

    /*! \brief Returns the image of the symmetry, i.e., \f$(\sigma(0),\sigma(1),\dots)\f$. */
    PartySym::Bare const &get_bare_sym() const { return m_sym; }
    /*! \brief Returns the inverse permutation's image (computed once upon construction), i.e., \f$(\sigma^{-1}(0),\sigma^{-1}(1),\dots)\f$. */
    PartySym::Bare const &get_inverse_bare_sym() const { return m_inverse_sym; }

    void log() const override;

    // Operator overloads

    bool operator==(PartySym const &other) const;

    /*! \brief Lexicographical comparison of `m_sym` */
    bool operator<(PartySym const &other) const;

  private:
    /*! \brief The image of the permutation, i.e., \f$(\sigma(0),\sigma(1),\dots)\f$ */
    PartySym::Bare m_sym;
    /*! \brief The image of the inverse of the permutation, i.e., \f$(\sigma^{-1}(0),\sigma^{-1}(1),\dots)\f$ */
    PartySym::Bare m_inverse_sym;
    /*! \brief `true` if permutation is even (i.e., signature is one) */
    bool m_even;
};

} // namespace inf
