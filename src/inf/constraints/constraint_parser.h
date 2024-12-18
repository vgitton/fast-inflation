#pragma once

#include "constraint.h"

namespace inf {

/*! \ingroup infcons
 * \brief To convert a human-readable formulation of inflation constraints into the necessary internal format
 * \details
 * Recall that, in the paper, an inflation constraint is written as \f$(\infmarg_0,\dots,\infmarg_{k-1},\infmargg) \in \infconstraints\f$
 * for some \f$k \geq 1\f$, where \f$\infmarg_i,\infmargg \subset \infparties\f$ (where \f$\infparties\f$ denotes the set of inflation parties).
 * This will then imply a constraint of the form
 * \f$q_{\infmarg_0\cdots\infmarg_{k-1}\infmargg} = p_{\infmarg_0} \dots p_{\infmarg_{k-1}} \cdot q_{\infmargg}\f$
 * in the inflation problems we consider (i.e., "find an inflation distribution satisfying some symmetry requirements as well as this marginal constraint").
 * The class inf::ConstraintParser takes care of two things:
 * - It defines the format in which the user should enter inflation constraints, allowing to convert the user input into
 *   list of indices of inflation parties as defined in inf::Inflation.
 *   This is used in particular when initializing an inf::Constraint.
 * - It takes care of checking that the data that was entered makes sense as an inflation constraint.
 *   This essentially consists in checking that each \f$\infmarg_i\f$ is an injectable set (\f$\injsets\f$ in the paper),
 *   and that for each \f$\infmarg,\infmarg' \in \{\infmarg_0,\dots,\infmarg_{k-1},\infmargg\}\f$,
 *   \f$\infmarg\f$ is d-separated from \f$\infmarg'\f$, i.e., that there is no common parent source between \f$\infmarg\f$ and \f$\infmarg'\f$.
 *
 * ### Formatting of inflation constraints
 *
 * We represent this with the inf::Constraint::Description type.
 * For instance,
 * - \f$(\infmarg_0 = \{\alice_{00},\bob_{00},\charlie_{00}\},\infmarg_1 = \{\alice_{11},\bob_{11},\charlie_{11}\},\infmargg = \emptyset)\f$
 *   is represented as
 *   ```
 *   // Notice the last empty string, which represents the empty set!
 *   inf::Constraint::Description constraint_description = {"A00,B00,C00", "A11,B11,C11", ""};
 *   ```
 *   Spaces are ignored, so we can equivalently write this as
 *   ```
 *   inf::Constraint::Description constraint_description = {"A 0 0 , B 0 0  , C 00   ", "   A11  ,B11 ,C11", "  "};
 *   ```
 * - \f$(\infmarg_0 = \{\alice_{00},\bob_{00},\charlie_{00}\},\infmargg = \{\alice_{11},\bob_{11},\charlie_{11}\})\f$
 *   is represented as
 *   ```
 *   inf::Constraint::Description constraint_description = {"A00,B00,C00", "A11,B11,C11"};
 *   ```
 * */
class ConstraintParser {
  public:
    /*! \brief Initializes the information necessary to construct a inf::Constraint
     * \param inflation The inflation relative to which we are formulating the inflation constraints,
     * providing in particular the method `inf::Inflation::get_party_index(std::string const&) const`.
     * \param description See the inf::ConstraintParser documentation for the adequate format */
    ConstraintParser(inf::Inflation::ConstPtr const &inflation,
                     inf::Constraint::Description const &description);
    //! \cond
    ConstraintParser(ConstraintParser const &) = delete;
    ConstraintParser(ConstraintParser &&) = delete;
    ConstraintParser &operator=(ConstraintParser const &) = delete;
    ConstraintParser &operator=(ConstraintParser &&) = delete;
    //! \endcond

    /*! \brief Accepts a string of the form " A11, A22,B00 " and returns the corresponding list of inflation party indices
     * \details This method is static to be useable also without creating a full inf::ConstraintParser.
     * \param inflation The inflation relative to which we are formulating the inflation marginal,
     * providing in particular the method `inf::Inflation::get_party_index(std::string const&) const`.
     * \param name See the inf::ConstraintParser documentation for the adequate format */
    static std::vector<Index> parse_inflation_marginal(inf::Inflation const &inflation,
                                                       std::string const &name);

    /*! \brief The left-hand-side inflation marginal party indices, i.e., the party indices corresponding to the set
     * \f$\infmarg_0 \cup \dots \cup \infmarg_{k-1} \cup \infmargg\f$ */
    std::vector<Index> get_lhs_marg_parties() const;
    /*! \brief The right-hand-side inflation marginal party indices, i.e., the party indices corresponding to the set
     * \f$\infmargg\f$ */
    std::vector<Index> get_rhs_marg_parties() const;
    /*! \brief The list of target distribution marginals corresponding to each \f$\infmarg_i\f$
     * \details The idea is that each \f$\infmarg_i\f$ is an injectable set, so if we strip away
     * the copy indices from the party to just retain their type (Alice, Bor or Charlie),
     * we get something that defines an inf::TargetDistr::MarginalName.
     * This method allows to compute more easily the necessary marginals \f$\targetp_{\infmarg_i}\f$. */
    std::vector<inf::TargetDistr::MarginalName> get_target_marg_names() const;

  private:
    /*! \brief see get_lhs_marg_parties() */
    std::vector<Index> m_lhs_marginal_parties;
    /*! \brief see get_target_marg_names() */
    std::vector<inf::TargetDistr::MarginalName> m_target_marginal_names;
    /*! \brief see get_rhs_marg_parties() */
    std::vector<Index> m_rhs_marginal_parties;
};

} // namespace inf
