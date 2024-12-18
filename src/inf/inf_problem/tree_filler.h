#pragma once

#include "../../util/chrono.h"
#include "../events/event_tree.h"
#include "../inf_problem/inflation.h"

namespace inf {

/*! \ingroup infback
 * \brief This class takes care of filling an inf::EventTree that stores the symmetrized inflation events \f$\redinfevents\f$
 * \details
 * Let \f$G\f$ be the symmetry group of the inflation, obtained with inf::Inflation::get_inflation_symmetries().
 * Recall that \f$G\f$ is either just \f$\sourcegroup\f$ or the full \f$\distrgroup \subset \sourcegroup \times \partygroup \times \outgroup\f$
 * (as described in inf::Inflation::UseDistrSymmetries) depending on how the inf::Inflation was initalized.
 * As described in the paper, the group \f$G\f$ acts on the inflation events \f$\infevents\f$.
 * We can thus construct the orbits of the inflation events, \f$\infevents \setminus G\f$, and the representative events according to the lexicographic
 * ordering of \f$\infevents\f$ are denoted \f$\redinfevents\f$.
 * The idea of the lexicographic ordering is that an event, described in the paper as a function \f$\infevent \in \infevents\f$,
 * i.e., \f$\infevent : \infparties \to \outset\f$, has an image of the form
 * \f$\infevent(\alice_{00}) = a_{00}\f$, \f$\infevent(\alice_{01}) = a_{01}\f$, etc.
 * The class inf::Inflation chooses a canonical ordering of the inflation events, which we take here to be \f$\alice_{00},\alice_{01},\dots\f$ for simplicity
 * (this is not the actual ordering, but the actual ordering is irrelevant).
 * The event \f$\infevent\f$ is then stored as the list \f$(a_{00},a_{01},\dots)\f$.
 * The ordering between events is then obtained by thinking of \f$a_{00}a_{01}\cdots\f$ as a base-\f$\nouts\f$ (the number of outcomes per party) number.
 *
 * Then, we store the events in \f$\redinfevents\f$ as a tree.
 * For simplicity, suppose that \f$\redinfevents = \{(00),(01)\}\f$.
 * We would then store this as the tree
 * ```
 *   0
 *  / \
 * 0   1
 * ```
 *
 * Naively, to obtain this tree, one would first store the full set of events \f$\infevents\f$,
 * extract the representatives \f$\redinfevents\f$, and then attempt to store them as a tree.
 * However, this would be too costly when the inflation has many parties (and hence many, many events).
 * Instead, we use an algorithm that works as follows.
 * Suppose that the set of events is simply \f$\{0,1\}^{\times 3}\f$,
 * and the symmetry group \f$G\f$ only consists of all party exchanges, i.e., of a \f$\shortsymgroup3\f$ party symmetry group.
 * The set of symmetrized events would then be \f$\{(0,0,0), (0,0,1), (0,1,1), (1,1,1)\}\f$.
 * Here is the algorithm (in pseudo-C++) that we use to obtain this set of symmetric events based on the tree approach.
 * ```
 *     // We start with an "all-unknown" event
 *     working_event = (?,?,?);
 *     symmetrized_events = {};
 *     // Fill in first_outcome = 0
 *     working_event = (0,?,?);
 *     // This looks symmetrized, we continue
 *     {
 *         // Fill in second_outcome = 0
 *         working_event = (0,0,?);
 *         // This looks symmetrized, we continue
 *         {
 *             // Fill in third_outcome = 0
 *             working_event = (0,0,0);
 *             // This is symmetrized, we add it to the set of symmetrized events
 *             symmetrized_events.push_back(working_event);
 *
 *             // Fill in third_outcome = 1
 *             working_event = (0,0,1);
 *             // This is symmetrized, we add it to the set of symmetrized events
 *             symmetrized_events.push_back(working_event);
 *         }
 *
 *         // Fill in second_outcome = 1
 *         working_event = (0,1,?)
 *         // This looks symmetrized, we continue
 *         {
 *             // Fill in third_outcome = 0
 *             working_event = (0,1,0);
 *             // This is not symmetrized, the exchange of parties 1 and 2 lead to
 *             // working_event = (0,0,1) which is lower in the lexicographic ordering,
 *             // so we do not add it to the set of symmetrized events
 *
 *             // Fill in third_outcome = 1
 *             working_event = (0,1,1);
 *             // This is symmetrized, we add it to the set of symmetrized events
 *             symmetrized_events.push_back(working_event);
 *         }
 *     }
 *
 *     // Fill in first_outcome = 1
 *     working_event = (1,?,?)
 *     // This looks symmetrized, we continue
 *     {
 *         // Fill in second_outcome = 0
 *         working_event = (1,0,?);
 *         // -> We can already tell that this is not symmetrized!
 *         // Indeed, exchanging parties 0 and 1 lead to working_event = (0,1,?),
 *         // which is lower in the lexicographic ordering (we treat the symbol "?" as
 *         // after the last outcome, i.e., ? = 2 in this case with two outcomes).
 *         // In other words, no matter how we continue to fill in working_event,
 *         // we will find events that are not symmetrized. Thus, we dismiss this branch
 *         // entirely, which for large trees lead to a big speedup.
 *
 *         // Fill in second_outcome = 1
 *         working_event = (1,1,?)
 *         // This looks symmetrized, we continue
 *         {
 *             // Fill in third_outcome = 0
 *             working_event = (1,1,0);
 *             // This is not symmetrized, the exchange of parties 0 and 2 lead to
 *             // working_event = (0,1,1) which is lower in the lexicographic ordering,
 *             // so we do not add it to the set of symmetrized events
 *
 *             // Fill in third_outcome = 1
 *             working_event = (1,1,1);
 *             // This is symmetrized, we add it to the set of symmetrized events
 *             symmetrized_events.push_back(working_event);
 *         }
 *     }
 * ```
 * This example describes how this algorithm would be used to obtain a list representation of \f$\redinfevents\f$,
 * but it is easy to adapt it to obtain a tree representation instead, which is what the class inf::TreeFiller does.
 *
 * This class is tested in the application user::inf_tree_filler.
 * */
class TreeFiller {
  public:
    /*! \brief To store the symmetrized events \f$\redinfevents\f$ of the \p inflation in the \p tree
     * \details This convenience static method avoids the use of declaring an otherwise-unused instance of inf::InfTreeSplitter. */
    static void fill_tree(inf::EventTree &tree, inf::Inflation const &inflation);

    /*! \brief To store the symmetrized events \f$\redinfevents\f$ of the \p inflation in the \p tree */
    TreeFiller(inf::EventTree &tree,
               inf::Inflation const &inflation);

  private:
    /*! \brief This util::Chrono tracks the time spent checking whether or not events are symmetrized */
    util::Chrono m_sym_chrono;
    /*! \brief This util::Chrono tracks the time spent inserting nodes in the underlying inf::EventTree */
    util::Chrono m_node_chrono;

    /*! \brief The inf::EventTree to fill */
    inf::EventTree &m_tree;
    /*! \brief The inf::Inflation whose symmetrized event set \f$\redinfevents\f$ will be stored in the inf::EventTree */
    inf::Inflation const &m_inflation;

    /*! \brief We store the image of the inflation inverse party symmetries for faster execution */
    std::vector<inf::PartySym::Bare> m_inflation_inverse_party_symmetries;
    /*! \brief We store the image of the inflation outcome symmetries for faster execution */
    std::vector<inf::OutcomeSym::Bare> m_inflation_outcome_symmetries;
    /*! \brief The number of inflation parties */
    Index const m_inflation_n_parties;
    /*! \brief The unknown outcome */
    inf::Outcome const m_unknown_outcome;
    /*! \brief The number of outcomes, which is equal to `m_unknown_outcome` */
    inf::Outcome const m_n_outcomes;

    /*! \brief This recursive function implements the tree-filling algorithm presented above
     * \details A call to `find_children(depth)` assumes that `m_working_event` is set to something like
     * `(0,1,0,2,?,?,?)`, where the first "?" is at position `depth`.
     * Then, `find_children(depth)` fills the outcome at position `depth` with the different possible outcomes,
     * checks whether the event is clearly non-symmetrized (in which case the attempted outcome at position `depth` is dismissed),
     * but if not, goes on to call `find_children(depth+1)`.
     * If the call to `find_children(depth+1)` returns a non-empty set of children, a new node is inserted in `m_tree` at position `depth`
     * with the outcome tried at position `depth`.
     * Eventually, when all outcomes at position `depth` have been tried, the resulting indices of the nodes inserted in `m_tree`
     * (obtained with inf::EventTree::insert_node()) are returned. */
    std::vector<Index> find_children(Index depth);

    /*! \brief The event that is used (modified and read) by the recursive function inf::TreeFiller::find_children() */
    inf::Event m_working_event;

    /*! \brief The list (for each depth) of lists of relevant symmetries that is used (modified and read) by the recursive function inf::TreeFiller::find_children()
     * \details The outer list corresponds to depth in the tree, the inner list consists of the indices of symmetries
     * (relative to both `m_inflation_inverse_party_symmetries` and `m_inflation_outcome_symmetries`)
     * that are relevant at the specified depth.
     * The idea is that a call of `find_children[depth]` will use only the symmetries specified in `m_current_syms[depth]`, because
     * the symmetries not specified in `m_current_syms[depth]` had a reason to be discarded.
     * Then, in `find_children[depth]`, it may be that upon setting `m_working_event[depth]` to a specific outcome, it is clear
     * that a symmetry will be irrelevant when trying to symmetrize `m_working_event` for all further fillings.
     * In this case, and only in this case, this symmetry is ommitted from `m_current_syms[depth+1]`. */
    std::vector<std::vector<Index>> m_current_syms;
};
} // namespace inf
