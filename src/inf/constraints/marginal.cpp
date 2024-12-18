#include "marginal.h"
#include "../../util/debug.h"
#include "../../util/logger.h"
#include "dual_vector.h"

// inf::Marginal::Permutation

bool inf::Marginal::Permutation::operator<(inf::Marginal::Permutation const &other) const {
    ASSERT_EQUAL(outcome_sym.get_bare_sym().size(), other.outcome_sym.get_bare_sym().size())
    ASSERT_EQUAL(parties.size(), other.parties.size())

    if (outcome_sym < other.outcome_sym)
        return true;
    else if (other.outcome_sym < outcome_sym)
        return false;
    else
        return parties < other.parties;
}

// inf::Marginal::Evaluator

inf::Marginal::Evaluator::Evaluator(Index n_inflation_parties,
                                    Index n_marginal_parties,
                                    inf::Outcome n_outcomes,
                                    Index n_marg_perms,
                                    inf::Marginal::Evaluator::PartyToUpdateRules const &party_to_update_rules)
    : m_is_scalar_marginal(n_marginal_parties == 0),
      m_inflation_event(n_inflation_parties, inf::Outcome(0)),
      m_n_marginal_parties(n_marginal_parties),
      m_n_outcomes(n_outcomes),
      m_marg_event_hashes(n_marg_perms, Index(0)),
      m_party_to_update_rules(party_to_update_rules),
      m_dual_vector(nullptr),
      m_scale(nullptr) {}

void inf::Marginal::Evaluator::set_dual_vector_reference(inf::EventTensor const *dual_vector) {
    ASSERT_TRUE(m_dual_vector == nullptr)
    ASSERT_TRUE(dual_vector != nullptr)
    ASSERT_EQUAL(dual_vector->get_n_parties(), m_n_marginal_parties)
    m_dual_vector = dual_vector;
}

void inf::Marginal::Evaluator::set_scale_reference(Num const *scale) {
    ASSERT_TRUE(m_scale == nullptr)
    ASSERT_TRUE(scale != nullptr)
    m_scale = scale;
}

void inf::Marginal::Evaluator::set_outcome(Index const inflation_party, inf::Outcome const outcome) {
    ASSERT_LT(outcome, m_n_outcomes)
    ASSERT_LT(inflation_party, m_inflation_event.size())

    // Don't do anything if the outcome is already set
    if (m_is_scalar_marginal or outcome == m_inflation_event[inflation_party])
        return;

    for (inf::Marginal::Evaluator::UpdateRule const &update_rule : m_party_to_update_rules[inflation_party]) {
        inf::Outcome const new_outcome = update_rule.inverse_outcome_sym[outcome];
        inf::Outcome const old_outcome = update_rule.inverse_outcome_sym[m_inflation_event[inflation_party]];

        m_marg_event_hashes[update_rule.marg_perm_index] += (new_outcome - old_outcome) * update_rule.party_weight;
    }

    m_inflation_event[inflation_party] = outcome;
}

Num inf::Marginal::Evaluator::evaluate_dual_vector() const {
    ASSERT_TRUE(m_dual_vector != nullptr)
    ASSERT_TRUE(m_scale != nullptr)

    Num score = 0;

    if (m_is_scalar_marginal) {
        score = m_dual_vector->get_num(0);
    } else {
        for (Index const marg_event_hash : m_marg_event_hashes)
            score += m_dual_vector->get_num(marg_event_hash);
    }

    return (*m_scale) * score;
}

inf::Event const &inf::Marginal::Evaluator::get_inflation_event() const {
    return m_inflation_event;
}

// inf::Marginal::EvaluatorSet

inf::Marginal::EvaluatorSet::EvaluatorSet(std::vector<inf::Marginal::Evaluator> const &evaluators)
    : m_evaluators(evaluators) {}

Num inf::Marginal::EvaluatorSet::evaluate_dual_vector() const {
    Num ret = 0;
    for (inf::Marginal::Evaluator const &evaluator : m_evaluators)
        ret += evaluator.evaluate_dual_vector();

    return ret;
}

void inf::Marginal::EvaluatorSet::set_outcome(Index const inflation_party, inf::Outcome const outcome) {
    for (inf::Marginal::Evaluator &evaluator : m_evaluators)
        evaluator.set_outcome(inflation_party, outcome);
}

inf::Event const &inf::Marginal::EvaluatorSet::get_inflation_event() const {
    return m_evaluators[0].get_inflation_event();
}

// Marginal

inf::Marginal::Marginal(inf::Inflation::ConstPtr const &inflation,
                        std::vector<Index> const &marginal_parties,
                        inf::Symmetry::Group const &constraint_group,
                        inf::DualVector::StoreBounds store_bounds)
    : m_inflation(inflation),
      m_marginal_parties(marginal_parties),
      m_store_bounds(store_bounds),
      m_marginal_permutations{},
      m_denom(0), // will be initialized in init_marginal_permutations()
      m_marginal_symmetries{} {

    // The way we implement this class does not allow for repeated parties in the marginal
    {
        std::set<Index> const marginal_parties_set(marginal_parties.begin(), marginal_parties.end());
        HARD_ASSERT_EQUAL(marginal_parties_set.size(), marginal_parties.size())
    }

    init_marginal_symmetries(constraint_group);

    init_marginal_permutations();
}

void inf::Marginal::log() const {
    LOG_BEGIN_SECTION("inf::Marginal")

    util::logger
        << m_inflation->get_marginal_name(m_marginal_parties)
        << " = (" << util::cr;

    bool first = true;
    for (inf::Marginal::Permutation const &marg_perm : m_marginal_permutations) {
        if (first) {
            first = false;
            util::logger << "      ";
        } else {
            util::logger << "    + ";
        }
        util::logger << marg_perm.outcome_sym << "^{-1} . " << m_inflation->get_marginal_name(marg_perm.parties) << util::cr;
    }
    util::logger << ") / " << get_denom() << util::cr << util::cr;

    LOG_BEGIN_SECTION("Graphically")
    util::logger << util::cr;

    m_inflation->log_marginal(m_marginal_parties);
    util::logger << util::cr << "=" << util::cr << util::cr;

    bool is_first = true;
    for (inf::Marginal::Permutation const &marg_perm : m_marginal_permutations) {
        if (not is_first) {
            util::logger << "+" << util::cr << util::cr;
        } else {
            is_first = false;
        }
        util::logger << marg_perm.outcome_sym << "^{-1} . " << util::cr;
        m_inflation->log_marginal(marg_perm.parties);
        util::logger << util::cr;
    }

    util::logger << "/ " << get_denom() << util::cr;

    LOG_END_SECTION

    LOG_END_SECTION
}

inf::Inflation::ConstPtr const &inf::Marginal::get_inflation() const {
    return m_inflation;
}

std::vector<Index> const &inf::Marginal::get_parties() const {
    return m_marginal_parties;
}

Index inf::Marginal::get_n_parties() const {
    return m_marginal_parties.size();
}

Num inf::Marginal::get_denom() const {
    return m_denom;
}

std::vector<inf::Event> inf::Marginal::extract_marg_perm_events(inf::Event const &inflation_event) const {
    std::vector<inf::Event> marg_perm_events(m_marginal_permutations.size(), inf::Event(m_marginal_parties.size()));

    for (Index const i : util::Range(m_marginal_permutations.size()))
        extract_marg_perm_event(inflation_event, m_marginal_permutations[i], marg_perm_events[i]);

    return marg_perm_events;
}

void inf::Marginal::init_marginal_symmetries(inf::Symmetry::Group const &constraint_group) {
    ASSERT_EQUAL(m_marginal_symmetries.size(), 0)
    ASSERT_LT(0, constraint_group.size())

    if (m_marginal_parties.size() == 0)
        return;

    // These two are used to do the down-conversion
    std::map<Index, Index> inf_to_marg_index;
    std::map<Index, Index> marg_to_inf_index;
    for (Index const marg_party_index : util::Range(get_n_parties())) {
        Index const inf_party_index = m_marginal_parties[marg_party_index];

        inf_to_marg_index[inf_party_index] = marg_party_index;
        marg_to_inf_index[marg_party_index] = inf_party_index;
    }

    for (inf::Symmetry const &inf_sym : constraint_group) {

        inf::PartySym::Bare const &inf_party_sym = inf_sym.get_party_sym().get_bare_sym();
        inf::PartySym::Bare marg_party_sym(get_n_parties(), 0);

        for (Index const marg_party_index : util::Range(get_n_parties())) {
            marg_party_sym[marg_party_index] =
                // inf to marg
                inf_to_marg_index.at(
                    // inf symmetry
                    inf_party_sym[
                        // marg to inf
                        marg_to_inf_index.at(marg_party_index)]);
        }

        // Here expect that some symmetries will already be contained (because we only look at a sub-part of the inflation symmetry)
        m_marginal_symmetries.insert(inf::Symmetry(inf::PartySym(marg_party_sym), inf_sym.get_outcome_sym()));
    }
}

void inf::Marginal::init_marginal_permutations() {
    ASSERT_EQUAL(m_marginal_permutations.size(), 0)

    if (m_marginal_parties.size() == 0) {
        m_denom = 1;
        return;
    }

    // We expect some marginal permutations to be duplicate in the below loop (because some inflation symmetries
    // simply leave the marginal invariant), so we use a std::set to remove duplicates on the fly
    std::set<inf::Marginal::Permutation> marg_perm_set;

    // We first need to find all the transformations (denoted marg_perm in the following)
    // under the inflation symmetries (\mathbb{G}^p) of the identity marginal permutation (id,id_A)
    for (inf::Symmetry const &inf_sym : m_inflation->get_inflation_symmetries()) {

        std::vector<Index> transformed_marg_parties(get_n_parties(), 0);
        inf_sym.get_party_sym().act_entrywise(m_marginal_parties, transformed_marg_parties);
        inf::Marginal::Permutation const marg_perm(inf_sym.get_outcome_sym(), transformed_marg_parties);

        // By default, marg_perm is a representative of its orbit under marginal symmetries
        bool is_representative = true;

        // We now need to check if marg_perm is indeed a representative of its orbit
        for (inf::Symmetry const &marg_sym : m_marginal_symmetries) {

            inf::OutcomeSym const other_outcome_sym = marg_perm.outcome_sym.get_composition_after(marg_sym.get_outcome_sym().get_inverse());

            std::vector<Index> other_marg_parties(get_n_parties(), 0);
            marg_sym.get_party_sym().act_on_list(transformed_marg_parties, other_marg_parties);

            inf::Marginal::Permutation const other_marg_perm(other_outcome_sym, other_marg_parties);

            if (other_marg_perm < marg_perm) {
                is_representative = false;
                break;
            }
        }

        if (is_representative)
            marg_perm_set.insert(marg_perm);
    }

    m_marginal_permutations = std::vector<inf::Marginal::Permutation>(marg_perm_set.begin(), marg_perm_set.end());
    m_denom = m_marginal_permutations.size();
}

void inf::Marginal::extract_marg_perm_event(inf::Event const &inflation_event,
                                            inf::Marginal::Permutation const &marg_perm,
                                            inf::Event &dest) const {
    ASSERT_EQUAL(inflation_event.size(), m_inflation->get_n_parties())
    ASSERT_EQUAL(dest.size(), get_n_parties())
    ASSERT_TRUE(&inflation_event != &dest)

    for (Index i : util::Range(get_n_parties())) {
        dest[i] = marg_perm.outcome_sym.get_inverse_bare_sym()[inflation_event[marg_perm.parties[i]]];
    }
}

inf::Symmetry::Group const &inf::Marginal::get_marginal_symmetries() const {
    return m_marginal_symmetries;
}

inf::DualVector::StoreBounds inf::Marginal::get_store_bounds() const {
    return m_store_bounds;
}

inf::Marginal::Evaluator inf::Marginal::get_evaluator() const {
    std::vector<Index> party_weights = inf::DualVector::compute_weights(m_marginal_parties.size(),
                                                                        m_inflation->get_network()->get_n_outcomes(),
                                                                        m_store_bounds);

    inf::Marginal::Evaluator::PartyToUpdateRules party_to_update_rules{};

    for (Index inflation_party : util::Range(m_inflation->get_n_parties())) {
        party_to_update_rules.emplace_back();
        // The update rules of the inflation_party
        inf::Marginal::Evaluator::UpdateRules &update_rules = party_to_update_rules.back();

        // For each marginal permutation, we see if changing the outcome of the inflation_party
        // would have an impact on the corresponding marginal-permutation-extract marginal event
        for (Index const marg_perm_index : util::Range(m_marginal_permutations.size())) {
            inf::Marginal::Permutation const &marg_perm = m_marginal_permutations[marg_perm_index];

            for (Index const marginal_party : util::Range(get_n_parties())) {
                if (marg_perm.parties[marginal_party] == inflation_party) {
                    inf::OutcomeSym::Bare outcome_sym = marg_perm.outcome_sym.get_inverse_bare_sym();
                    // Need to extend the outcome symmetry to account for the unknown outcome,
                    // which does not undergo any transformation when we swap outcomes
                    if (m_store_bounds == inf::DualVector::StoreBounds::yes)
                        outcome_sym.push_back(static_cast<inf::Outcome>(outcome_sym.size()));

                    // Changing the outcome of the inflation event at position inflation_party
                    // has an impact on the marg_perm-extract marginal event at position marginal_party
                    update_rules.emplace_back(marg_perm_index, outcome_sym, party_weights[marginal_party]);
                    break;
                }
            }
        }
    }

    return inf::Marginal::Evaluator(m_inflation->get_n_parties(),
                                    get_n_parties(),
                                    inf::DualVector::get_outcomes_per_party(m_inflation->get_network()->get_n_outcomes(), m_store_bounds),
                                    m_marginal_permutations.size(),
                                    party_to_update_rules);
}
