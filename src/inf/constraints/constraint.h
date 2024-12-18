#pragma once

#include "dual_vector.h"
#include "marginal.h"

namespace inf {

/*! \ingroup infcons
 * \brief Describes a single constraint of the form \f$\infq_{\infmarg_{0}\cdots\infmarg_{k-1}\infmargg} = \targetp_{\infmarg_{0}} \cdots \targetp_{\infmarg_{k-1}}\cdot \infq_{\infmargg}\f$
 * \details Recall that an inflation problem is essentially of the following form: given a target distribution \f$\targetp \in \targetps\f$ (whose nonlocality we want to test), find an inflation distribution \f$\infq\in\infqs\f$ over the inflation graph satisfying some symmetry requirements and some constraints.
 * The constraints are specified as a set \f$\constraintlist \subset \infconstraints\f$ as described in inf::ConstraintSet.
 * Each constraint is of the form \f$\constraintname = (\infmarg_{0},\dots,\infmarg_{k-1},\infmargg) \in \constraintlist\f$,
 * where \f$\infmarg_i,\infmargg \subset \infparties\f$ satisfy some properties described and checked in inf::ConstraintParser.
 * The idea is that the properties satisfied by these sets of inflation parties imply that imposing the constraint \f$\infq_{\infmarg_{0}\cdots\infmarg_{k-1}\infmargg} = \targetp_{\infmarg_{0}} \cdots \targetp_{\infmarg_{k-1}}\cdot \infq_{\infmargg}\f$ yields a valid relaxation of the local set of the relevant inf::Network.
 *
 * As described in the paper, the constraint \f$\infq_{\infmarg_{0}\cdots\infmarg_{k-1}\infmargg} = \targetp_{\infmarg_{0}} \cdots \targetp_{\infmarg_{k-1}}\cdot \infq_{\infmargg}\f$ ends up being represented as \f$\totconstraintmapelem(\infq) = 0\f$,
 * where \f$\totconstraintmapelem : \vecset{\infparties} \to \quovecspace{\infmarg_{0}\cdots\infmarg_{k-1}\infmargg}\f$.
 * To check nonlocality certificates and enable the Frank-Wolfe algorithm that finds such nonlocality certificates, we need to be able to evaluate inner products of the form \f$\inner{\quovec}{\totconstraintmapelem(\detdistr\infevent)}\f$, where \f$\quovec \in \quovecspace{\infmarg_{0}\cdots\infmarg_{k-1}\infmargg}\f$ is a dual vector (which is reduced/quotiented with respect to the relevant symmetries, we call this an inf::Quovec in the code),
 * and where \f$\detdistr\infevent \in \infqs\f$ is the deterministic inflation distribution with support on the inflation event \f$\infevent \in \infevents\f$.
 *
 * The class inf::Constraint provides the means necessary to evaluate such inner products.
 * Internally, it requires two inf::Marginal, one (`m_lhs_inflation_marginal`) to represent the left-hand-side inflation marginal \f$\infq_{\infmarg_0\cdots\infmarg_{k-1}\infmargg}\f$ and one (`m_rhs_inflation_marginal`) to represent the right-hand-side inflation marginal \f$\infq_{\infmargg}\f$.
 * Furthermore, it stores the current dual vector \f$\quovec\f$ as the inf::DualVector `m_lhs_dual_vector`.
 * It also stores an inf::EventTensor, `m_rhs_target_tensor`, representing \f$\targetp_{\infmarg_0}\cdots\targetp_{\infmarg_{k-1}}\f$.
 * As described in the paper, we can more efficiently evaluate the right-hand-side term of the inner product using the dual vector \f$\tilde\quovec\in\quovecspace{\infmargg}\f$, stored as `m_rhs_reduced_dual_vector`, which represents the contraction of \f$\targetp_{\infmarg_0}\cdots\targetp_{\infmarg_{k-1}}\f$ with \f$\altquovecembed{\infmarg_0\cdots\infmarg_{k-1}\infmargg}(\quovec)\f$.
 */
class Constraint : public util::Loggable {
  public:
    typedef std::unique_ptr<inf::Constraint> UniquePtr;

    /*! \brief The text description of an inflation constraint, see inf::ConstraintParser for the adequate format */
    typedef std::vector<std::string> Description;

    /*! \brief To print nicely an inf::Constraint::Description
     * \details We copy the argument to more conveniently remove whitespace
     * \param description E.g., `{"A00,B00,C00", "A11,B11,C11", ""}`
     * \return E.g., `q(A00,B00,C00 , A11,B11,C11) = p(A00,B00,C00) * p(A11,B11,C11)` */
    static std::string pretty_description(inf::Constraint::Description description);

    /*! \brief This computes the group \f$\marggroup\f$, the subgroup of the inflation symmetries \f$\distrgroup\f$ (see inf::Inflation::get_inflation_symmetries()) that leaves both \f$\infmarg_0\cup\cdots\cup\infmarg_{k-1}\f$ and \f$\infmargg\f$ invariant
     * \details This is a static method because it is also used directly in some tests.
     * \param inflation The relevant inflation
     * \param lhs_marg This describes \f$\infmarg_0\cup\cdots\cup\infmarg_{k-1}\f$ relative to the inflation party indices (see inf::Inflation::get_party_index())
     * \param rhs_marg This describes \f$\infmargg\f$ relative to the inflation party indices (see inf::Inflation::get_party_index())
     * \return The group \f$\marggroup\subset\distrgroup\f$ */
    static inf::Symmetry::Group get_constraint_group(inf::Inflation const &inflation,
                                                     std::vector<Index> const &lhs_marg,
                                                     std::vector<Index> const &rhs_marg);

    /*! \brief Initialize a constraint based on a text description
     *  \param inflation The inflation relative to which the constraint is formulated
     *  \param description The description of the constraint in the format described in the inf::ConstraintParser documentation
     *  \param store_bounds This allows to set up each internal inf::DualVector adequately.
     *  If we store the bounds, we need to allow the inf::DualVector to be valued over inflation events with unknowns,
     *  see inf::DualVector::StoreBounds for more details. */
    Constraint(inf::Inflation::ConstPtr const &inflation,
               inf::Constraint::Description const &description,
               inf::DualVector::StoreBounds store_bounds);
    //! \cond
    Constraint(Constraint const &other) = delete;
    Constraint(Constraint &&other) = delete;
    Constraint &operator=(Constraint const &other) = delete;
    Constraint &operator=(Constraint &&other) = delete;
    //! \endcond

    void log() const override;

    // Getters

    /*! \brief Returns \f$\quovec \in \quovecspace{\infmarg_0\cdots\infmarg_{k-1}\infmargg}\f$, the underlying left-hand-side inf::DualVector `m_lhs_dual_vector` */
    inf::DualVector const &get_dual_vector() const;
    /*! \brief The size \f$|\events{\infmarg_0\cdots\infmarg_{k-1}\infmargg}\setminus\marggroup|\f$, the number of elements in a quovec
     * \details No matter the value of inf::DualVector::StoreBounds, the value that is returned here does not take into account the marginal events with unknowns.
     * */
    Index get_quovec_size() const;

    /*! \brief This allows to more conveniently return two inf::Marginal::Evaluator in inf::Constraint::get_marg_evaluators() */
    typedef std::pair<inf::Marginal::Evaluator, inf::Marginal::Evaluator> EvaluatorPair;
    /*! \brief This returns the inf::Marginal::Evaluator for the left-hand-side marginal \f$\infq_{\infmarg_0\cdots\infmarg_{k-1}\infmargg}\f$
     * and the right-hand-side marginal \f$\infq_{\infmargg}\f$. */
    inf::Constraint::EvaluatorPair get_marg_evaluators() const;

    std::string const &get_pretty_description() const;

    // Arithmetic

    /*! \brief The left-hand-side denominator, i.e., \f$|\redpermutedmargs{\infmarg_0\cdots\infmarg_{k-1}\infmargg}|\f$,
     * see inf::Marginal::get_denom() */
    Num get_lhs_denom() const;
    /*! \brief Sets the left-hand-side scale factor, denoted \f$\consscale\f$ in the documentation of inf::Marginal */
    void set_lhs_scale(Num scale);
    /*! \brief The right-hand-side denominator, i.e., the denominator of `m_rhs_target_tensor` times \f$|\redpermutedmargs{\infmargg}|\f$,
     * see inf::Marginal::get_denom() */
    Num get_rhs_denom() const;
    /*! \brief Sets the right-hand-side scale factor, denoted \f$\consscale\f$ in the documentation of inf::Marginal */
    void set_rhs_scale(Num scale);

    // Setters

    /*! \brief To modify the target distribution \f$\targetp\f$. This updates `m_rhs_target_tensor` and `m_rhs_reduced_dual_vector`.
     * \details It is asserted that the new distribution \p d has symmetries compatible with the inf::Inflation that was passed
     * to the constructor of the inf::Constraint, see inf::Inflation::has_symmetries_compatible_with()
     * \param d The distribution \f$\targetp \in \targetps\f$ */
    void set_target_distribution(inf::TargetDistr const &d);
    /*! \brief Sets the left-hand-side inf::DualVector `m_lhs_dual_vector` and update `m_rhs_reduced_dual_vector` appropriately
     * \details Recall that an inf::DualVector represents primarily the quovec \f$\quovec \in \quovecspace{\infmarg_{0}\cdots\infmarg_{k-1}\infmargg}\f$,
     * which is what is being set with this method,
     * but it additionally stores \f$\altquovecembed{\infmarg_{0}\cdots\infmarg_{k-1}\infmargg} \in \vecset{\infmarg_0\cdots\infmarg_{k-1}\infmargg}\f$.
     * \param coeffs Contains the desired quovec as a sub-vector, starting at \p start_pos
     * \param start_pos */
    void set_dual_vector_from_quovec(inf::Quovec const &coeffs, const Index start_pos);

    // Evaluation on inflation event

    /*! \brief This returns \f$\totconstraintmapelem(\detdistr\infevent)\f$ where \f$\infevent\in\infevents\f$ is passed as \p inflation_event.
        \param inflation_event The inflation event \f$\infevent\in\infevents\f$ that describes a deterministic inflation distribution \f$\detdistr\infevent\f$
        \param ret The return quovec is stored there, starting at position \p offset
        \param offset */
    void compute_inflation_event_quovec(inf::Event const &inflation_event,
                                        inf::Quovec &ret,
                                        const Index offset) const;

    // Serialization

    /*! \brief This implements the I/O mechanism for the underlying inf::DualVector */
    void io_dual_vector(util::FileStream &stream);

  private:
    std::string const m_pretty_description;
    /*! \brief The underlying inflation */
    inf::Inflation::ConstPtr m_inflation;

    /*! \brief The group \f$\marggroup\f$ of constraint symmetries */
    inf::Symmetry::Group m_constraint_group;

    /*! \brief The left-hand-side inflation marginal \f$\infq_{\infmarg_0\cdots\infmarg_{k-1}\infmargg}\f$ */
    inf::Marginal::UniquePtr m_lhs_inflation_marginal;
    /*! \brief The right-hand-side inflation marginal \f$\infq_{\infmargg}\f$ */
    inf::Marginal::UniquePtr m_rhs_inflation_marginal;

    /*! \brief This boolean is there to make sure that the user of the class calls inf::Constraint::set_target_distribution() at least once */
    bool m_target_distribution_fixed;
    /*! \brief This stores the copy-indices-stripped marginals \f$\infmarg_0,\dots,\infmarg_{k-1}\f$, e.g., `{{"A","B","C"},{"B"}}`
     * \details This then allows to retrieve the target distribution marginal \f$p_{\infmarg_i}\f$
     * \sa inf::TargetDistr::get_marginal() */
    std::vector<inf::TargetDistr::MarginalName> m_target_marginal_names;
    /*! \brief This stores \f$\targetp_{\infmarg_0}\cdots\targetp_{\infmarg_{k-1}}\f$ */
    inf::EventTensor::UniquePtr m_rhs_target_tensor;

    /*! \brief The dual vector to be evaluated on `m_lhs_marginal` */
    inf::DualVector::UniquePtr m_lhs_dual_vector;
    /*! \brief The dual vector to be evaluated on `m_rhs_marginal`. It store the partial contraction of `m_lhs_dual_vector` with `m_rhs_target_tensor` */
    inf::DualVector::UniquePtr m_rhs_reduced_dual_vector;
    /*! \brief This contracts `m_lhs_dual_vector` and `m_rhs_target_tensor` and places the result in `m_rhs_reduced_dual_tensor` */
    void update_rhs_reduced_dual_vector();

    // Arithmetic

    /*! \brief This is the multiplication of all the RHS and LHS denominator, except this constraint's LHS denom, and all divided by an appropriate GCD
     * \sa inf::ConstraintSet::update_constraint_scale_factors() */
    Num m_lhs_scale;
    /*! \brief This is the multiplication of all the RHS and LHS denominator, except this constraint's RHS denom, and all divided by an appropriate GCD
     * \sa inf::ConstraintSet::update_constraint_scale_factors() */
    Num m_rhs_scale;
};

} // namespace inf
