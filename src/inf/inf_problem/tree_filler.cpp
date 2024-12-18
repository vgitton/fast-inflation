#include "tree_filler.h"
#include "../../util/debug.h"
#include "../../util/logger.h"

void inf::TreeFiller::fill_tree(inf::EventTree &tree, inf::Inflation const &inflation) {
    inf::TreeFiller TreeFiller(tree, inflation);
}

inf::TreeFiller::TreeFiller(inf::EventTree &tree,
                            inf::Inflation const &inflation)
    : m_sym_chrono(util::Chrono::State::paused),
      m_node_chrono(util::Chrono::State::paused),
      m_tree(tree),
      m_inflation(inflation),
      m_inflation_inverse_party_symmetries(),
      m_inflation_outcome_symmetries(),
      m_inflation_n_parties(inflation.get_n_parties()),
      m_unknown_outcome(inflation.get_network()->get_outcome_unknown()),
      m_n_outcomes(inflation.get_network()->get_n_outcomes()),
      m_working_event(inflation.get_all_unknown_event()),
      m_current_syms(inflation.get_n_parties()) {
    // Initialize the flattened symmetries
    {
        // vector rather than set
        m_inflation_inverse_party_symmetries.reserve(m_inflation.get_inflation_symmetries().size());
        m_inflation_outcome_symmetries.reserve(m_inflation.get_inflation_symmetries().size());

        for (inf::Symmetry const &sym : m_inflation.get_inflation_symmetries()) {
            m_inflation_inverse_party_symmetries.emplace_back(sym.get_party_sym().get_inverse_bare_sym());
            m_inflation_outcome_symmetries.emplace_back(sym.get_outcome_sym().get_bare_sym());
        }
        ASSERT_EQUAL(m_inflation_inverse_party_symmetries.size(), m_inflation.get_inflation_symmetries().size())
        ASSERT_EQUAL(m_inflation_outcome_symmetries.size(), m_inflation.get_inflation_symmetries().size())

        m_current_syms[0] = std::vector<Index>(m_inflation.get_inflation_symmetries().size());
        for (Index i : util::Range(m_inflation.get_inflation_symmetries().size())) {
            m_current_syms[0][i] = i;
        }
    }

    util::Chrono total_symtree_chrono(util::Chrono::State::running);
    std::vector<Index> root_children = this->find_children(0);
    (void)root_children;
    total_symtree_chrono.pause();

    m_tree.finish_initialization();

    ASSERT_LT(0, m_tree.get_root_children_count())

    // LOGGING
    {
        LOG_BEGIN_SECTION("inf::TreeFiller timing")
        util::logger
            << util::begin_comment << "Event symmetrization: " << util::end_comment
            << m_sym_chrono << util::cr
            << util::begin_comment << "Node insertion in cache: " << util::end_comment
            << m_node_chrono << util::cr
            << util::begin_comment << "Total: " << util::end_comment
            << total_symtree_chrono << util::cr;
        LOG_END_SECTION
    }
}

std::vector<Index> inf::TreeFiller::find_children(Index depth) {
    ASSERT_LT(depth, m_inflation_n_parties)
    ASSERT_EQUAL(m_working_event[depth], m_unknown_outcome)

    // Will be returned
    std::vector<Index> child_nodes;

    std::vector<Index> const &current_syms = m_current_syms[depth];
    bool const not_at_last_depth = (depth < m_inflation_n_parties - 1);
    // We use forward declarations, this is maybe a bit faster
    Index depth_bis;
    // This will only be relevant if we're not at last depth
    std::vector<Index> *next_syms = nullptr;
    if (not_at_last_depth)
        next_syms = &m_current_syms[depth + 1];
    inf::OutcomeSym::Bare const *outcome_sym;
    inf::PartySym::Bare const *inverse_party_sym;
    inf::Outcome base_outcome, transformed_outcome;
    inf::Outcome &outcome_to_fill = m_working_event[depth];

    // We fill outcome_to_fill = m_working_event[depth] with all possible values,
    // and for each value, we check whether or not m_working_event is symmetrized
    for (outcome_to_fill = 0; outcome_to_fill < m_n_outcomes; ++outcome_to_fill) {
        // By default, m_working_event is symmetrized, but if we find a symmetry that
        // proves it is not symmetrized, is_symmetrized will be set to false
        bool is_symmetrized = true;

        if (not_at_last_depth)
            next_syms->clear();

        // This loop is in charge of verifying whether or not m_working_event is symmetrized,
        // which is stored in is_symmetrized, and it furthermore adequately initializes the
        // next_syms for the next recursive call
        m_sym_chrono.start();
        for (Index const sym_index : current_syms) {
            outcome_sym = &m_inflation_outcome_symmetries[sym_index];
            inverse_party_sym = &m_inflation_inverse_party_symmetries[sym_index];

            // We want to compare event_bis := party_sym(outcome_sym(m_working_event))
            // with m_working_event with respect to the lexicographic ordering.
            // If event_bis < m_working_event, then m_working_event is not symmetrized
            // no matter how we will fill it, and we dismiss m_working_event.
            // If event_bis > m_working_event, then it can be that the current symmetry indexed
            // by sym_index will not help to symmetrize the event no matter how we fill it
            // and we can hence dismiss sym_index from the next_syms.
            for (depth_bis = 0; depth_bis < m_inflation_n_parties; ++depth_bis) {
                // This is (party_sym(m_working_event))[depth_bis] = m_working_event[inverse_party_sym[depth_bis]]
                transformed_outcome = m_working_event[(*inverse_party_sym)[depth_bis]];

                // This happens when e.g. m_working_event = (2,?,?,...) and party_sym(m_working_event) = (?,2,?,...)
                // In this case, event_bis > m_working_event, but we cannot say whether
                // or not the sym_index will help to symmetrize m_working_event when it
                // will be filled more, so we add sym_index to the next_syms.
                // Furthermore, we do not know whether or not m_working_event is symmetrized
                // (if all outcome symmetries are there, (0,?,?,...) will be the symmetrized one),
                // so we do not touch is_symmetrized before breaking
                //
                // The other case where this can happen is if e.g., m_working_event = (0,0,?)
                // and the symmetry index by sym_index is just the outcome swap of 2 and 3.
                // The above reasoning applies equally well in that case.
                if (transformed_outcome == m_unknown_outcome) {
                    if (not_at_last_depth)
                        next_syms->push_back(sym_index);

                    break;
                }

                base_outcome = m_working_event[depth_bis];
                ASSERT_LT(base_outcome, m_unknown_outcome)

                // We now have transformed_outcome = party_sym(outcome_sym(m_working_event))[depth_bis]
                transformed_outcome = (*outcome_sym)[transformed_outcome];

                // If the transformed event is "lower" than the working event, exit the loop concluding
                // that m_working_event is not symmetrized. In this case, no need to update the next_syms,
                // because there is anyway no child of this outcome (it is dismissed because not symmetrized)
                if (transformed_outcome < base_outcome) {
                    is_symmetrized = false;
                    break;
                }

                // In this case, the symmetry transformation takes the event up in its orbit
                // This will remain true when filling it further.
                // This symmetry is thus "irrelevant" for future depths: it has no chance
                // of symmetrizing the event. We don't add it to next_syms.
                if (transformed_outcome > base_outcome)
                    break;
            }

            // If this event is not symmetrized, we can stop. Note that in this case, the value of next_syms
            // is irrelevant because this node will have no children.
            if (not is_symmetrized)
                break;
        }
        m_sym_chrono.pause();

        // We dismiss m_working_event with the current outcome_to_fill based on symmetry
        if (not is_symmetrized)
            continue;

        std::vector<Index> children{};
        if (not_at_last_depth) {
            children = this->find_children(depth + 1);

            // Dismiss empty childset, unless we're at last depth
            // This can happen if e.g. (0,1,?) looks symmetrized,
            // but somehow all the filling (0,1,x) are not symmetrized.
            if (children.size() == 0)
                continue;
        }

        // We can now construct the node!
        m_node_chrono.start();
        child_nodes.push_back(m_tree.insert_node(depth, outcome_to_fill, children));
        m_node_chrono.pause();
    }

    // We need to reset m_working_event[depth] to "?" so that the outer level
    // of the recursion still has an event where depth+1,depth+2 etc have all "?" outcomes
    outcome_to_fill = m_unknown_outcome;

    return child_nodes;
}
