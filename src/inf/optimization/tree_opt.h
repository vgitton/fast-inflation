#pragma once

#include "../events/event_tree.h"
#include "../events/tree_splitter.h"
#include "optimizer.h"

#include <shared_mutex>

namespace inf {

/*! \ingroup opt
 * \brief This inf::Optimizer is the most efficient one we used, it uses multi-threading and a branch-and-bound tree search to minimize inner products
 * \details This class efficiently minimizes an inf::DualVector, representing some \f$\quovec = \{\quovec_\constraintname\}_{\constraintname\in\constraintlist}\in\totquovecspace\f$, on the events of some inf::Inflation, i.e., solving the minimization problem
 * \f[
 *     \min_{\infevent\in\infevents} \inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist},
 * \f]
 * The optimizations that are implemented are as follows:
 * - First off, instead of looking at all \f$\infevent\in\infevents\f$, this class only looks at \f$\redinfevents \subset \infevents\f$, the reduced set of inflation events,
 *   exploiting the symmetries of the inflation (see inf::Inflation::get_inflation_symmetries()) as well as those of the target distribution (see inf::Inflation::UseDistrSymmetries).
 * - Then, the minimization is performed using a tree search based on the inf::EventTree that is returned by inf::Inflation::get_symtree().
 *   This allows to start computing inflation marginals high up in the tree, and to do the minimum amount of computations at the leaves of the tree.
 *   This works in tandem with the class inf::Marginal::Evaluator, which takes care of computing inner products in an efficient way.
 * - The tree search can be accelerated using a branch-and-bound approach (this is controlled by inf::DualVector::StoreBounds).
 *   The idea is that if we can already tell halfway down the tree that, no matter how we fill up the inflation event, the corresponding inner product will not be better than the
 *   current minimum, then we can discard (prune) this branch. This has proven to be extremely efficient for "random" initial inf::DualVector that the inf::FeasProblem
 *   start with, but as the inf::DualVector becomes more and more fine-tuned to try to prove that an inf::TargetDistr is incompatible with an inf::Inflation,
 *   this branch-and-bound approach becomes less useful.
 * - The tree search can be parallelized by specifying a number of threads. Using inf::TreeSplitter, the inf::EventTree is then split into approximately equal (in terms of number of leaves)
 *   subtrees, and each subtree is investigated in parallel to find its local minimum. When combined with the branch-and-bound approach, each thread informs the other threads of its best current
 *   minimum, such that the lower bounds on the local scores can be compared with the global minimum, potentially pruning many more branches.
 * */
class TreeOpt : public inf::Optimizer {
  public:
    /*! \brief This is the shared data between threads, storing the global smallest value found so far, and providing a thread-safe interface
     * \details Although we use this structure even without the branch-and-bound mechanism, it is really
     * only useful with the branch-and-bound approach. Indeed, without brach-and-bound, each thread
     * could simply compute it's own minimum, and we can then take the global minimum once every thread is finished. */
    class GlobalMinimum {
      public:
        /*! \brief Initializes `m_global_minimum` to the largest ::Num */
        GlobalMinimum();

        /*! \brief Sets `m_global_minimum = min(m_global_minimum, score)` in a thread-safe manner */
        void set_if_smaller(Num score);
        /*! \brief Gets the current value of `m_global_minimum` in a thread-safe manner */
        Num get() const;
        /*! \brief Reset `m_global_minimum` to the largest ::Num */
        void reset();

      private:
        /*! \brief The current global minimum accross all threads */
        Num m_global_minimum;
        /*! \brief This mutex allows threads to know when they are allowed to access `m_global_minimum`
         * \details This is `mutable` since we need to change its state in the `const` method inf::TreeOpt::GlobalMinimum::get(). */
        mutable std::shared_mutex m_shared_mutex;
    };

    /*! \brief This struct encapsulates the data that each thread works with
     * \details This allows in particular to conveniently initialize the storage of each thread is a single struct instantiation. */
    struct ThreadWorker {
        /*! \brief The \p marg_evaluators allow the inf::TreeOpt::ThreadWorker to evaluate the underlying inf::DualVector,
         * i.e., to evaluate the underlying inner products \f$\inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist}\f$. */
        ThreadWorker(inf::Marginal::EvaluatorSet const &marg_evaluators);

        /*! \brief To allow the inf::TreeOpt::ThreadWorker to evaluate the underlying inf::DualVector,
         * i.e., to evaluate the underlying inner products \f$\inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist}\f$. */
        inf::Marginal::EvaluatorSet marg_evaluators;
        /*! \brief The current best (i.e., minimum) score found by the thread
         * \details The idea is that each thread stores the current minimum it discovered itself,
         * together with the corresponding best event. It should be true at the end of the optimization that the
         * best current minimum of all threads equals the global minimum, which is asserted */
        Num current_minimum;
        /*! \brief %Inflation event corresponding to the score of `inf::TreeOpt::CurrentMinimum::current_minimum` */
        inf::Event current_best_event;
        /*! \brief This stores the last depth of the tree that was processed, allowing to reset the outcomes stored in `inf::TreeOpt::CurrentMinimum::marg_evaluators`
         * to the unknown outcome */
        Index last_depth_processed;
        /*! \brief The stack of fill requests to be processed in inf::TreeOpt::go_down_from() */
        inf::EventTree::NodePos::Queue queue;
        /*! \brief The number of leaves perceived thanks to the branch-and-bound approach
         * \details If the \p constraints passed to inf::TreeOpt::TreeOpt() have `constraints.get_store_bounds == inf::DualVector::StoreBounds::no`,
         * then `n_leaves_effective` will just be the number of leaves of the subtree that the inf::TreeOpt::ThreadWorker considers.
         * If instead `constraints.get_store_bounds == inf::DualVector::StoreBounds::yes`, meaning that the branch-and-bound approach is used,
         * this is the number of effective leaves perceived, i.e., when a branch containing, say, 10 leaves is discarded from a non-terminal depth,
         * this counts as one effective leave.
         * \sa inf::ConstraintSet::get_store_bounds() */
        Index n_leaves_effective;
    };

    /*! \brief This is what each thread returns, essentially an inf::Optimizer::PreSolution
     * \details The overall solution will then be the minimum of the solutions of the different threads. */
    struct ThreadReturn {
        /*! \param pre_solution The minimum inner product score obtained by the thread
         * \param n_leaves_effective The number of leaves perceived thanks to the branch-and-bound approach, see inf::TreeOpt::ThreadWorker::n_leaves_effective. */
        ThreadReturn(inf::Optimizer::PreSolution &&pre_solution, Index const n_leaves_effective);

        /*! \brief The inf::Optimizer::PreSolution holding the minimum score and the corresponding optimizer inflation event
         * found by the thread */
        inf::Optimizer::PreSolution const pre_solution;
        /*! \brief See inf::TreeOpt::ThreadWorker::n_leaves_effective */
        Index const n_leaves_effective;
    };

    /*! \brief Initialize with the relevant set of inf::Constraint, whether or not to read/write the inf::EventTree from disk, and the number of threads to use
     * \param constraints The relevant set of inf::Constraint defining the inner products \f$\inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist}\f$.
     * \param symtree_io How to obtain the reduced set of inflation events \f$\redinfevents\f$, see inf::EventTree::IO.
     * \param n_threads The number of threads to use to accelerate the tree search. */
    TreeOpt(inf::ConstraintSet::Ptr const &constraints,
            inf::EventTree::IO symtree_io,
            Index n_threads);
    //! \cond
    TreeOpt(TreeOpt const &) = delete;
    TreeOpt(TreeOpt &&) = delete;
    TreeOpt &operator=(TreeOpt const &) = delete;
    TreeOpt &operator=(TreeOpt &&) = delete;
    //! \endcond

    inf::Optimizer::PreSolution get_pre_solution() override;

    void log_info() const override;

    void log_status() const override;

  private:
    /*! \brief The relevant set of inf::Constraint defining the inner products \f$\inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist}\f$ */
    inf::EventTree const *m_event_tree;
    /*! \brief How to obtain the reduced set of inflation events \f$\redinfevents\f$, see inf::EventTree::IO */
    inf::EventTree::IO const m_symtree_io;
    /*! \brief To allow each inf::TreeOpt::ThreadWorker to evaluate the underlying inf::DualVector
     * \details I.e., to evaluate the underlying inner products \f$\inner{\quovec}{\totconstraintmap(\detdistr\infevent)}_{\constraintlist}\f$.
     * This will be copied in each inf::TreeOpt::ThreadWorker. */
    inf::Marginal::EvaluatorSet const m_marg_evaluators;
    /*! \brief The number of threads to use to accelerate the tree search. */
    Index const m_n_threads;
    /*! \brief When `m_n_threads > 1`, this contains a partition of `m_event_tree` into `m_n_threads` approximately equal subtrees
     * \sa inf::TreeSplitter::get_path_partition() */
    inf::TreeSplitter::PathPartitionConstPtr m_path_partition;
    /*! \brief This is `constraints.get_store_bounds() == inf::DualVector::StoreBounds::yes`, which is stored as a boolean for readability in this class code
     * \sa inf::ConstraintSet::get_store_bounds() */
    bool const m_store_bounds;
    /*! \brief The number \f$|\infparties|\f$ of inflation parties */
    Index const m_inflation_n_parties;
    /*! \brief The unknown outcome "?", stored as \f$\nouts+1\f$, where \f$\nouts\f$ is the physical number of outcomes per party */
    inf::Outcome const m_outcome_unknown;
    /*! \brief The sum of the n_leaves_effective of the thread workers, see inf::TreeOpt::ThreadWorker::n_leaves_effective */
    Index m_n_leaves_effective;
    /*! \brief The thread-safe readable/writable global minimum value shared by the threads */
    GlobalMinimum m_global_minimum;
    /*! \brief This function is a single-threaded minimization
     * \details It minimizes the inner products using inf::TreeOpt::m_marg_evaluators
     * over all inflation events of inf::TreeOpt::m_event_tree that are contained in the subtree described by \p paths.
     * The parallelization is then performed by creating multiple threads running this function in parallel with different \p paths. */
    inf::TreeOpt::ThreadReturn thread_opt(std::vector<inf::TreeSplitter::Path> const &paths);
    /*! \brief This function investigates a single node of inf::TreeOpt::m_event_tree
     * \details This function assumes that \p thread_worker contains (within inf::TreeOpt::ThreadWorker::marg_evaluators)
     * a partially filled inflation event up to `node_pose.depth-1` included.
     * It then fills the outcome of the inflation event according to the node of inf::TreeOpt::m_event_tree at position `node_pos`.
     * Depending on inf::TreeOpt::m_store_bounds, if possible, it does nothing (meaning that the current branch is no good for minimization).
     * Otherwise, it appends the children of \p node_pos to `thread_worker.queue` and returns.
     * If \p node_pos is a terminal node of the inf::TreeOpt::m_event_tree, then it evaluates the inner product and calls
     * inf::TreeOpt::GlobalMinimum::set_if_smaller() on inf::TreeOpt::m_global_minimum. */
    void go_down_from(inf::TreeOpt::ThreadWorker &thread_worker,
                      inf::EventTree::NodePos const &node_pos);
};

} // namespace inf
