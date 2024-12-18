#pragma once

#include "constraint.h"

/*! \defgroup infcons Inflation constraints
 * \ingroup infback
 * \brief To specify and manipulate inflation constraints of the form \f$\infq_{\infmarg_0\cdots\infmarg_{k-1}\infmargg} = \targetp_{\infmarg_0}\cdots\targetp_{\infmarg_{k-1}}\cdot\infq_{\infmargg}\f$
 * \details The top-level class is inf::ConstraintSet, which handles a set of inf::Constraint.
 * The user inputs constraints in the format described in inf::ConstraintParser, which is the class in charge of interpreting the user's text description of the constraints.
 * The class inf::DualVector represents a dual vector acting on a constraint, and
 * the class inf::Marginal is used to efficiently evaluate the inner product of an inf::DualVector with a deterministic inflation distribution.
 * */

namespace inf {

/*! \ingroup infcons
 * \brief A set of inf::Constraint that, together with the inflation size, determines the inflation problem at hand.
 * \details In the paper, a set of constraints is denoted \f$\constraintlist \subset \infconstraints\f$.
 * Each element \f$\constraintname = (\infmarg_0,\dots,\infmarg_{k-1},\infmargg) \in \constraintlist\f$ represents a single constraint and is represented, in the code, with the class inf::Constraint.
 *
 * The class inf::ConstraintSet is in particular in charge of coordinating the arithmetic of the internal inf::Constraint, see inf::ConstraintSet::update_constraint_scale_factors().
 * This is done, as explained in the paper, by computing adequate scale factors \f$\consscale\f$ that will eventually appear in the calculation
 * of inner products, see inf::Marginal.
 * Furthermore, the inf::ConstraintSet computes the maximal absolute value that a dual vector component can have, as explained in the paper. This is implemented in inf::ConstraintSet::update_constraint_scale_factors().
 * The methods that can modify the dual vector components (inf::ConstraintSet::set_dual_vector_from_quovec(), inf::ConstraintSet::read_dual_vector_from_file()) then check that the bound is respected.
 * The method inf::ConstraintSet::set_target_distribution() can change the denominator of the target distribution, and hence also the overflow bound.
 * To make sure that the internal quovecs always respect the bound, inf::ConstraintSet::set_target_distribution() resets the internal quovecs to be all zero.
 * */
class ConstraintSet : public util::Loggable {
  public:
    typedef std::shared_ptr<inf::ConstraintSet> Ptr;
    typedef std::shared_ptr<const inf::ConstraintSet> ConstPtr;

    /*! \brief A text description of the set of constraints, see inf::ConstraintParser for the format of each inf::Constraint::Description
     * \details Note that the user is allowed to input two or more identical constraints.
     * This is not recommended for performance since duplicate constraints are not removed. */
    typedef std::vector<inf::Constraint::Description> Description;

    /*! \brief The set of constraints is defined by an inf::Inflation and a list of inf::Constraint::Description
     * \param inflation The underlying inflation
     * \param constraints This describes the constraints to impose in the inflation problem, see inf::ConstraintParser for the relevant format.
     * \param store_bounds This indicates whether or not the underlying inf::DualVector should store upper/lower bounds
     * to allow for a branch-and-bound optimization of inner products. */
    ConstraintSet(inf::Inflation::ConstPtr const &inflation,
                  inf::ConstraintSet::Description const &constraints,
                  inf::DualVector::StoreBounds store_bounds);
    //! \cond
    ConstraintSet(ConstraintSet const &other) = delete;
    ConstraintSet(ConstraintSet &&other) = delete;
    ConstraintSet &operator=(ConstraintSet const &other) = delete;
    ConstraintSet &operator=(ConstraintSet &&other) = delete;
    //! \endcond

    // Logging

    void log() const override;
    /*! \brief Logs the current values held in each internal inf::DualVector */
    void log_dual_vectors() const;

    // Getters

    /*! \brief Returns the underlying inflation */
    inf::Inflation::ConstPtr const &get_inflation() const;
    /*! \brief Whether or not each underlying inf::DualVector members store upper/lower bounds */
    inf::DualVector::StoreBounds get_store_bounds() const;
    /*! \brief The size of the list representation of a quovec \f$\{\quovec_\constraintname\}_{\constraintname\in\constraintlist} \in \totquovecspace\f$
     * \details This is the sum of the size of the list representation of each \f$\quovec_\constraintname \in \quovecspace{\infmarg_{0}\cdots\infmarg_{k-1}\infmargg}\f$, where \f$\constraintname = (\infmarg_0,\dots,\infmarg_{k-1},\infmargg)\f$.
     * In other words, this is the sum of the number of event orbits of each constraint, relative to the relevant marginal symmetries \f$\marggroup\f$:
     * \f[
     *     \sum_{\constraintname = (\infmarg_0,\dots,\infmarg_{k-1},\infmargg)\in\constraintlist} | \events{\infmarg_0\dots\infmarg_{k-1}\infmargg} \setminus \marggroup |
     * \f] */
    Index get_quovec_size() const;
    /*! \brief The max overflow-safe value that can be held by a dual vector */
    Num get_max_dual_vector_component() const;
    /*! \brief The set of inf::Marginal::Evaluator, allowing to efficiently evaluate inner products */
    inf::Marginal::EvaluatorSet get_marg_evaluators() const;

    // Setters

    /*! \brief This updates every internal inf::Constraint with this target distribution
     * \sa inf::Constraint::set_target_distribution()
     * \warning This method resets the internal dual vector to zero! */
    void set_target_distribution(inf::TargetDistr const &d);
    /*! \brief This updates every internal inf::DualVector
     * \details This method should only be called after inf::ConstraintSet::set_target_distribution()
     *  has been called. This is checked internally.
     * \sa inf::Constraint::set_dual_vector_from_quovec()
     * \param coeffs This is a representation of a quovec \f$\{\quovec_\constraintname\}_{\constraintname\in\constraintlist} \in \totquovecspace\f$ */
    void set_dual_vector_from_quovec(inf::Quovec const &coeffs);

    // Evaluation

    /*! \brief This returns \f$\{\totconstraintmapelem(\detdistr\infevent)\}_{\constraintname\in\constraintlist}\f$
     * \details In other words, this returns the direct sum of each constraint's quovec,
     * \f[
     *     \{\quovec_\constraintname = \totconstraintmapelem(\detdistr\infevent) \in \quovecspace{\infmarg_0\dots\infmarg_{k-1}\infmargg}\}_{\constraintname\in\constraintlist} \in \totquovecspace,
     * \f]
     * where \f$\infevent\in\infevents\f$ is an inflation event,
     * \f$\detdistr\infevent\in\infqs\f$ is the deterministic inflation distribution with support on the event \f$\infevent\f$,
     * \f$\constraintname = (\infmarg_0,\dots,\infmarg_{k-1},\infmargg)\f$ is an inflation constraint,
     * and \f$\totconstraintmapelem(\infq) \sim \infq_{\infmarg_{0}\cdots\infmarg_{k-1}\infmargg} - \targetp_{\infmarg_0}\cdots\targetp_{\infmarg_{k-1}}\cdot\infq_{\infmargg}\f$ (in reality, this map is pre- and postprocessed with some symmetrization operation as described in the paper).
     * \sa inf::Constraint::compute_inflation_event_quovec()
     *  \param inflation_event Encodes a deterministic inflation distribution \f$\detdistr\infevent\f$.
     *  \return \f$\{\totconstraintmapelem(\detdistr\infevent)\}_{\constraintname\in\constraintlist}\f$ */
    inf::Quovec get_inflation_event_quovec(inf::Event const &inflation_event) const;

    /*! \brief This is the total scale factor \f$\scaletot \in \N\f$ that appears in both inner product evaluation and quovec calculations
     * \details The way that inner products
     * (see inf::ConstraintSet::get_marg_evaluators())
     * and quovecs
     * (see inf::ConstraintSet::get_inflation_event_quovec())
     * are obtained with our exact arithmetic is that they are \f$\scaletot \cdot \text{(true value)}\f$.
     * For minimizing inner products and checking their signs, this positive scale factor is irrelevant.
     * However, for numerical stability of the Frank-Wolfe algorithm, it is convenient to cancel out this scale factor,
     * and this method is used in inf::FrankWolfe::memorize_event_and_quovec().
     *  \return \f$\scaletot\in\N\f$ */
    double get_quovec_denom() const;

    // Serialization

    /*! \brief This saves the current dual vector held by inf::ConstraintSet to a text file.
     * \param filename The filename (without extension) to save the dual vector to
     * \param metadata An arbitrary string to give a little context in the file, e.g., a text description of which nonlocality problem was considered when obtaining this dual vector.
     * \details The idea is that one can run inf::FeasProblem::get_feasibility(), and then, if the resulting distribution is nonlocal, one can call inf::FeasProblem::save_dual_vector_to_file()
     * to store the corresponding nonlocality certificate for later re-use.
     *
     * \details This method can't be const, because of the util::Serializable and util::FileStream interface that does not allow (by design!) to
     * output const objects, see util::OutputFileStream. */
    void write_dual_vector_to_file(std::string const &filename, std::string const &metadata);

    /*! \brief This reads the current dual vector held by inf::ConstraintSet from a text file
     * \param filename The filename (without extension) to read the dual vector from
     * \param metadata This string needs to match the one passed to inf::ConstraintSet::save_dual_vector_to_file() */
    void read_dual_vector_from_file(std::string const &filename, std::string const &metadata);

  private:
    /*! \brief This is used by write_dual_vector_to_file() and read_dual_vector_from_file() */
    void io_dual_vector(util::FileStream &stream);

    /*! \brief The underlying inflation */
    const inf::Inflation::ConstPtr m_inflation;

    /*! \brief The list of inf::Constraint
     *  \details These are pointers to avoid having to have move constructors, required by std::vector */
    std::vector<inf::Constraint::UniquePtr> m_constraints;

    /*! \brief This is stored to be retrievable by the inf::Optimizer */
    inf::DualVector::StoreBounds m_store_bounds;

    /*! \brief The sum of each constraint's quovec size, see inf::ConstraintSet::get_quovec_size() */
    Index m_quovec_size;
    /*! \brief Initializes `m_quovec_size` */
    void init_quovec_size();

    // Arithmetic

    /*! \brief This is used to be able to bring back the quovecs into a normalized floating point form, see inf::ConstaintSet::get_quovec_denom() */
    double m_quovec_denom;
    /*! \brief This is the maximal value a dual vector entry can take to be guaranteed to never overflow, see inf::ConstraintSet::check_quovec_for_overflow() */
    Num m_max_dual_vector_component;
    /*! \brief This arithmetic method looks at the denominators of the LHS and RHS of each constraint, simplifies where possible, and assigns the appropriate scale factors to the constraints
     * \details The way this method works is explained in the paper. */
    void update_constraint_scale_factors();
    /*! \brief This method checks that the internal inf::DualVector held by each inf::Constraint has components no bigger than `m_max_dual_vector_component`.
     * \details The idea is that, when computing inner products with the ::Num arithmetic type, we want to avoid any overflow.
     * To do so, as described in the paper, it is sufficient to ensure that the components (values) of a quovec are not too large.
     * This method checks that the maximal value is respected, and ensures correct behavior.
     * In practice, we never encountered an issue with this since ::Num is capable of holding fairly large numbers.
     * However, the larger the denominator of the distribution and the larger the constraints (in the sense that they feature many products of \f$\targetp\f$ in their right-hand side),
     * the more likely this is to be triggered. This is thus a fundamental limit of the current implementation.
     * The alternative would be to use proper exact arithmetic types that can hold exactly arbitrarily large number, but this would probably slow down
     * the code significantly, which is not desired for the applications we had in mind when writing this code. */
    void hard_assert_quovecs_within_bound() const;
};

} // namespace inf
