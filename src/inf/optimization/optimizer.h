#pragma once

#include "../../util/frac.h"
#include "../constraints/constraint_set.h"
#include "../inf_problem/target_distr.h"

/*! \file */

/*! \defgroup opt Linear minimization oracles
 * \ingroup infback
 * \brief This module describes how to efficiently minimize an inf::DualVector, representing some \f$\quovec = \{\quovec_\constraintname\}_{\constraintname\in\constraintlist}\in\totquovecspace\f$, on the events of some inf::Inflation, i.e., solving the minimization problem
 * \f[
 *     \min_{\infevent\in\infevents} \inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist},
 * \f]
 * where \f$\constraintlist \subset \infconstraints\f$ describes the constraints of an inflation problem (see inf::ConstraintSet).
 * The class inf::Optimizer provides the general interface, and the two implementations that we propose are as follows:
 * - inf::BruteForceOpt simply loops over all events \f$\infevent \in \infevents\f$.
 *   This is mostly used to test that other means of optimizaitons give the expected results on relatively small instances.
 * - inf::TreeOpt only looks at the symmetrized events \f$\infevent \in \redinfevents\f$, represented as a tree with inf::Inflation::get_symtree().
 *   It performs a tree search, and can be accelerated using a branch-and-bound approach, see inf::DualVector::StoreBounds.
 * */

namespace inf {

/*! \ingroup opt
 * \brief This class defines the interface for inf::BruteForceOpt and inf::TreeOpt
 * \details It describes how to efficiently minimize an inf::DualVector, representing some \f$\quovec = \{\quovec_\constraintname\}_{\constraintname\in\constraintlist}\in\totquovecspace\f$, on the events of some inf::Inflation, i.e., solving the minimization problem
 * \f[
 *     \min_{\infevent\in\infevents} \inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist},
 * \f]
 * where \f$\constraintlist \subset \infconstraints\f$ describes the constraints of an inflation problem (see inf::ConstraintSet).
 * */
class Optimizer {
  public:
    /*! \brief The total time spent in inf::Optimizer::optimize()
     * \details Note that this is not reset when running different inf::FeasProblem within an inf::VisProblem. */
    static util::Chrono total_optimization_chrono;

    typedef std::shared_ptr<inf::Optimizer> Ptr;

    /*! \brief This describes how the inf::Optimizer should stop: should it really minimize an inf::DualVector, or merely check that a negative or zero score can be achieved? */
    enum class StopMode {
        sat, ///< This means that the optimization stops when a value smaller or equal to zero is obtained
        opt  ///< This means that the optimization stops when the minimal value is obtained
    };

    static void log(inf::Optimizer::StopMode stop_mode);

    /*! \brief This is the base of the inf::Solution returned by an inf::Optimizer, containing the optimizer (an inflation event) and its score */
    class PreSolution {
      public:
        /*! \brief Initialize this with the score of the optimizer and the optimizer
            \param inflation_event_score the score of \p inflation_event, i.e., \f$\inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist}\f$
            for the relevant quovec \f$\quovec \in \totquovecspace\f$.
            \param inflation_event The event \f$\infevent\in\infevents\f$ that scores \p inflation_event_score */
        PreSolution(Num inflation_event_score,
                    inf::Event const &inflation_event);

        /*! \brief The score of \p inflation_event, i.e., \f$\inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist}\f$
            for the relevant quovec \f$\quovec \in \totquovecspace\f$. */
        const Num inflation_event_score;
        /*! \brief The event \f$\infevent\in\infevents\f$ that scores \p inflation_event_score */
        const inf::Event inflation_event;
    };

    /*! \brief To store an inf::Optimizer::PreSolution along with additional context */
    class Solution : public util::Loggable {
      public:
        /*! \brief Takes an inf::Optimizer::PreSolution along with the context in which it was obtained
            \param pre_sol The optimizer and its score
            \param inflation The inflation on which the optimizer event lives
            \param stop_mode Describes whether the inf::Optimizer::Solution is a satisfiability or optimization problem solution */
        Solution(inf::Optimizer::PreSolution const &pre_sol,
                 inf::Inflation::ConstPtr const &inflation,
                 inf::Optimizer::StopMode stop_mode);

        void log() const override;

        /*! \brief Get the score of the optimizer inflation event */
        Num get_inflation_event_score() const;
        /*! \brief Get the optimizer inflation event */
        inf::Event const &get_inflation_event() const;

      private:
        /*! \brief The optimizer and its score */
        const inf::Optimizer::PreSolution m_pre_sol;
        /*! \brief The inflation on which the optimizer event lives */
        const inf::Inflation::ConstPtr m_inflation;
        /*! \brief Describes whether we had a satisfiability or optimization problem */
        const inf::Optimizer::StopMode m_stop_mode;
    };

    /*! \brief Describes the search strategy: how should one tackle the problem of enumerating all relevant inflation events?
     * \details This is is used in in::Optimzier::get_optimizer() to conveniently select the subclass of inf::Optimizer
     * that should be instantiated. */
    enum class SearchMode {
        brute_force, ///< Simply loop over all events. Suitable for small inflations. See inf::BruteForceOpt.
        tree_search, ///< The symmetrized events are stored as a tree and a tree search is performed to solve the minimization. See inf::TreeOpt.
    };

    static void log(inf::Optimizer::SearchMode search_mode);

    /*! \brief To conveniently instantiate a subclass of inf::Optimizer
        \param search_mode This parameter defines which subclass should be called, i.e., inf::BruteForceOpt or inf::TreeOpt for now.
        \param constraints An optimizer gets access to an inf::ConstraintSet, which is typically used outside of the optimizer.
        This parameter allows to forward the inf::Inflation at hand, but also the current value of the inf::DualVector to optimize
        through inf::ConstraintSet::get_marg_evaluators().
        \param symtree_io This parameter is only relevant when `search_mode == inf::Optimizer::SearchMode::tree_search`, see inf::EventTree::IO for more details.
        \param n_threads This parameter is only relevant when `search_mode == inf::Optimizer::SearchMode::tree_search`, see inf::TreeOpt for more details.
        \return An adequate instance of a subclass of inf::Optimizer */
    static inf::Optimizer::Ptr get_optimizer(inf::Optimizer::SearchMode search_mode,
                                             inf::ConstraintSet::Ptr const &constraints,
                                             inf::EventTree::IO symtree_io,
                                             Index n_threads);

    /*! \brief This general constructor simply initializes the member `m_constraints`
        \param constraints An optimizer gets access to an inf::ConstraintSet, which is typically used outside of the optimizer.
        This parameter allows to forward the inf::Inflation at hand, but also the current value of the inf::DualVector to optimize
        through inf::ConstraintSet::get_marg_evaluators(). */
    Optimizer(inf::ConstraintSet::Ptr const &constraints);
    //! \cond ignore deleted
    Optimizer(inf::Optimizer const &other) = delete;
    Optimizer(inf::Optimizer &&other) = delete;
    Optimizer &operator=(inf::Optimizer const &other) = delete;
    Optimizer &operator=(inf::Optimizer &&other) = delete;
    //! \endcond

    /*! \brief This method is in charge of producing the optimizer (depending on `m_stop_mode`, see inf::Optimizer::StopMode) and its score */
    virtual inf::Optimizer::PreSolution get_pre_solution() = 0;

    /*! \brief This method invokes inf::Optimizer::get_pre_solution() and dresses up the solution
        \param stop_mode Updates `m_stop_mode` so that inf::Optimzier::get_pre_solution() can use it
        \return The optimizer, its score, and extra context about the target inf::TargetDistr and inf::Inflation */
    inf::Optimizer::Solution optimize(inf::Optimizer::StopMode stop_mode);

    /*! \brief Logs information about the inf::Optimizer to `util::logger`, this is meant to be called on initializing an inf::FeasProblem */
    virtual void log_info() const {}

    /*! \brief Logs information about the optimization status, this is meant to be called regularly within the inf::FeasProblem */
    virtual void log_status() const {}

  protected:
    /*! \brief This parameter allows to forward the inf::Inflation at hand, but also the current value of the inf::DualVector to optimize
        through inf::ConstraintSet::get_marg_evaluators(). */
    inf::ConstraintSet::Ptr m_constraints;

    /*! \brief See inf::Optimizer::StopMode. This attribute is set by inf::Optimizer::optimize(), so that it can be accessed within inf::Optimizer::get_pre_solution() overrides.
     * \details This is a member variable to be easily accessed by the child classes of inf::Optimizer.
     * Otherwise, if it was just a parameter of inf::Optimizer::get_pre_solution(), it would also need to be forwarded to all the other child functions. */
    inf::Optimizer::StopMode m_stop_mode;
};

} // namespace inf
