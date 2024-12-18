#pragma once

#include "../../util/chrono.h"
#include "../inf_problem/inflation.h"
namespace inf {
class Marginal;
}
#include "dual_vector.h"

/*! \file */

namespace inf {

/*! \ingroup infcons
 * \brief This class represents an inflation marginal \f$\infq_\infmarg\f$ for some set
 * of inflation parties \f$\infmarg \subset \infparties\f$ and allows to efficiently evaluate inner products
 * \details To solve inflation problems with a Frank-Wolfe algorithm, and to verify nonlocality certificates,
 * as shown in the paper, we require to evaluate the following expression:
 * \f[
 *      \consscale \cdot \sum_{\margperm \in \redpermutedmargs\infmarg} F(\extract{\infevent}{\margperm}),
 * \f]
 * where
 * - \f$\consscale \in \Z\f$ is a signed integer scale factor, incurred to be able to perform evaluation of inner products in integer arithmetic,
 * - \f$\infmarg \subset \infparties\f$ indicates an inflation marginal.
 * - \f$\infevent\in\infevents\f$ is an inflation event.
 * - \f$\redpermutedmargs\infmarg\f$ is the set of reduced marginal permutations associated to the symmetries of the inflation and the marginal \f$\infmarg\f$.
 *   More specifically, we define \f$\redpermutedmargs\infmarg = \repr{\permutedmargs\infmarg \setminus \marggroup}\f$,
 *   where \f$\marggroup\f$ describes the group of constraint symmetries of some constraint \f$\constraintname = (\infmarg_0,\dots,\infmarg_{k-1},\infmargg)\in \infconstraints\f$, and where \f$\marggroup\f$ acts on \f$\permutedmargs\infmarg\f$ with the action denoted \f$\actt{\marggroup}{\permutedmargs\infmarg}\f$ in the paper.
 *   Recall that we have either \f$\infmarg = \infmarg_0\cup\cdots\cup\infmarg_{k-1}\cup\infmargg\f$ or \f$\infmarg = \infmargg\f$.
 *   The set \f$\permutedmargs\infmarg\subset\margperms\infmarg\f$ describes the set of marginal permutations obtained as \f$\act{\distrgroup}{\margperms\infmarg}(\gid, \id{\infmarg})\f$, where \f$\distrgroup\f$ is described in inf::Inflation::UseDistrSymmetries, and where \f$(\gid,\id\infmarg) \in \margperms\infmarg\f$ describes the trivial marginal permutation.
 * - \f$\extract{\infevent}{\margperm} \in \events\infmarg\f$ is the marginal inflation event extracted from \f$\infevent\f$ thanks to the marginal permutation \f$\margperm\f$.
 * - \f$F \in \vecset{\infmarg}\f$ is a dual vector that is in practice of the form \f$F = \altquovecembed\infmarg(\quovec)\f$ for some quotiented vector \f$\quovec \in \quovecspace\infmarg\f$.
 *
 * The class inf::Marginal allows to efficiently evaluate such expressions.
 *
 * This class is tested in the application user::inflation_marginal.
 * */
class Marginal : public util::Loggable {
  public:
    typedef std::unique_ptr<inf::Marginal> UniquePtr;

    /*! \brief This describes a marginal permutation \f$(\sigma,\pi) \in \margperms\infmarg\f$
     * with \f$\sigma\f$ an outcome permutation and \f$\pi : \infmarg \to \infparties\f$ */
    struct Permutation {
        /*! \brief The outcome symmetry \f$\sigma \in \outgroup\f$ */
        inf::OutcomeSym outcome_sym;
        /*! \brief The party map \f$\pi : \infmarg \to \infparties\f$ */
        std::vector<Index> parties;

        /*! \brief This provides the ordering necessary to extract representatives out of orbits of marginal permutations */
        bool operator<(inf::Marginal::Permutation const &other) const;
    };

    /*! \brief This class contains minimal data allowing to perform the operations described in inf::Marginal
     * \details The idea is that the inf::Marginal::Evaluator contains "compiled" data from an inf::Marginal,
     * allowing efficient extraction of marginal events from marginal permutations \f$\redpermutedmargs\infmarg\f$
     * and an inflation event \f$\infevent\in\infevents\f$.
     * Furthermore, this is a minimal data structure that will be duplicated to be held by different threads
     * in multithreading applications.
     * Internally, it stores a representation of the set
     * \f[
     *     \{\extract{\infevent}{\margperm}\}_{\margperm\in\redpermutedmargs\infmarg},
     * \f]
     * and allows the user to update individual outcomes of the inflation event \f$\infevent\f$, which updates the above set accordingly.
     * This allows the tree-based evaluation of inner products (see inf::TreeOpt) to perform the minimum amount of work
     * at terminal nodes.
     * */
    class Evaluator {
      public:
        typedef std::unique_ptr<inf::Marginal::Evaluator> UniquePtr;

        /*! \brief An update rule is implicitly associated to an inflation party and specifies what to do when the outcome of an inflation party changes
         * \details The update rule specifies which marginal-permutation-extracted event to update (based on \p marg_perm_index),
         * and how to modify it:
         * - we need to know which inverse outcome symmetry to apply with \p inverse_outcome_sym
         * - we need to know which marginal party of the marginal event is impacted by a change of outcome to the original inflation party.
         *   In principle, this could be specified by the index of the marginal party in the marginal event, but it is slightly more efficient
         *   to directly store the weight of that marginal party. The reason is that we eventually want to query inf::EventTensor::get_num(),
         *   which works by hashing the event first, and the weight of the marginal party allows to directly manipulate the hashes. */
        struct UpdateRule {
            /*! \brief Specifies which term (which marginal event) to update */
            Index marg_perm_index;
            /*! \brief The inverse outcome symmetry to be appledi */
            inf::OutcomeSym::Bare inverse_outcome_sym;
            /*! \brief This identifies the marginal party within the marginal event */
            Index party_weight;
        };

        /*! \brief Changing the outcome of an inflation party typically implies to modify several of the marginal events,
         * and thus, each inflation party is assigned a list of such update rules, see inf::Marginal::Evaluator::UpdateRule */
        typedef std::vector<inf::Marginal::Evaluator::UpdateRule> UpdateRules;

        /*! \brief Each inflation party is associated to its own update rules, see inf::Marginal::Evaluator::UpdateRules */
        typedef std::vector<inf::Marginal::Evaluator::UpdateRules> PartyToUpdateRules;

        /*! \brief Initializing an inf::Marginal::Evaluator consists mostly of passing it the adequate inf::Marginal::Evaluator::PartyToUpdateRules
         * \param n_inflation_parties The number of parties \f$|\infparties|\f$ of the underlying inf::Inflation (see inf::Inflation::get_n_parties())
         * \param n_marginal_parties The number of parties \f$|\infmarg|\f$ of the underlying inf::Marginal (see inf::Marginal::get_n_parties())
         * \param n_outcomes The number of outcomes, including the unknown outcome or not depending on the parameter inf::DualVector::StoreBounds passed
         * to the constructor of the underlying inf::Marginal
         * \param n_marg_perms The number of marginal permutations \f$|\redpermutedmargs\infmarg|\f$, corresponding to the number of marginal events
         * that are being extracted
         * \param party_to_update_rules See inf::Marginal::Evaluator::PartyToUpdateRules for more details */
        Evaluator(Index n_inflation_parties,
                  Index n_marginal_parties,
                  inf::Outcome n_outcomes,
                  Index n_marg_perms,
                  inf::Marginal::Evaluator::PartyToUpdateRules const &party_to_update_rules);

        Evaluator(Evaluator const &) = default;
        Evaluator(Evaluator &&) = default;
        Evaluator &operator=(Evaluator const &) = delete;
        Evaluator &operator=(Evaluator &&) = delete;

        /*! \brief This sets the reference \f$F \in \vecset\infmarg\f$ to be evaluated as described in inf::Marginal
         * \details It is implicitly assumed that \f$F = \altquovecembed\infmarg(\quovec)\f$
         * for some \f$\quovec \in \quovecspace\infmarg\f$. */
        void set_dual_vector_reference(inf::EventTensor const *dual_vector);

        /*! \brief This sets the reference scale \f$\consscale\f$ that will be multiplied to the result of the evaluation described in inf::Marginal
         * \details This scale depends in practice on the target distribution \f$\targetp\f$, which means that it will typically change
         * during nonlocality tests of successive distributions, as e.g. done in inf::VisProblem.
         * Furthermore, this scale depends on the overall constraint set \f$\constraintlist \subset \infconstraints\f$,
         * which is why this is computed by inf::ConstraintSet and then passed here as a reference. */
        void set_scale_reference(Num const *scale);

        /*! \brief This modifies one outcome of the underlying inflation event \f$\infevent\in\infevents\f$ as described in inf::Marginal::Evaluator
         * \param inflation_party The inflation party whose outcome needs to be set to \p outcome.
         * The parameter \p inflation_party must be between `0` and `inf::Inflation::get_n_parties()-1`, both included.
         * \param outcome To be set as the new outcome of \p inflation_party */
        void set_outcome(Index const inflation_party, inf::Outcome const outcome);

        /*! \brief Evaluates the dual vector \f$F\f$ (passed to inf::Marginal::Evaluator::set_dual_vector_reference())
         * as described in inf::Marginal */
        Num evaluate_dual_vector() const;

        /*! \brief This allows to retrieve the underlying inflation event \f$\infevent\in\infevents\f$, this is useful
         * for retrieving the optimal inflation event minimizing an inner product */
        inf::Event const &get_inflation_event() const;

      private:
        /*! \brief It is useful to allow for the case \f$\infmarg = \emptyset\f$, in which case this is `true` */
        bool const m_is_scalar_marginal;

        /*! \brief This underlying inflation event \f$\infevent\in\infevents\f$
         * \details This is needed in particular to know how to update `m_marg_event_hashes` when calling inf::Marginal::Evaluator::set_outcome()
         * This could probably be shared across all the different evaluators, but it is easier to implement
         * like this and does not incur a significant overhead. */
        inf::Event m_inflation_event;

        /*! \brief The number of parties \f$|\infmarg|\f$ in the underlying inflation marginal */
        Index const m_n_marginal_parties;
        /*! \brief The number of outcomes (which may or may not take into account the unknown outcome, depending on the value of inf::DualVector::StoreBounds
         * passed to the underlying inf::Marginal)
         * \details This is used to validate the input of inf::Marginal::Evaluator::set_outcome() */
        inf::Outcome const m_n_outcomes;
        /*! \brief For each marginal permutation \f$\margperm\in\redpermutedmargs\infmarg\f$ of the underlying inf::Marginal,
         * we store the marginal event \f$\extract{\infevent}{\margperm}\f$ (where \f$\infevent\f$ is the underlying inflation event)
         * as the hash (integer) that is used internally by inf::EventTensor, so that we can directly use it when we call inf::EventTensor::get_num() */
        std::vector<Index> m_marg_event_hashes;

        /*! \brief See inf::Marginal::Evaluator::PartyToUpdateRules for more details */
        inf::Marginal::Evaluator::PartyToUpdateRules const m_party_to_update_rules;

        /*! \brief The reference dual vector \f$F\f$ to be evaluated */
        inf::EventTensor const *m_dual_vector;

        /*! \brief The reference multiplicative scale constant \f$\consscale\f$ */
        Num const *m_scale;
    };

    /*! \brief This class is a convenience for storing multiple inf::Marginal::Evaluator and adding up their results */
    class EvaluatorSet {
      public:
        /*! \brief Initialization with a list of inf::Marginal::Evaluator */
        EvaluatorSet(std::vector<Evaluator> const &evaluators);

        /*! \brief This evaluates each internal inf::Marginal::Evaluator and returns the sum of their results */
        Num evaluate_dual_vector() const;

        /*! \brief This sets the outcome of the specified inflation party for each internal inf::Marginal::Evaluator
         * \sa inf::Marginal::set_outcome() */
        void set_outcome(Index const inflation_party, inf::Outcome const outcome);

        /*! \brief All internal inf::Marginal::Evaluator store the same underlying inflation event \f$\infevent\f$ which is returned here */
        inf::Event const &get_inflation_event() const;

      private:
        /*! \brief The list of inf::Marginal::Evaluator */
        std::vector<Evaluator> m_evaluators;
    };

    /*! \brief Initialize with an inf::Inflation and a set of inflation party \f$\infmarg\subset\infparties\f$ essentially
     * \details The inf::EventTensor \f$F\f$ relative to which the evaluation will take place will be specified in inf::Marginal::Evaluator.
     * \param inflation The inflation on which the inf::Marginal lives.
     * \param marginal_parties The set of inflation parties \f$\infmarg\subset\infparties\f$ that describes the marginal we're referring to
     * \param constraint_group The group of constraint symmetries \f$\marggroup\f$ obtained from an inf::Constraint.
     * The relation between \f$\infmarg\f$ and \f$\constraintname = (\infmarg_{0},\dots,\infmarg_{k-1},\infmargg)\in\infconstraints\f$ is
     * either \f$\infmarg = \infmarg_{0}\cup\dots\cup\infmarg_{k-1}\cup\infmargg\f$ or \f$\infmarg = \infmargg\f$, depending on whether
     * we are dealing with the inflation marginal appearing in the left-hand side or the right-hand sideof the constraint \f$\constraintname\f$.
     * \param store_bounds We need to specify whether or not the relevant inf::EventTensor, storing the dual vector \f$F\f$, stores upper or lower bounds
     * allowing the branch-and-bound acceleration of the inflation event tree. Indeed, effectively, this changes the number of outcomes of the relevant
     * inf::EventTensor, and this changes the representation of marginal events in inf::Marginal::Evaluator.
     * */
    Marginal(inf::Inflation::ConstPtr const &inflation,
             std::vector<Index> const &marginal_parties,
             inf::Symmetry::Group const &constraint_group,
             inf::DualVector::StoreBounds store_bounds);
    //! \cond
    Marginal(inf::Marginal const &other) = delete;
    Marginal(inf::Marginal &&other) = delete;
    inf::Marginal &operator=(inf::Marginal const &other) = delete;
    inf::Marginal &operator=(inf::Marginal &&other) = delete;
    //! \endcond

    // Public interface

    void log() const override;
    /*! \brief Returns the underlying inflation */
    inf::Inflation::ConstPtr const &get_inflation() const;
    /*! \brief Returns the set \f$\infmarg\f$, stored in `m_marginal_parties`, as inflation indices (see inf::Inflation::get_party_index()) */
    std::vector<Index> const &get_parties() const;
    /*! \brief Returns the size \f$|\infmarg|\f$ of inflation parties, i.e., `m_marginal_parties.size()` */
    Index get_n_parties() const;
    /*! \brief This returns the number \f$|\redpermutedmargs\infmarg|\f$ of relevant marginal permutations, unless we are dealing
     * with a trivial marginal \f$\infmarg = \emptyset\f$, in which case this just returns 1 */
    Num get_denom() const;

    /*! \brief Extract the set of marginal events \f$\extract{\infevent}{(\sigma,\pi)} \in \events{\infmarg}\f$ from the inflation event \f$\infevent\in\infevents\f$
     * for all marginal permutations \f$(\sigma,\pi) \in \redpermutedmargs\infmarg\f$
     * \details Recall that \f$\redpermutedmargs\infmarg\f$ is stored in the member `m_marginal_permutations` */
    std::vector<inf::Event> extract_marg_perm_events(inf::Event const &inflation_event) const;

    /*! \brief Extract the marginal event \f$\extract{\infevent}{(\sigma,\pi)} \in \events{\infmarg}\f$ from the inflation event \f$\infevent\in\infevents\f$ based on the marginal permutation \f$(\sigma,\pi)\in\margperms\infmarg\f$
     * \details Recall that, as defined in the paper, \f$\extract{\infevent}{(\sigma,\pi)} = \sigma^{-1} \circ \infevent \circ \pi\f$.
     * \param inflation_event The event \f$\infevent \in \infevents\f$
     * \param marg_perm The marginal permutation \f$(\sigma,\pi)\in\margperms\infmarg\f$
     * \param dest This is updated with \f$\extract{\infevent}{(\sigma,\pi)}\f$. \p dest need to already have the expected size
     * given by inf::Marginal::get_n_parties(), which is just \f$|\infmarg|\f$. */
    void extract_marg_perm_event(inf::Event const &inflation_event,
                                 inf::Marginal::Permutation const &marg_perm,
                                 inf::Event &dest) const;

    /*! \brief The symmetries of the marginal, i.e., a representation of the group \f$\marggroup\f$ passed in the constructor
     * \details This is useful to intialize the symmetries of the inf::DualVector acting on this marginal */
    inf::Symmetry::Group const &get_marginal_symmetries() const;

    /*! \brief This specifies whether or not this inf::Marginal expects to evaluate a dual vector (stored as inf::EventTensor)
     * that stores bounds, see inf::DualVector::StoreBounds */
    inf::DualVector::StoreBounds get_store_bounds() const;

    /*! \brief This initializes and returns an adequate inf::Marginal::Evaluator */
    inf::Marginal::Evaluator get_evaluator() const;

  private:
    /*! \brief The underlying inflation */
    const inf::Inflation::ConstPtr m_inflation;

    /*! \brief Returns the set \f$\infmarg\f$ as inflation indices (see inf::Inflation::get_party_index()) */
    const std::vector<Index> m_marginal_parties;

    /*! \brief This specifies whether or not this inf::Marginal expects to evaluate a dual vector (stored as inf::EventTensor)
     * that stores bounds, see inf::DualVector::StoreBounds */
    const inf::DualVector::StoreBounds m_store_bounds;

    /*! \brief This is the set \f$\redpermutedmargs\infmarg = \repr{\permutedmargs\infmarg \setminus \marggroup}\f$, the reduced set (relative to the constraint symmetries \f$\marggroup\f$ stored in `m_marginal_symmetries`) of marginal permutations
     * \details This is used to evaluate inner products efficiently. It is initialized in inf::Marginal::init_marginal_permutations() */
    std::vector<inf::Marginal::Permutation> m_marginal_permutations;
    /*! \brief This initializes `m_marginal_permutations` */
    void init_marginal_permutations();

    /*! \brief The denominator is either `m_marginal_permutations.size()` or simply 1 if the inf::Marginal is empty/trivial
     * \details This is initialized in inf::Marginal::init_marginal_permutations() */
    Num m_denom;

    /*! \brief This stores \f$\actt{\marggroup}{\permutedmargs\infmarg}\f$, i.e., the constraint symmetries with inf::PartySym acting on the marginal parties only */
    inf::Symmetry::Group m_marginal_symmetries;
    /*! \brief This initializes `m_marginal_symmetries` */
    void init_marginal_symmetries(inf::Symmetry::Group const &constraint_group);
};
} // namespace inf
